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
static const std::string             kArmEndpoint("^/arm/?$");
static const std::string             kCameraEndpoint("^/camera/?$");
static const std::string             kDriveEndpoint("^/drive/?$");
static const std::string             kMoveEndpoint("^/move/?$");
static bool                          stop_signal = false;
static std::shared_ptr<serial::Port> serial_port;

void SignalHandler(const int signal)
{
    (void)(signal);

    stop_signal = true;
}

void InitializeEndpoint(SimpleWeb::SocketServerBase<SimpleWeb::WS>::Endpoint &endpoint, const std::string &connection_name, std::function<void(std::shared_ptr<WsServer::Connection>, std::shared_ptr<WsServer::InMessage>)> on_message)
{
    endpoint.on_message = on_message;
    endpoint.on_open    = [connection_name](std::shared_ptr<WsServer::Connection> connection)
    {
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Command server opened {} connection: {:#x}", connection_name, reinterpret_cast<std::uintptr_t>(connection.get()));
    };
    endpoint.on_close = [connection_name](std::shared_ptr<WsServer::Connection> connection, const int status, const std::string &)
    {
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Command server closed {} connection: {:#x}, status code: {}", connection_name, reinterpret_cast<std::uintptr_t>(connection.get()), status);
    };
    endpoint.on_handshake = [connection_name](std::shared_ptr<WsServer::Connection>, SimpleWeb::CaseInsensitiveMultimap &)
    {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };
    endpoint.on_error = [connection_name](std::shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &error_code)
    {
        LOGGER_LOG_ERROR(std::cerr, kLogTag, "Command server error: {}, {} connection: {:#x}", error_code.message(), connection_name, reinterpret_cast<std::uintptr_t>(connection.get()));
    };
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Set up serial port
    if (argc < 4)
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
    std::shared_ptr<cavebox::ListenerCallbacks> listener_callbacks = std::make_shared<cavebox::ListenerCallbacks>(talker, argv[3]);
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
    WsServer command_server;
    command_server.config.port = kWsPort;
    InitializeEndpoint(command_server.endpoint[kArmEndpoint],
                       "arm",
                       [talker](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> message)
    {
        UNUSED(connection);

        bool arm = false;

        message->read(reinterpret_cast<char *>(&arm), sizeof(arm));

        talker->SpeakArm(arm);

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Arm message {} received", arm);
    });
    InitializeEndpoint(command_server.endpoint[kCameraEndpoint],
                       "camera",
                       [talker](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> message)
    {
        UNUSED(connection);

        CaveTalk_Radian_t pan  = 0U;
        CaveTalk_Radian_t tilt = 0U;

        message->read(reinterpret_cast<char *>(&pan), sizeof(pan));
        message->read(reinterpret_cast<char *>(&tilt), sizeof(tilt));

        talker->SpeakCameraMovement(pan, tilt);

        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Camera message pan {} tilt {} received", pan, tilt);
    });
    InitializeEndpoint(command_server.endpoint[kDriveEndpoint],
                       "drive",
                       [talker](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> message)
    {
        UNUSED(connection);

        CaveTalk_MetersPerSecond_t speed      = 0U;
        CaveTalk_RadiansPerSecond_t turn_rate = 0U;

        message->read(reinterpret_cast<char *>(&speed), sizeof(speed));
        message->read(reinterpret_cast<char *>(&turn_rate), sizeof(turn_rate));

        talker->SpeakMovement(speed, turn_rate);

        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Drive message speed {} turn rate {} received", speed, turn_rate);
    });
    InitializeEndpoint(command_server.endpoint[kMoveEndpoint],
                       "move",
                       [talker](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> message)
    {
        UNUSED(connection);

        CaveTalk_Meter_t position = 0U;
        CaveTalk_Radian_t pose    = 0U;

        message->read(reinterpret_cast<char *>(&position), sizeof(position));
        message->read(reinterpret_cast<char *>(&pose), sizeof(pose));

        talker->SpeakRelativeMove(cave_talk::RELATIVE_MOVE_TYPE_CMD, position, pose);

        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "Move message position {} pose {} received", position, pose);
    });
    std::thread command_server_thread([&command_server]()
    {
        command_server.start([](const std::uint16_t port) {
            LOGGER_LOG_DEBUG(std::cout, kLogTag, "Command server listening on port: {}", port);
        });

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Command server stopped");
    });

    std::chrono::steady_clock::time_point last_movement  = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_send      = std::chrono::steady_clock::now();
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
        if (listener_callbacks->IsConnected() && (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_movement) >= std::chrono::milliseconds(50)))
        {
            if ((speed != last_speed) || (turn_rate != last_turn_rate))
            {
                talker->SpeakMovement(speed, turn_rate);

                last_speed     = speed;
                last_turn_rate = turn_rate;
                last_send      = now;
            }

            if ((pan != last_pan) || (tilt != last_tilt))
            {
                talker->SpeakCameraMovement(pan, tilt);

                last_pan  = pan;
                last_tilt = tilt;
                last_send = now;
            }

            last_movement = now;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send) >= std::chrono::milliseconds(500))
        {
            talker->SpeakOogaBooga(cave_talk::Say::SAY_OOGA);
            last_send = now;
        }
    }

    command_server.stop();
    command_server_thread.join();
    controller_handler.Stop();
    game_controller::Deinitialize();

    return 0;
}