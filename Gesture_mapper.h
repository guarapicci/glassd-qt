#ifndef GESTURE_MAPPER_H
#define GESTURE_MAPPER_H

#include <QObject>
#include <asm-generic/int-ll64.h>
#include <linux/input-event-codes.h>
#include <list>
#include <map>

class Gesture_mapper : public QObject
{
    Q_OBJECT
public:
    Gesture_mapper(QObject *parent = nullptr)
        : QObject(parent){
        current_assignment = {0,gesture_reaction::none, {}};
        };

    static const uint8_t ASSIGNMENT_STATE_INVALID = 255;

    enum class mapper_status : uint8_t { valid_state, krash };
    //ALL the default gestures expected to be mapped 1:1 to some keyboard/mouse action.
    enum class gesture_action : uint8_t {
        drag_general,
        entered_bottom_edge,
        entered_top_edge,
        entered_left_edge,
        entered_right_edge,
        left_edges,
        released,
        touch_count,
        spiral_clockwise,
        spiral_counterclockwise,
        none    //this value tags an invalid element. There are no actual gesture assignments or entries with this action.
    };
    enum gesture_reaction {
        keycode,
        cursor_vert_1D,
        cursor_horiz_1D,
        cursor_2D,
        none };

    struct gesture_entry
    {
        uint8_t state;
        gesture_action action = gesture_action::none; //default empty state is "none"
        bool operator<(const gesture_entry &other) const
        {
            uint16_t self_hash = 0;
            self_hash |= ((uint8_t)(state) << 8);
            self_hash |= (uint8_t)(action);
            uint16_t other_hash = 0;
            other_hash |= ((uint8_t)(other.state) << 8);
            other_hash |= (uint8_t)(other.action);
            return (self_hash < other_hash);
        }
    };

    struct gesture_assignment
    {
        uint8_t state = ASSIGNMENT_STATE_INVALID;
        gesture_reaction reaction = none;
        std::list<__u16> codes;
    };

    gesture_assignment current_assignment;

    //linux input event keycodes. check "linux/input-event-codes.h" for exact numbers
    std::map<gesture_entry, gesture_assignment> gesture_map;

    void reset(){
        current_assignment = {0,gesture_reaction::none,{}};
    }
public slots:
    //int parse_config(std::iostream stream);

    //run an iteration of internal state machines.
    void step(gesture_action stimulus);

private:
    // Glassd *gestureEngine;
    int current_trigger;

    std::multimap<int, int, int> boundaries_to_states;

signals:
};

#endif // GESTURE_MAPPER_H
