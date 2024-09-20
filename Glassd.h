#ifndef GLASSD_H
#define GLASSD_H

#include <QObject>
#include <map>

#include "Gesture_mapper.h"
#include "Gesture_handlers.h"

#include <list>

extern "C"{
    #include <libevdev/libevdev.h>
    #include <libevdev/libevdev-uinput.h>

    #include <omniGlass/constants.h>
    #include <omniGlass/omniglass.h>

    #include <stdio.h>
}
class Glassd: public QObject
{
    Q_OBJECT
public:
    //enumerations used while operating Glassd's core logic.
    enum class tracking_mode : int {
        INHIBIT = 0,         //not mapping any gestures.
        INHIBIT_TO_GLASSING, //mode-switch gesture is in progress (finishing leads to GLASSING)
        GLASSING, //mode-switch happened, gestures now will lead to modes that perform keyboard/mouse macros.
        MENU, //the gesture assigned to menu navigation was triggered. Focus a menu element and trigger it.
        JINX //the state machines are stuck. You might want to re-init Glassd.
    };
    enum class status: int
    {
        blank = 0, //Glassd was just created. Honestly this state is just a detail of the C++ QObject binding.
        no_virtual_input, //Glassd couldn't initialize completely. Best case: no virtual input. Worst case: krash!
        ready, //Glassd is in the proper state to scan for gestures on the next step() call.
        reconfiguring, //Glassd is recovering from some invalid state right now, so step() will not work.
        krashed //Glassd is borked. Stuff will not work. You might want to reset it..
    };

    //detailed parameters related to real-time interaction between a hand and the touchpad.
    //  honestly this should be computed at the omniglass lua layer,
    //  but right now glassd is the only application using it.
    struct touchpad_tracking_data
    {
        uint8_t touch_count_transition = 255; //0 is no touch; touch-counts are between 1 and 23; 255 is no-change;
        float single_touch_drag_x = 0;
        float single_touch_drag_y = 0;
        bool is_on_top_edge = false;
        bool is_on_bottom_edge = false;
        bool is_on_left_edge = false;
        bool is_on_right_edge = false;
        float rotation = 0;  //(NOT IMPLEMENTED) angle since last observation, in radians.
        int multifinger = 0; //(NOT IMPLEMENTED)
        float drags[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    };

    Glassd()
        : movement_deadzone{GLASSD_DEFAULTS_TOUCHPAD_DEADZONE}
        , edge_slide_accumulated{0.0}
        , edge_slide_threshold{GLASSD_DEFAULTS_TAB_MOVEMENT_THRESHOLD}
        , tapped{0}
        , finger_pressed{0}
        , current_mode{tracking_mode::INHIBIT}
        , touchpad_specifications{NULL}
        , single_finger_last_state{0, 0, 0}
        , current_report{NULL}
        , handle{NULL}
        , execution_status{status::blank}
    {
        init();
    }


    void change_gesture_mapper(QString name);

    //void run();

    //EVENTUALLY FIX THIS PLEASE:
    //  destructor should gracefully free structures for
    //  libevdev, omniglass, file descriptors, etc.
    ~Glassd(){};
    //GAHAHAHAH I'M UNEMPLOYED AND DROWNED IN CALCULUS HOMEWORK
    //i ain't fixing it.

public slots:
    //void glassd_run();

    //set up Glassd to start tracking gestures with step().
    void init();

    //compute all gesture stuff that happened between last step() call and right now.
    void step();

    //query status, as in: is it broken? is it running properly? is it stuck?
    status get_status();

    tracking_mode get_current_tracking_mode();

    //int focused_application_changed(QString application_canonical_name);
signals:
    //(NOTIMPL) emitted when the gesture tracker changes gesture mapping mode.
    void touchpad_mode_switched(tracking_mode mode);

    //you can ignore most of the following implementation details,
    //unless you're actually modifying Glassd's behavior and not the GUI around it.
private:

    //constants
    const double GLASSD_DEFAULTS_TOUCHPAD_DEADZONE = 0.1;
    const double GLASSD_DEFAULTS_TAB_MOVEMENT_THRESHOLD = 1.6;

    const int DEVICE_DESCRIPTION_END = -1;

    const double DEFAULT_POINT_RADIUS = 24.1;
    const double DEFAULT_DRAG_TRIGGER_DISTANCE = 25;

    QString current_application_canonical_id = "";

    //enumerations for state machine
    enum class dragging_action_state { STARTED, DRAGGING, CROSSED };

    //gesture state
    double movement_deadzone{};
    double edge_slide_accumulated{};
    double edge_slide_threshold{};
    int tapped{};
    int finger_pressed{};
    tracking_mode current_mode{};
    touchpad_tracking_data gesture_data; //fine-grained gesture parameter data.

    //gesture mappers with profiles
    std::map<QString, Gesture_mapper *> mappers;
    std::map<QString, QString> application_classes;
    Gesture_mapper::gesture_action current_action;
    Gesture_mapper *current_gesture_mapper;

    //gesture handlers with internal state for each gesture
    Gesture_handlers::Handler_drag handler_drag;

    //...
    //egads, this is getting big.
    //should i be shoving this much into a single class?

    //omniglass C interface wrapped inside private members.
    struct omniglass *handle;
    omniglass_raw_specifications *touchpad_specifications{};
    omniglass_raw_touchpoint single_finger_last_state{};
    omniglass_raw_report *current_report{};
    omniglass_raw_report previous_report{4,
                                         (omniglass_raw_touchpoint *) malloc(
                                             sizeof(omniglass_raw_touchpoint) * 4)};
    double drag_to_glassing_accumulator;
    struct libevdev_uinput *virtual_keyboard;
    // struct libevdev_uinput *virtual_touchpad;

    int raw_touchpad_file_descriptor{-1};

    status execution_status;

    //basic linear algebra for custom touchpad gestures
    double euclidean_distance(omniglass_raw_touchpoint p0, omniglass_raw_touchpoint p1);

    //utility functions for input device emulation
    void register_input_codes(struct libevdev *handle, int codes[]);
    //
    //SHORTHAND FUNCTIONS: virtual keypresses
    //
    void press_F_to_pay_respects(struct libevdev_uinput *handle);
    void generate_tab(struct libevdev_uinput *handle);
    void generate_shift_tab(struct libevdev_uinput *handle);
    void generate_menu(struct libevdev_uinput *handle);
    void generate_space_tap(struct libevdev_uinput *handle);
    void generate_enter_tap(struct libevdev_uinput *handle);

    //state machine maintenance operations
    void glassd_update_edge(double amount, void *data);
    void check_points(omniglass_raw_report *report, void *data);
    void modeswitch(tracking_mode new_mode);

    //gesture tracking and mapping maintenance
    void compute_gesture_parameters();
    Gesture_mapper::gesture_action compute_gesture_next_action();

    //
    //OFFLOAD: bits of code too long to put inside the usual functions
    // and too small in significance to the semantics.
    void generate_default_mappers();

public:
    //stuff that the state machine code will use
    void simulate_keycodes(std::list<__u16> keycodes);

    //legacy c code interop
    void refresh_edge_slide(double amount){
        this->edge_slide_accumulated += amount;
    }

    QString get_current_application_canonical_id() const;
    void setCurrent_application_canonical_id(const QString &newCurrent_application_canonical_id);
};

#endif // GLASSD_H
