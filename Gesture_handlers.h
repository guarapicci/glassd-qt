#ifndef GESTURE_HANDLERS_H
#define GESTURE_HANDLERS_H

#include <cstdint>
#include <cstdlib>
#include <cmath>
namespace Gesture_handlers {

enum class Drag_outcome: uint8_t{
    none,
    drag_right,
    drag_left,
    drag_up,
    drag_down
};

class Handler_drag{

public:
    Handler_drag(){}

    //from delta x and y, quantize if a key should be pressed
    // to move some element horizontally or vertically (or nowhere).
    Drag_outcome check_for_drag(float x, float y){
        Drag_outcome decision{Drag_outcome::none};
        accumulated_x += x *abs(x);
        accumulated_y += y * abs(y);

        if(abs(accumulated_x) > threshold){
            if(accumulated_x > 0){
                decision = Drag_outcome::drag_right;
                accumulated_x = 0;
            }
            else{
                decision = Drag_outcome::drag_left;
                accumulated_x = 0;
            }
            if (abs(accumulated_y) < deadzone)
                accumulated_y = 0;
        } else {
            if(abs(accumulated_y) > threshold){
                if(accumulated_y > 0){
                    decision = Drag_outcome::drag_up;
                    accumulated_y = 0;
                }
                else{
                    decision = Drag_outcome::drag_down;
                    accumulated_y = 0;
                }
                if (abs(accumulated_x) < deadzone)
                    accumulated_x = 0;
            }
        }
        return decision;
    }

private:
    //movement tracking state
    float accumulated_x{0};
    float accumulated_y{0};

    //values for filtering and quantization
    const float deadzone{0.3};
    const float threshold{1.2};
};

} // namespace Gesture_handlers

#endif // GESTURE_HANDLERS_H
