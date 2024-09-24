#include "Glassd.h"

extern "C" {
#include <asm-generic/errno-base.h>
#include <linux/input-event-codes.h>

#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <linux/ioctl.h>
#include <linux/input.h>

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
void Glassd::change_gesture_mapper(QString name){
    current_gesture_mapper->reset();
    std::map<QString, Gesture_mapper *>::iterator search_results;
    QString app_class = application_classes[current_application_canonical_id];
    search_results = mappers.find(app_class);
    if(search_results != mappers.end()){
        if ( !(app_class.isEmpty()) )
            qDebug() << " changing to profile " << app_class << Qt::endl;
        current_gesture_mapper = search_results->second;
    }
    else{
        current_gesture_mapper = mappers["fallback"];
    }
}

//set up all the lower layers the Glassd core instance will operate.
void Glassd::init(){

    //initialize interface to omniGlass touchpad backend
    struct omniglass *touchpad_handle;
    if (omniglass_init(&touchpad_handle) != OMNIGLASS_RESULT_SUCCESS) {
        fprintf(stderr, "failed to initialize touchpad gesture engine.");
    }

    // ioctl(raw_touchpad_file_descriptor,EVIOCGRAB, 1);


    omniglass_get_touchpad_specifications(touchpad_handle, &(touchpad_specifications));
    omniglass_get_raw_report(touchpad_handle, &(current_report));
    this->handle = touchpad_handle;

    //create touch gesture mapper default profiles
    generate_default_mappers();
    current_gesture_mapper = mappers[application_classes[""]];

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
    //  ideally a full keyboard should be available, but i don't need all the keys right now.
    int virtual_keyboard_events_definition[] = {
        EV_SYN, SYN_REPORT,
        EV_KEY, KEY_F,
        EV_KEY, KEY_F6,
        EV_KEY, KEY_F10,
        EV_KEY, KEY_F11,
        EV_KEY, KEY_TAB,
        EV_KEY, KEY_LEFTSHIFT,
        EV_KEY, KEY_ESC,
        EV_KEY, KEY_SPACE,
        EV_KEY, KEY_ENTER,
        EV_KEY, KEY_LEFT,
        EV_KEY, KEY_RIGHT,
        EV_KEY, KEY_UP,
        EV_KEY, KEY_DOWN,
        EV_KEY, KEY_7,
        EV_KEY, KEY_8,
        EV_KEY, KEY_9,
        EV_KEY, KEY_0,
        EV_KEY, KEY_LEFTCTRL,
        EV_KEY, KEY_PAGEDOWN,
        EV_KEY, KEY_PAGEUP,
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
    compute_gesture_parameters();

    //
    //COMPUTE GESTURE PARAMETERS
    //

    // if(single_finger_last_state.is_touching){
    //     double distance = euclidean_distance(current_report->points[0], omniglass_raw_touchpoint({true,45,120}));
    //     printf("you are %f units away from the test ref point.\n",distance);
    //     printf("touching at (%f,%f)\n",single_finger_last_state.x, single_finger_last_state.y);
    // }
    switch (current_mode) {
    case tracking_mode::INHIBIT: //waiting for the user to start switching modes.

        //ungrab, just in case the touchpad was being grabbed before.
        omniglass_ungrab_touchpad(handle);

        if (current_report->points[0].is_touching && !(single_finger_last_state.is_touching)
            && euclidean_distance(current_report->points[0],
                                  omniglass_raw_touchpoint({0, 0, touchpad_specifications->height}))
                   < DEFAULT_POINT_RADIUS) {
            printf("waiting on glassing drag.\n");
            modeswitch(tracking_mode::INHIBIT_TO_GLASSING);
        }
        break;
    case tracking_mode::INHIBIT_TO_GLASSING: //user is trying to switch modes.


        //grab the touchpad device to avoid cursor moving when glassing mode needs it off
        omniglass_grab_touchpad(handle);

        if (current_report->points[0].is_touching) {
            omniglass_raw_touchpoint current = current_report->points[0];
            omniglass_raw_touchpoint previous = single_finger_last_state;
            omniglass_raw_touchpoint drag({1, current.x - previous.x, current.y - previous.y});
            if (drag.x != 0 && drag.y != 0 && drag.x > 0 && drag.y < 0) {
                double tangent = drag.y / drag.x;
                if (tangent < (-0.65)
                    && tangent
                           > (-1.535)) { //magic numbers for drag direction, from trial and error.
                    drag_to_glassing_accumulator += (sqrt(pow(drag.x, 2) + pow(drag.y, 2)));
                    printf("drag distance is %f\n", drag_to_glassing_accumulator);
                }
            }
            if (drag_to_glassing_accumulator > DEFAULT_DRAG_TRIGGER_DISTANCE) {
                printf("drag triggered glassing mode.\n");
                drag_to_glassing_accumulator = 0;
                edge_slide_accumulated = 0;
                modeswitch(tracking_mode::GLASSING);
            }
        } else {
            printf("drag incomplete, returning to inhibit.\n");
            drag_to_glassing_accumulator = 0;
            modeswitch(tracking_mode::INHIBIT);
        }
        break;
    //glassing mode: different gestures started will result in different keyboard strokes emulated.
    case tracking_mode::GLASSING:
    {
        //leave glassing if touched on special corner
        bool just_touched = (
            current_report->points[0].is_touching
            && !(previous_report.points[0].is_touching)
            );
        omniglass_raw_touchpoint pivot {
            0,
            0,
            touchpad_specifications->height
        };
        float distance_to_pivot = euclidean_distance(current_report->points[0], pivot);
        bool close_enough = (distance_to_pivot < DEFAULT_POINT_RADIUS);
        if ( just_touched && close_enough )
        {
            printf("touch on corner returned from glassing mode.\n");
            modeswitch(tracking_mode::INHIBIT);
        }

        //if not leaving glassing mode, let per-application gesture mappers translate motion to keyboard chords.
        //figure out what immediate movements the user is doing and feed them to the gesture mapper.
        Gesture_mapper::gesture_action next_action = compute_gesture_next_action();
        current_gesture_mapper->step(next_action);

        switch(current_gesture_mapper->current_assignment.reaction) {
        case Gesture_mapper::gesture_reaction::cursor_2D :
        {
            fprintf(stderr, "as of state %d "
                    "i'm haulin' yo' mouse.\n", current_gesture_mapper->current_assignment.state);
            float drag_x = gesture_data.drags[0][0];
            float drag_y = gesture_data.drags[0][1];
            switch (handler_drag.check_for_drag(drag_x,drag_y))
            {
            case Gesture_handlers::Drag_outcome::drag_left:
                simulate_keycodes({KEY_LEFT});
                break;
            case Gesture_handlers::Drag_outcome::drag_right:
                simulate_keycodes({KEY_RIGHT});
                break;
            case Gesture_handlers::Drag_outcome::drag_up:
                simulate_keycodes({KEY_UP});
                break;
            case Gesture_handlers::Drag_outcome::drag_down:
                simulate_keycodes({KEY_DOWN});
                break;
            }
        }
            break;
        case Gesture_mapper::gesture_reaction::keycode :
            simulate_keycodes(current_gesture_mapper->current_assignment.codes);
            // fprintf(stderr, "i'm supposed to press keys here, but it's not implemented.\n");
            break;
        case Gesture_mapper::gesture_reaction::cursor_horiz_1D:
        {
            float drag_x = gesture_data.drags[0][0];
            float drag_y = gesture_data.drags[0][1];

            std::list<__u16> remapped_keys_left = {KEY_LEFT};
            std::list<__u16> remapped_keys_right = {KEY_RIGHT};

            std::list<__u16> assigned_codes = current_gesture_mapper->current_assignment.codes;
            if(assigned_codes.size() >= 4) {
                remapped_keys_left = {};
                remapped_keys_right = {};
                std::list<__u16>::iterator next_code = assigned_codes.begin();
                std::list<__u16> *remap_target = &remapped_keys_left;
                while(*next_code != KEY_RESERVED){
                    remap_target->push_back(*next_code);
                    next_code++;
                    if(next_code == assigned_codes.end()){
                        goto gesture_reaction_horiz_invalid_codes;
                    }
                }
                next_code++;
                remap_target = &remapped_keys_right;
                while(*next_code != KEY_RESERVED){
                    remap_target->push_back(*next_code);
                    next_code++;
                    if(next_code == assigned_codes.end()){
                        goto gesture_reaction_horiz_invalid_codes;
                    }
                }
            }
            switch (handler_drag.check_for_drag(drag_x,drag_y)) {
            case Gesture_handlers::Drag_outcome::drag_left:
                simulate_keycodes(remapped_keys_left);
                qDebug() << "remapped left (" << remapped_keys_left << ")" << Qt::endl;
                break;
            case Gesture_handlers::Drag_outcome::drag_right:
                qDebug() << "remapped right (" << remapped_keys_right << ")" << Qt::endl;
                simulate_keycodes(remapped_keys_right);
                break;
            }
            break;
            gesture_reaction_horiz_invalid_codes:
                qDebug() << "invalid keystroke combo on gesture mapper definition." << Qt::endl;
                current_gesture_mapper->reset();
                break;
        }
        case Gesture_mapper::gesture_reaction::cursor_vert_1D:
        {
            float drag_x = gesture_data.drags[0][0];
            float drag_y = gesture_data.drags[0][1];

            std::list<__u16> remapped_keys_down = {KEY_DOWN};
            std::list<__u16> remapped_keys_up = {KEY_UP};

            std::list<__u16> assigned_codes = current_gesture_mapper->current_assignment.codes;
            if(assigned_codes.size() >= 4) {
                remapped_keys_down = {};
                remapped_keys_up = {};
                std::list<__u16>::iterator next_code = assigned_codes.begin();
                std::list<__u16> *remap_target = &remapped_keys_down;
                while(*next_code != KEY_RESERVED){
                    remap_target->push_back(*next_code);
                    next_code++;
                    if(next_code == assigned_codes.end()){
                        goto gesture_reaction_vert_invalid_codes;
                    }
                }
                next_code++;
                remap_target = &remapped_keys_up;
                while(*next_code != KEY_RESERVED){
                    remap_target->push_back(*next_code);
                    next_code++;
                    if(next_code == assigned_codes.end()){
                        goto gesture_reaction_vert_invalid_codes;
                    }
                }
            }
            switch (handler_drag.check_for_drag(drag_x,drag_y)) {
            case Gesture_handlers::Drag_outcome::drag_down:
                simulate_keycodes(remapped_keys_down);
                qDebug() << "remapped down (" << remapped_keys_down << ")" << Qt::endl;
                break;
            case Gesture_handlers::Drag_outcome::drag_up:
                qDebug() << "remapped up (" << remapped_keys_up << ")" << Qt::endl;
                simulate_keycodes(remapped_keys_up);
                break;
            }
            break;
        gesture_reaction_vert_invalid_codes:
            qDebug() << "invalid keystroke combo on gesture mapper definition." << Qt::endl;
            current_gesture_mapper->reset();
            break;
        }
        case Gesture_mapper::gesture_reaction::none:
            break;
        }
        break;
    }
    case tracking_mode::JINX: //hold still, don't panic, avoid breaking anything else.
        return;
    }
        single_finger_last_state = current_report->points[0];
        return;
    }

void Glassd::compute_gesture_parameters()
{

    for (int i = 0; i < current_report->points_max; i++) {
        //copy values of touchpoints from current points to previous points
        //(not sure if i'm copying values or just their references...)
        previous_report.points[i] = current_report->points[i];
    }



    int omniglass_status = omniglass_step(handle);
    if (omniglass_status != 0) {
        fprintf(stderr,
                "Guarapicci here. Somehow omniglass returned error on step. I didn't even code "
                "it to do that!\n");
        modeswitch(tracking_mode::JINX);
        return;
    }

    int count_then = 0, count_now = 0;
    for (int i = 0; i < current_report->points_max; i++) {
        //copy values of touchpoints from current points to previous points
        //(not sure if i'm copying values or just their references...)
        if (previous_report.points[i].is_touching) { count_then++; }
        if (current_report->points[i].is_touching) { count_now++; }
    }
    if(count_then != count_now)
        gesture_data.touch_count_transition = count_now;
    else
        gesture_data.touch_count_transition = 255;

    //compute 2D movement deltas of touchpoints
    for (int i = 0; i < current_report->points_max; i++) {
        if (!(current_report->points[i].is_touching)) {
            gesture_data.drags[i][0] = 0;
            gesture_data.drags[i][1] = 0;
        } else {
            float x1 = previous_report.points[i].x;
            float y1 = previous_report.points[i].y;
            float x2 = current_report->points[i].x;
            float y2 = current_report->points[i].y;
            gesture_data.drags[i][0] = (x2 - x1);
            gesture_data.drags[i][1] = (y2 - y1);
        }
    }

    //figure out for single-touch if the main contact is on one of the borders
    {
        gesture_data.is_on_bottom_edge = false;
        gesture_data.is_on_top_edge = false;
        gesture_data.is_on_left_edge = false;
        gesture_data.is_on_right_edge = false;

        omniglass_raw_touchpoint main_point = current_report->points[0];
        float main_x = (float)main_point.x;
        float main_y = (float)main_point.y;
        if(main_point.is_touching){
            float margin = DEFAULT_POINT_RADIUS;
            float height = touchpad_specifications->height;
            float width = touchpad_specifications->width;

            float x_left = 0.0 + margin;
            float x_right = width - margin;
            float y_bottom = 0.0 + margin;
            float y_top = height - margin;

            if (main_x < x_left
                && main_y > y_bottom
                && main_y < y_top)
            {
                gesture_data.is_on_left_edge = true;
            }
            if (main_x > x_right
                && main_y > y_bottom
                && main_y < y_top)
            {
                gesture_data.is_on_right_edge = true;
            }
            if (main_y < y_bottom
                && main_x > x_left
                && main_x < x_right)
            {
                gesture_data.is_on_bottom_edge = true;
            }
            if ((main_y > y_top)
                && (main_x > x_left)
                && (main_x < x_right))
            {
                gesture_data.is_on_top_edge = true;
            }

        }

    }
}

Gesture_mapper::gesture_action Glassd::compute_gesture_next_action()
{
    int presence_change = gesture_data.touch_count_transition;
    if (presence_change == 0)
        return Gesture_mapper::gesture_action::released;
    else {
        if (presence_change>0){
            if(presence_change == 1)
            {
                if(gesture_data.is_on_top_edge)
                    return Gesture_mapper::gesture_action::entered_top_edge;
                if(gesture_data.is_on_bottom_edge)
                    return Gesture_mapper::gesture_action::entered_bottom_edge;
                if(gesture_data.is_on_right_edge)
                    return Gesture_mapper::gesture_action::entered_right_edge;
                if(gesture_data.is_on_left_edge)
                    return Gesture_mapper::gesture_action::entered_left_edge;
            }
            if(presence_change <=23)
                return Gesture_mapper::gesture_action::touch_count;
        }
    }




    if ((abs(gesture_data.drags[0][0]) > 0) || (abs(gesture_data.drags[0][1]) > 0))
        return Gesture_mapper::gesture_action::drag_general;
    else
        return Gesture_mapper::gesture_action::none;
}

//This function does much less than its sheer size in Lines-Of-Code suggests.
//  it just hardwires a bunch of per-application shortcut mappings.
void Glassd::generate_default_mappers()
{
    //all classes and aliases that will be matched to their respective gesture mappers
    std::map<QString, QString> new_app_classes{{"mpv", "media player"},
                                               {"org.kde.haruna", "media player"},
                                               {"org.kde.dolphin", "file browser"},
                                               {"firefox", "web browser"},
                                               {"", "fallback"}};
    application_classes = new_app_classes;


    std::map<QString, Gesture_mapper *> new_mappers;
    //when all else fails, use the fallback.
    Gesture_mapper *fallback_mapper = new Gesture_mapper();
    fallback_mapper->gesture_map
        = std::map<Gesture_mapper::gesture_entry, Gesture_mapper::gesture_assignment>{
            {{0, Gesture_mapper::gesture_action::drag_general}, {0, Gesture_mapper::gesture_reaction::cursor_2D, {}}},
            {{0, Gesture_mapper::gesture_action::entered_top_edge}, {1, Gesture_mapper::gesture_reaction::keycode, {KEY_F10}}},
            {{1, Gesture_mapper::gesture_action::released}, {2, Gesture_mapper::gesture_reaction::none, {}}},
            {{2, Gesture_mapper::gesture_action::drag_general}, {2, Gesture_mapper::gesture_reaction::cursor_2D, {}}},
            {{2, Gesture_mapper::gesture_action::released}, {0, Gesture_mapper::gesture_reaction::keycode, {KEY_ENTER}}},
            // {{0, Gesture_mapper::entered_top_edge}, {1, Gesture_mapper::none, {}}},
        };
    new_mappers["fallback"] = fallback_mapper;
    new_mappers[""] = fallback_mapper;

    Gesture_mapper *media_player_profile = new Gesture_mapper();
    media_player_profile->gesture_map
        = std::map<Gesture_mapper::gesture_entry, Gesture_mapper::gesture_assignment>{
            {{0, Gesture_mapper::gesture_action::entered_bottom_edge}, {1, Gesture_mapper::gesture_reaction::none, {}}},
            {{0, Gesture_mapper::gesture_action::entered_right_edge}, {2, Gesture_mapper::gesture_reaction::none, {}}},
            {{1, Gesture_mapper::gesture_action::drag_general}, {1, Gesture_mapper::gesture_reaction::cursor_horiz_1D, {}}},
            {{1, Gesture_mapper::gesture_action::released}, {0, Gesture_mapper::gesture_reaction::none, {}}},
            {{2, Gesture_mapper::gesture_action::drag_general}, {2, Gesture_mapper::gesture_reaction::cursor_vert_1D, {KEY_0, KEY_RESERVED, KEY_9, KEY_RESERVED} }}, //using KEY_RESERVED as a separator
            {{2, Gesture_mapper::gesture_action::released}, {0, Gesture_mapper::gesture_reaction::keycode, {KEY_ENTER} }},
            };
    new_mappers["media player"] = media_player_profile;


    Gesture_mapper *web_browser_profile = new Gesture_mapper();
    web_browser_profile->gesture_map
        = std::map<Gesture_mapper::gesture_entry, Gesture_mapper::gesture_assignment>{
            {{0, Gesture_mapper::gesture_action::entered_bottom_edge}, {1, Gesture_mapper::gesture_reaction::none, {}}},
            {{0, Gesture_mapper::gesture_action::entered_right_edge}, {2, Gesture_mapper::gesture_reaction::none, {}}},
            {{1, Gesture_mapper::gesture_action::drag_general}, {1, Gesture_mapper::gesture_reaction::cursor_horiz_1D, {KEY_LEFTCTRL,KEY_PAGEUP,KEY_RESERVED, KEY_LEFTCTRL,KEY_PAGEDOWN,KEY_RESERVED}}},
            {{1, Gesture_mapper::gesture_action::released}, {0, Gesture_mapper::gesture_reaction::none, {}}},
            {{2, Gesture_mapper::gesture_action::drag_general}, {2, Gesture_mapper::gesture_reaction::cursor_vert_1D, {KEY_9, KEY_RESERVED, KEY_0, KEY_RESERVED} }},
            {{2, Gesture_mapper::gesture_action::released}, {0, Gesture_mapper::gesture_reaction::keycode, {KEY_ENTER} }},
            };
    new_mappers["web browser"] = web_browser_profile;

    mappers = new_mappers;


    // new_mappers.insert("file browser", std::map<>)
}

//default mode of simulating key-chords is to tap them infinetely fast.
//  don't hold them, else stuck keys will happen simply because
//  someone's profile forgot to release them.
void Glassd::simulate_keycodes(std::list<__u16> keycodes){
    for (__u16 code : keycodes){
    libevdev_uinput_write_event(virtual_keyboard, EV_KEY, code, 1);
    }
    libevdev_uinput_write_event(virtual_keyboard, EV_SYN, SYN_REPORT, 0);
    for (__u16 code : keycodes){
        libevdev_uinput_write_event(virtual_keyboard, EV_KEY, code, 0);
    }
    libevdev_uinput_write_event(virtual_keyboard, EV_SYN, SYN_REPORT, 0);
}
