#include <csignal>
#include <memory>

#include <game_controller.h>

static bool stop_signal = false;

class InputHandler : public game_controller::InputHandler
{
    public:
        void HandleButtonDown(const game_controller::Controller *const controller, const game_controller::Event &event)
        {
            (void)(controller);
            (void)(event);
        }

        void HandleAxisMotion(const game_controller::Controller *const controller, const game_controller::Event &event)
        {
            (void)(controller);
            (void)(event);
        }
};

void SignalHandler(const int signal)
{
    (void)(signal);

    stop_signal = true;
}

int main(void)
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    game_controller::Initialize();
    game_controller::ControllerHandler controller_handler(std::make_shared<InputHandler>());

    while (controller_handler.IsRunning() && !stop_signal)
    {
    }

    controller_handler.Stop();
    game_controller::Deinitialize();

    return 0;
}