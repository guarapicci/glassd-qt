
#include "Glassd.h"

extern "C" {
#include <asm-generic/errno-base.h>
#include <linux/input-event-codes.h>

#include <stdio.h>
#include <unistd.h>
#include <math.h>

}

void glassd_handle_edge_swipe(double swipe_amount, void *passthrough){
    Glassd *instance = (Glassd *) passthrough;
    instance->refresh_edge_slide(swipe_amount);
}

//linear algebra
double Glassd::euclidean_distance(omniglass_raw_touchpoint p0, omniglass_raw_touchpoint p1){
    return sqrt((pow((p1.x - p0.x), 2) + pow((p1.y - p0.y),2)));
}

//utility function for registering all evdev input event codes sent by our virtual input devices.
//array of input codes must be terminated by {-1, -1}.
//this is similar in practice to HID's device descriptors
void Glassd::register_input_codes(struct libevdev *handle, int codes[]){
    for (int i=0; codes[i] != -1 && codes[i+1] != -1; i+=2){
        libevdev_enable_event_code(handle, codes[i], codes[i+1], NULL);
    }
}

//check overall status of the touchpad handler
Glassd::status Glassd::get_status()
{
    return execution_status;
}

Glassd::tracking_mode Glassd::get_current_tracking_mode()
{
    return current_mode;
}

QString Glassd::get_current_application_canonical_id
    () const
{
    return current_application_canonical_id;
}

void Glassd::setCurrent_application_canonical_id(const QString &newCurrent_application_canonical_id)
{
    current_application_canonical_id = newCurrent_application_canonical_id;
}


//
//SHORTHAND FUNCTIONS: virtual keypresses
//
void Glassd::press_F_to_pay_respects(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_F, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_F, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}

void Glassd::generate_tab(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_TAB, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_TAB, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}

void Glassd::generate_shift_tab(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_LEFTSHIFT, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_TAB, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_TAB, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_LEFTSHIFT, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}

void Glassd::generate_menu(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_F10, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_F10, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}

void Glassd::generate_space_tap(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_SPACE, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_SPACE, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}
void Glassd::generate_enter_tap(struct libevdev_uinput *handle){
    libevdev_uinput_write_event(handle, EV_KEY, KEY_ENTER, 1);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(handle, EV_KEY, KEY_ENTER, 0);
    libevdev_uinput_write_event(handle, EV_SYN, SYN_REPORT, 0);
}
void Glassd::glassd_update_edge(double amount, void *data){
    Glassd *state = (Glassd *)data;
    if (amount > state->movement_deadzone
        || amount < (-1 * state->movement_deadzone))
    {
        state->edge_slide_accumulated += amount;
    }
}
void Glassd::check_points(omniglass_raw_report *report, void *data){
    Glassd *state = (Glassd *)data;
    state->single_finger_last_state = state->current_report->points[0];
    state->current_report = report;
}

void Glassd::modeswitch(tracking_mode new_mode){
    current_mode = new_mode;
    emit(touchpad_mode_switched(new_mode));
}
//set up all the lower layers the Glassd core instance will operate.
void Glassd::init(){

    //initialize interface to omniGlass touchpad backend
    struct omniglass *touchpad_handle;
    if (omniglass_init(&touchpad_handle) != OMNIGLASS_RESULT_SUCCESS) {
        fprintf(stderr, "failed to initialize touchpad gesture engine.");
    }
    omniglass_get_touchpad_specifications(touchpad_handle, &(touchpad_specifications));
    omniglass_get_raw_report(touchpad_handle, &(current_report));
    this->handle = touchpad_handle;

    //FIXME: C++ does not allow passing members of class instances as callback.
    //  change omniglass to make it export gesture events into parameters on step().
    omniglass_listen_gesture_edge(touchpad_handle, glassd_handle_edge_swipe, OMNIGLASS_EDGE_TOP, this);

    //
    //VIRTUAL KEYBOARD INIT - prepare a virtual keyboard with a specific set of keys
    //(considering the diversity of keybord shortcuts in desktop applications,
    // we'll probably end up using most of the keycodes in the future.)
    struct libevdev *virtual_keyboard_template = libevdev_new();

    //keep this name consistent to let applications check if it's a real keyboard or glassd.
    libevdev_set_name(virtual_keyboard_template, "glassd virtual keyboard");

    //declare to the uinput subsystem all keycodes that may be triggered on the virtual keyboard.
    int virtual_keyboard_events_definition[] = {
        EV_SYN, SYN_REPORT,
        EV_KEY, KEY_F,
        EV_KEY, KEY_F10,
        EV_KEY, KEY_TAB,
        EV_KEY, KEY_LEFTSHIFT,
        EV_KEY, KEY_ESC,
        EV_KEY, KEY_SPACE,
        EV_KEY, KEY_ENTER,
        DEVICE_DESCRIPTION_END, DEVICE_DESCRIPTION_END
    };
    register_input_codes(virtual_keyboard_template, virtual_keyboard_events_definition);

    //virtual keyboard creation: build a uinput device from the virtual keyboard template
    struct libevdev_uinput *virtual_keyboard_device = NULL;
    if (libevdev_uinput_create_from_device(virtual_keyboard_template, LIBEVDEV_UINPUT_OPEN_MANAGED, &virtual_keyboard_device)){
        fprintf(stderr,"unable to create virtual keyboard device.");
        execution_status = status::krashed;
        return;
    }
    this->virtual_keyboard = virtual_keyboard_device;
    printf("initialized glassd.\n");
    fflush(stdout);
    execution_status = status::ready;
}

    //endless state machine execution loop,
    //adapted into non-blocking step function with class instance for state.
void Glassd::step(){
        int omniglass_status = omniglass_step(handle);

        if(omniglass_status != 0){
            fprintf(stderr, "Guarapicci here. Somehow omniglass returned error on step. I didn't even code it to do that!\n");
            modeswitch(tracking_mode::JINX);
            return;
        }
        // if(single_finger_last_state.is_touching){
        //     double distance = euclidean_distance(current_report->points[0], omniglass_raw_touchpoint({true,45,120}));
        //     printf("you are %f units away from the test ref point.\n",distance);
        //     printf("touching at (%f,%f)\n",single_finger_last_state.x, single_finger_last_state.y);
        // }
        switch(current_mode){
        case tracking_mode::INHIBIT:    //waiting for the user to start switching modes.
            if(
                current_report->points[0].is_touching
                && !(single_finger_last_state.is_touching)
                && euclidean_distance(current_report->points[0],
                                      omniglass_raw_touchpoint({0,0,touchpad_specifications->height}))
                    < DEFAULT_POINT_RADIUS
                )
            {
                printf("waiting on glassing drag.\n");
                modeswitch(tracking_mode::INHIBIT_TO_GLASSING);
            }
            break;
        case tracking_mode::INHIBIT_TO_GLASSING: //user is trying to switch modes.
            if (current_report->points[0].is_touching){
                omniglass_raw_touchpoint current = current_report->points[0];
                omniglass_raw_touchpoint previous = single_finger_last_state;
                omniglass_raw_touchpoint drag
                ({
                    1,
                    current.x - previous.x,
                    current.y - previous.y
                });
                if(drag.x != 0 && drag.y !=0 && drag.x > 0 && drag.y < 0){
                    double tangent = drag.y / drag.x;
                    if (tangent < (-0.65) && tangent > (-1.535)){ //magic numbers for drag direction, from trial and error.
                        drag_to_glassing_accumulator += (sqrt(pow(drag.x, 2) + pow(drag.y, 2)));
                        printf("drag distance is %f\n", drag_to_glassing_accumulator);
                    }
                }
                if(drag_to_glassing_accumulator > DEFAULT_DRAG_TRIGGER_DISTANCE){
                    printf("drag triggered glassing mode.\n");
                    drag_to_glassing_accumulator = 0;
                    edge_slide_accumulated = 0;
                    modeswitch(tracking_mode::GLASSING);
                }
            }
            else{
                printf("drag incomplete, returning to inhibit.\n");
                drag_to_glassing_accumulator = 0;
                modeswitch(tracking_mode::INHIBIT);
            }
            break;
        case tracking_mode::GLASSING:   //glassing mode: different gestures started will result in different keyboard strokes emulated.
            if(edge_slide_accumulated > GLASSD_DEFAULTS_TOUCHPAD_DEADZONE){
                generate_menu(virtual_keyboard);
                edge_slide_accumulated = 0;
                printf("sliding on edge triggered menu mode.\n");
                modeswitch(tracking_mode::MENU);
            }
            if(current_report->points[0].is_touching
                && !(single_finger_last_state.is_touching)
                && euclidean_distance(current_report->points[0], omniglass_raw_touchpoint({0,45,120})) < DEFAULT_POINT_RADIUS)
            {
                printf("touch on corner returned from glassing mode.\n");
                modeswitch(tracking_mode::INHIBIT);
            }
            break;
        case tracking_mode::MENU: // menu mode: dragging horizontally on top edge selects menus and buttons to actuate on the focused application.
            if (edge_slide_accumulated > edge_slide_threshold){
                generate_tab(virtual_keyboard);
                edge_slide_accumulated = 0.0;
            }
            if (edge_slide_accumulated < (-1 * edge_slide_threshold)){
                generate_shift_tab(virtual_keyboard);
                edge_slide_accumulated = 0.0;
            }
            if (!(current_report->points[0].is_touching)){
                generate_enter_tap(virtual_keyboard);
                printf("returning from menu.\n");
                modeswitch(tracking_mode::INHIBIT);
            }
            break;
        case tracking_mode::JINX:   //hold still, don't panic, avoid breaking anything else.
            return;
        }
        single_finger_last_state = current_report->points[0];
        return;
    }



// int main(int argc, char *argv[])
// {
//     QCoreApplication a(argc, argv);

//     return a.exec();
// }

