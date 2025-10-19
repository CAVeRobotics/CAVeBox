#include <chrono>
#include <csignal>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "cave_talk.h"

#include "game_controller.h"
#include "input_handler.h"
#include "listener_callbacks.h"
#include "logger.h"
#include "serial.h"
#include "talker.h"

#define UNUSED(x) (void)(x)

static const std::string             kLogTag("CAVEBOX");
static bool                          stop_signal = false;
static std::shared_ptr<serial::Port> serial_port;

void SignalHandler(const int signal)
{
    (void)(signal);

    stop_signal = true;
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Set up serial port
    if (argc < 3)
    {
        LOGGER_LOG_ERROR(std::cerr, kLogTag, "Invalid number of arguments");
        throw std::runtime_error("Invalid number of arguments");
    }
    serial_port = std::make_shared<serial::Port>(argv[1]);
    serial_port->Open(std::stoi(argv[2]));

    // Set up CAVeTalk Talker and Listener
    std::shared_ptr<cavebox::Talker> talker = std::make_shared<cavebox::Talker>([](const void *const data, const std::size_t size)
    {
        (void)serial_port->Write(static_cast<const std::uint8_t *>(data), size);

        return CAVE_TALK_ERROR_NONE;
    });
    std::shared_ptr<cavebox::ListenerCallbacks> listener_callbacks = std::make_shared<cavebox::ListenerCallbacks>(talker);
    cave_talk::Listener                         listener([](void *const data, const std::size_t size, std::size_t *const bytes_received)
    {
        *bytes_received = serial_port->Read(static_cast<std::uint8_t *>(data), size);

        return CAVE_TALK_ERROR_NONE;
    }, listener_callbacks);

    // Set up game controller and input handler
    game_controller::Initialize();
    std::shared_ptr<cavebox::InputHandler> input_handler = std::make_shared<cavebox::InputHandler>(talker);
    game_controller::ControllerHandler     controller_handler(input_handler);

    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    while (controller_handler.IsRunning() && !stop_signal)
    {
        CaveTalk_Error_t error = listener.Listen();

        if (CAVE_TALK_ERROR_NONE != error)
        {
            LOGGER_LOG_ERROR(std::cerr, kLogTag, "CAVeTalk Listen error: {}", (int)error);
        }

        error = talker->Run();

        if (CAVE_TALK_ERROR_NONE != error)
        {
            LOGGER_LOG_ERROR(std::cerr, kLogTag, "CAVeTalk Talk error: {}", (int)error);
        }

        // TODO remove
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last) >= std::chrono::milliseconds(50))
        {
            if (listener_callbacks->IsConnected())
            {
                talker->SpeakMovement(input_handler->GetSpeed(), input_handler->GetTurnRate());
            }

            last = now;
        }
    }

    controller_handler.Stop();
    game_controller::Deinitialize();

    return 0;
}