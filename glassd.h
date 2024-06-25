#ifndef GLASSD_H
#define GLASSD_H

#include <QObject>
extern "C"{
    #include <libevdev/libevdev.h>
    #include <libevdev/libevdev-uinput.h>

    #include <omniGlass/constants.h>
    #include <omniGlass/omniglass.h>
}
class glassd: public QObject
{
    Q_OBJECT
public:
    //enumerations used while operating glassd's core logic.
    enum class tracking_mode { INHIBIT, INHIBIT_TO_GLASSING, GLASSING, MENU, JINX };
    enum class status { blank, no_virtual_input, ready, reconfiguring, krashed };

    glassd():
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


    void init();
    //void run();

    //EVENTUALLY FIX THIS PLEASE:
    //  destructor should gracefully free structures for
    //  libevdev, omniglass, file descriptors, etc.
    ~glassd(){};

    status get_status();

public slots:
    //void glassd_run();
    void step();
    //int focused_application_changed(QString application_canonical_name);
signals:
    int touchpad_mode_switched();

    //you can ignore most of the following implementation details,
    //unless you're actually modifying glassd's behavior and not the GUI around it.
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
    void glassd_update_edge(double amount, void *data);
    void check_points(omniglass_raw_report *report, void *data);

};
#endif // GLASSD_H
