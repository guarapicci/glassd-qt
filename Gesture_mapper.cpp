#include "Gesture_mapper.h"

void Gesture_mapper::step(gesture_action stimulus)
{
    std::map<gesture_entry,gesture_assignment>::iterator transition = gesture_map.find(gesture_entry{current_assignment.state, stimulus});
    if (transition != gesture_map.end()){
        current_assignment = transition->second;
    } else{
        current_assignment.reaction = none;
    }
};
