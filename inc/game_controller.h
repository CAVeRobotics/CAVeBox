#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif // SDL_MAIN_HANDLED
#include <SDL.h>

namespace game_controller
{

using Controller = SDL_GameController;
using Event      = SDL_Event;

enum class EventCode
{
    AXIS_MOTION = 0x650,     // Game controller axis motion
    BUTTON_DOWN,             // Game controller button pressed
    BUTTON_UP,               // Game controller button released
    CONTROLLER_ADDED,        // A new Game controller has been inserted into the system
    CONTROLLER_REMOVED,      // An opened Game controller has been removed
    CONTROLLER_REMAPPED,     // The controller mapping was updated
    TOUCHPAD_DOWN,           // Game controller touchpad was touched
    TOUCHPAD_MOTION,         // Game controller touchpad finger was moved
    TOUCHPAD_UP,             // Game controller touchpad finger was lifted
    SENSOR_UPDATE,           // Game controller sensor was updated
};

enum class JoystickAxis
{
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y
};

enum class Button
{
    BUTTON_INVALID = -1,
    BUTTON_A,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_BACK,
    BUTTON_GUIDE,
    BUTTON_START,
    BUTTON_LEFTSTICK,
    BUTTON_RIGHTSTICK,
    BUTTON_LEFTSHOULDER,
    BUTTON_RIGHTSHOULDER,
    BUTTON_DPAD_UP,
    BUTTON_DPAD_DOWN,
    BUTTON_DPAD_LEFT,
    BUTTON_DPAD_RIGHT
};

class InputHandler
{
    public:
        virtual void HandleButtonDown(const game_controller::Controller *const controller, game_controller::Event &event) = 0;
        virtual void HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event) = 0;
};

class ControllerHandler
{
    public:
        ControllerHandler(std::shared_ptr<InputHandler> input_handler);
        ControllerHandler(ControllerHandler &event_handler)                  = delete;
        ControllerHandler(ControllerHandler &&event_handler)                 = delete;
        ControllerHandler &operator=(const ControllerHandler &event_handler) = delete;
        ControllerHandler &operator=(ControllerHandler &&event_handler)      = delete;
        ~ControllerHandler();
        void Stop(void);
        bool IsRunning(void);

    private:
        void HandleController(void);

        std::shared_ptr<InputHandler> input_handler_;
        std::atomic_bool running_;
        std::thread handle_controller_thread_;
};

bool Initialize(void);
std::vector<int> GetControllerNumbers(void);
Controller *OpenController(const int controller_number);
Controller *GetController(void);

[[maybe_unused]] inline void Deinitialize(void)
{
    SDL_Quit();
}

[[maybe_unused]] inline void CloseController(Controller *const controller)
{
    SDL_GameControllerClose(controller);
}

[[maybe_unused]] inline SDL_JoystickID GetControllerInstanceId(Controller *const controller)
{
    return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

[[maybe_unused]] inline int PollEvent(Event *const event)
{
    return SDL_PollEvent(event);
}

[[maybe_unused]] inline bool IsControllerEvent(Controller *const controller, const Event event)
{
    return (nullptr != controller) && (game_controller::GetControllerInstanceId(controller) == event.cdevice.which);
}

[[maybe_unused]] inline bool IsButtonHeld(Controller *const controller, const Button button)
{
    return SDL_PRESSED == SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button));
}

} // namespace game_controller

#endif // GAME_CONTROLLER_H