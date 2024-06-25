#ifndef GLASSD_H
#define GLASSD_H

#include <QObject>
extern "C"{
    #include <libevdev/libevdev.h>
    #include <libevdev/libevdev-uinput.h>

    #include <omniGlass/constants.h>
    #include <omniGlass/omniglass.h>

}
class Glassd: public QObject
{
    Q_OBJECT
public:
    //enumerations used while operating Glassd's core logic.
    enum class tracking_mode: int
    {
        INHIBIT = 0, //not mapping any gestures.
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

    Glassd():
        movement_deadzone {GLASSD_DEFAULTS_TOUCHPAD_DEADZONE},
        edge_slide_accumulated {0.0},
        edge_slide_threshold {GLASSD_DEFAULTS_TAB_MOVEMENT_THRESHOLD},
        tapped {0},
        finger_pressed {0},
        current_mode {tracking_mode::MENU},
        touchpad_specifications {NULL},
        single_finger_last_state {0,0,0},
        current_report {NULL},
        handle {NULL},
        execution_status {status::blank}
    {
        init();
    }

    //void run();

    //EVENTUALLY FIX THIS PLEASE:
    //  destructor should gracefully free structures for
    //  libevdev, omniglass, file descriptors, etc.
    ~Glassd(){};



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

    //enumerations for state machine
    enum class dragging_action_state { STARTED, DRAGGING, CROSSED };

    //gesture state
    double movement_deadzone{};
    double edge_slide_accumulated{};
    double edge_slide_threshold{};
    int tapped{};
    int finger_pressed{};
    tracking_mode current_mode{};

    //omniglass C interface wrapped inside private members.
    struct omniglass *handle;
    omniglass_raw_specifications *touchpad_specifications{};
    omniglass_raw_touchpoint single_finger_last_state{};
    omniglass_raw_report *current_report{};
    double drag_to_glassing_accumulator;
    struct libevdev_uinput *virtual_keyboard;
    // struct libevdev_uinput *virtual_touchpad;

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

//legacy c code interop
public:
    void refresh_edge_slide(double amount){
        this->edge_slide_accumulated += amount;
    }
};

#endif // GLASSD_H
