#include <chrono>
#include <csignal>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "cave_talk.h"
#define ASIO_STANDALONE
#include "server_ws.hpp"

#include "game_controller.h"
#include "input_handler.h"
#include "listener_callbacks.h"
#include "logger.h"
#include "serial.h"
#include "talker.h"

#define UNUSED(x) (void)(x)

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

static const std::string             kLogTag("CAVEBOX");
static const std::uint16_t           kWsPort = 8081;
static const std::string             kCameraEndpoint("^/camera/?$");
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

    // Set up websocket server
    WsServer camera_server;
    camera_server.config.port = kWsPort;
    auto &camera_endpoint = camera_server.endpoint[kCameraEndpoint]; // TODO
    camera_endpoint.on_message = [input_handler](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> in_message)
    {
        UNUSED(connection);
        UNUSED(in_message);

        // TODO unpack message and call input handler
        input_handler->HandleCameraCommand(0.0, 0.0);

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Message received");
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Camera message received");
    };
    camera_endpoint.on_open = [](std::shared_ptr<WsServer::Connection> connection)
    {
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Camera server opened connection: {:#x}", reinterpret_cast<std::uintptr_t>(connection.get()));
    };
    camera_endpoint.on_close = [](std::shared_ptr<WsServer::Connection> connection, const int status, const std::string &)
    {
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Camera server closed connection: {:#x}, status code: {}", reinterpret_cast<std::uintptr_t>(connection.get()), status);
    };
    camera_endpoint.on_handshake = [](std::shared_ptr<WsServer::Connection>, SimpleWeb::CaseInsensitiveMultimap &)
    {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };
    camera_endpoint.on_error = [](std::shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &error_code)
    {
        LOGGER_LOG_ERROR(std::cerr, kLogTag, "Camera server error: {}, connection: {:#x}", error_code.message(), reinterpret_cast<std::uintptr_t>(connection.get()));
    };
    std::thread camera_server_thread([&camera_server](){
        camera_server.start([](const std::uint16_t port) {
            LOGGER_LOG_DEBUG(std::cout, kLogTag, "Camera server listening on port: {}", port);
        });

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Camera server stopped");
    });

    std::chrono::steady_clock::time_point last           = std::chrono::steady_clock::now();
    double                                last_speed     = input_handler->GetSpeed();
    double                                last_turn_rate = input_handler->GetTurnRate();
    double                                last_pan       = input_handler->GetPan();
    double                                last_tilt      = input_handler->GetTilt();
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

        std::chrono::steady_clock::time_point now       = std::chrono::steady_clock::now();
        const double                          speed     = input_handler->GetSpeed();
        const double                          turn_rate = input_handler->GetTurnRate();
        const double                          pan       = input_handler->GetPan();
        const double                          tilt      = input_handler->GetTilt();
        if (listener_callbacks->IsConnected() && (std::chrono::duration_cast<std::chrono::milliseconds>(now - last) >= std::chrono::milliseconds(50)))
        {
            if ((speed != last_speed) || (turn_rate != last_turn_rate))
            {
                talker->SpeakMovement(speed, turn_rate);

                last_speed     = speed;
                last_turn_rate = turn_rate;
            }

            if ((pan != last_pan) || (tilt != last_tilt))
            {
                talker->SpeakCameraMovement(pan, tilt);

                last_pan  = pan;
                last_tilt = tilt;
            }

            last = now;
        }
    }

    camera_server.stop();
    camera_server_thread.join();
    controller_handler.Stop();
    game_controller::Deinitialize();

    return 0;
}