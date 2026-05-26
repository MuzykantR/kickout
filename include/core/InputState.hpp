#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP

struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool sprint = false;
    bool jumpHeld = false;
    /// Событие «нажали прыжок» в этом кадре
    bool jumpPressed = false;
};

#endif
