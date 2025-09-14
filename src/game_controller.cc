#include "game_controller.h"

#include <iostream>

#include "logger.h"

namespace game_controller
{

    static const std::string kLogTag = "GAME CONTROLLER";

    ControllerHandler::ControllerHandler(std::shared_ptr<InputHandler> input_handler) : input_handler_(input_handler), running_(true), handle_controller_thread_(&ControllerHandler::HandleController, this)
    {
    }

    ControllerHandler::~ControllerHandler()
    {
        Stop();
    }

    void ControllerHandler::Stop(void)
    {
        if (running_.load())
        {
            running_.store(false);

            if (handle_controller_thread_.joinable())
            {
                handle_controller_thread_.join();
            }
        }
    }

    bool ControllerHandler::IsRunning(void)
    {
        return running_.load();
    }

    void ControllerHandler::HandleController(void)
    {
        Controller *controller = GetController(); // TODO add option to select which controller to use if multiple are connected
        Event event;

        while (running_.load())
        {
            while (0 != PollEvent(&event))
            {
                switch (static_cast<EventCode>(event.type))
                {
                case EventCode::CONTROLLER_ADDED:
                    if (nullptr == controller)
                    {
                        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Controller added");
                        controller = GetController();
                    }
                    break;
                case EventCode::CONTROLLER_REMOVED:
                    if (IsControllerEvent(controller, event))
                    {
                        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Controller removed");
                        CloseController(controller);
                        controller = GetController();
                    }
                    break;
                case EventCode::BUTTON_DOWN:
                    if (IsControllerEvent(controller, event))
                    {
                        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Button down");
                        input_handler_->HandleButtonDown(controller, event);
                    }
                    break;
                case EventCode::AXIS_MOTION:
                    if (IsControllerEvent(controller, event))
                    {
                        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Axis motion");
                        input_handler_->HandleAxisMotion(controller, event);
                    }
                    break;
                case EventCode::BUTTON_UP:
                case EventCode::CONTROLLER_REMAPPED:
                case EventCode::TOUCHPAD_DOWN:
                case EventCode::TOUCHPAD_MOTION:
                case EventCode::TOUCHPAD_UP:
                case EventCode::SENSOR_UPDATE:
                default:
                    break;
                }
            }

            // TODO sleep thread?
        }

        if (nullptr != controller)
        {
            CloseController(controller);
        }
    }

    bool Initialize(void)
    {
        bool initialized = false;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        {
            LOGGER_LOG_ERROR(std::cout, kLogTag, "SDL error code {}", SDL_GetError());
            LOGGER_LOG_ERROR(std::cout, kLogTag, "Failed to initialize");
        }
        else
        {
            initialized = true;
        }

        return initialized;
    }

    std::vector<int> GetControllerNumbers(void)
    {
        std::vector<int> controller_numbers;
        int controller_count = SDL_NumJoysticks();

        LOGGER_UNUSED(controller_numbers);

        if (controller_count < 0)
        {
            LOGGER_LOG_ERROR(std::cout, kLogTag, "SDL error code {}", SDL_GetError());
            LOGGER_LOG_ERROR(std::cout, kLogTag, "Failed to get controller numbers");
        }
        else
        {
            controller_numbers.reserve(static_cast<size_t>(controller_count));

            for (int i = 0; i < controller_count; i++)
            {
                if (SDL_IsGameController(i))
                {
                    controller_numbers.emplace_back(i);
                }
            }
        }

        return controller_numbers;
    }

    Controller *OpenController(const int controller_number)
    {
        Controller *controller = nullptr;

        if (controller_number < SDL_NumJoysticks())
        {
            controller = SDL_GameControllerOpen(controller_number);

            if (nullptr == controller)
            {
                LOGGER_LOG_ERROR(std::cout, kLogTag, "SDL error code {}", SDL_GetError());
                LOGGER_LOG_ERROR(std::cout, kLogTag, "Failed to open controller");
            }
        }

        return controller;
    }

    Controller *GetController(void)
    {
        Controller *controller = nullptr;
        std::vector<int> controller_numbers = GetControllerNumbers();

        if (!controller_numbers.empty())
        {
            controller = OpenController(controller_numbers[0]);
        }

        return controller;
    }

} // namespace controller