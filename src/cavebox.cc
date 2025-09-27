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
#include "logger.h"
#include "serial.h"

#define UNUSED(x) (void)(x)

static const std::string             kLogTag("CAVEBOX");
static bool                          stop_signal = false;
static std::shared_ptr<serial::Port> serial_port;

class CaveboxListenerCallbacks : public cave_talk::ListenerCallbacks
{
    public:
        CaveboxListenerCallbacks(std::shared_ptr<cave_talk::Talker> talker);
        CaveboxListenerCallbacks(CaveboxListenerCallbacks &cavebox_listener_callbacks)                  = delete;
        CaveboxListenerCallbacks(CaveboxListenerCallbacks &&cavebox_listener_callbacks)                 = delete;
        CaveboxListenerCallbacks &operator=(const CaveboxListenerCallbacks &cavebox_listener_callbacks) = delete;
        CaveboxListenerCallbacks &operator=(CaveboxListenerCallbacks &&cavebox_listener_callbacks)      = delete;
        ~CaveboxListenerCallbacks();
        void HearOogaBooga(const cave_talk::Say ooga_booga);
        void HearMovement(const CaveTalk_MetersPerSecond_t speed, const CaveTalk_RadiansPerSecond_t turn_rate);
        void HearCameraMovement(const CaveTalk_Radian_t pan, const CaveTalk_Radian_t tilt);
        void HearLights(const bool headlights);
        void HearArm(const bool arm);
        void HearOdometry(const cave_talk::Imu &IMU,
                          const cave_talk::Encoder &encoder_wheel_0,
                          const cave_talk::Encoder &encoder_wheel_1,
                          const cave_talk::Encoder &encoder_wheel_2,
                          const cave_talk::Encoder &encoder_wheel_3);
        void HearLog(const char *const log);
        void HearConfigServoWheels(const cave_talk::Servo &servo_wheel_0,
                                   const cave_talk::Servo &servo_wheel_1,
                                   const cave_talk::Servo &servo_wheel_2,
                                   const cave_talk::Servo &servo_wheel_3);
        void HearConfigServoCams(const cave_talk::Servo &servo_cam_pan, const cave_talk::Servo &servo_cam_tilt);
        void HearConfigMotor(const cave_talk::Motor &motor_wheel_0,
                             const cave_talk::Motor &motor_wheel_1,
                             const cave_talk::Motor &motor_wheel_2,
                             const cave_talk::Motor &motor_wheel_3);
        void HearConfigEncoder(const cave_talk::ConfigEncoder &encoder_wheel_0,
                               const cave_talk::ConfigEncoder &encoder_wheel_1,
                               const cave_talk::ConfigEncoder &encoder_wheel_2,
                               const cave_talk::ConfigEncoder &encoder_wheel_3);
        void HearConfigLog(const cave_talk::LogLevel log_level);
        void HearConfigWheelSpeedControl(const cave_talk::PID &wheel_0_params,
                                         const cave_talk::PID &wheel_1_params,
                                         const cave_talk::PID &wheel_2_params,
                                         const cave_talk::PID &wheel_3_params,
                                         const bool enabled);
        void HearConfigSteeringControl(const cave_talk::PID &turn_rate_params, const bool enabled);
        void HearAirQuality(const uint32_t dust_ug_per_m3, const uint32_t gas_ppm, const double temperature_celsius);

    private:
        std::shared_ptr<cave_talk::Talker> talker_;
        bool connected_ = false;
};

// TODO need thread-safe access to talker_
class InputHandler : public game_controller::InputHandler
{
    public:
        InputHandler(std::shared_ptr<cave_talk::Talker> talker);
        void HandleButtonDown(const game_controller::Controller *const controller, game_controller::Event &event);
        void HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event);

    private:
        std::shared_ptr<cave_talk::Talker> talker_;
};

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
    std::shared_ptr<cave_talk::Talker> talker = std::make_shared<cave_talk::Talker>([](const void *const data, const std::size_t size)
    {
        (void)serial_port->Write(static_cast<const std::uint8_t *>(data), size);

        return CAVE_TALK_ERROR_NONE;
    });
    cave_talk::Listener listener([](void *const data, const std::size_t size, std::size_t *const bytes_received)
    {
        *bytes_received = serial_port->Read(static_cast<std::uint8_t *>(data), size);

        return CAVE_TALK_ERROR_NONE;
    }, std::make_shared<CaveboxListenerCallbacks>(talker));

    // Set up game controller and input handler
    game_controller::Initialize();
    game_controller::ControllerHandler controller_handler(std::make_shared<InputHandler>(talker));

    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    while (controller_handler.IsRunning() && !stop_signal)
    {
        CaveTalk_Error_t error = listener.Listen();

        if (CAVE_TALK_ERROR_NONE != error)
        {
            LOGGER_LOG_ERROR(std::cerr, kLogTag, "CAVeTalk Listen error: {}", (int)error);
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last) >= std::chrono::milliseconds(100))
        {
            talker->SpeakMovement(0, 0); // Send dummy movement commands for now to maintain connection
            last = now;
        }
    }

    controller_handler.Stop();
    game_controller::Deinitialize();

    return 0;
}

CaveboxListenerCallbacks::CaveboxListenerCallbacks(std::shared_ptr<cave_talk::Talker> talker) : talker_(talker)
{
}

CaveboxListenerCallbacks::~CaveboxListenerCallbacks()
{
}

void CaveboxListenerCallbacks::HearOogaBooga(const cave_talk::Say ooga_booga)
{
    switch (ooga_booga)
    {
    case cave_talk::Say::SAY_OOGA:
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Heard Ooga");
        talker_->SpeakOogaBooga(cave_talk::Say::SAY_BOOGA);
        if (!connected_)
        {
            talker_->SpeakOogaBooga(cave_talk::Say::SAY_OOGA);
        }
        break;
    case cave_talk::Say::SAY_BOOGA:
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Heard Booga");
        if (!connected_)
        {
            connected_ = true;

            LOGGER_LOG_INFO(std::cout, kLogTag, "Connected");
        }
        break;
    default:
        break;
    }
}

void CaveboxListenerCallbacks::HearMovement(const CaveTalk_MetersPerSecond_t speed, const CaveTalk_RadiansPerSecond_t turn_rate)
{
    UNUSED(speed);
    UNUSED(turn_rate);
}

void CaveboxListenerCallbacks::HearCameraMovement(const CaveTalk_Radian_t pan, const CaveTalk_Radian_t tilt)
{
    UNUSED(pan);
    UNUSED(tilt);
}

void CaveboxListenerCallbacks::HearLights(const bool headlights)
{
    UNUSED(headlights);
}

void CaveboxListenerCallbacks::HearArm(const bool arm)
{
    UNUSED(arm);
}

void CaveboxListenerCallbacks::HearOdometry(const cave_talk::Imu &IMU,
                                            const cave_talk::Encoder &encoder_wheel_0,
                                            const cave_talk::Encoder &encoder_wheel_1,
                                            const cave_talk::Encoder &encoder_wheel_2,
                                            const cave_talk::Encoder &encoder_wheel_3)
{
    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Acceleration: {}, {}, {}",
                       IMU.accel().x_meters_per_second_squared(),
                       IMU.accel().y_meters_per_second_squared(),
                       IMU.accel().z_meters_per_second_squared());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Angular Rate: {}, {}, {}",
                       IMU.gyro().roll_radians_per_second(),
                       IMU.gyro().pitch_radians_per_second(),
                       IMU.gyro().yaw_radians_per_second());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Quaternion: {}, {}, {}, {}",
                       IMU.quat().w(),
                       IMU.quat().x(),
                       IMU.quat().y(),
                       IMU.quat().z());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Encoder Wheel 0: {}, {}",
                       encoder_wheel_0.total_pulses(),
                       encoder_wheel_0.rate_radians_per_second());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Encoder Wheel 1: {}, {}",
                       encoder_wheel_1.total_pulses(),
                       encoder_wheel_1.rate_radians_per_second());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Encoder Wheel 2: {}, {}",
                       encoder_wheel_2.total_pulses(),
                       encoder_wheel_2.rate_radians_per_second());

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Encoder Wheel 3: {}, {}",
                       encoder_wheel_3.total_pulses(),
                       encoder_wheel_3.rate_radians_per_second());
}

void CaveboxListenerCallbacks::HearLog(const char *const log)
{
    LOGGER_LOG_INFO(std::cout, kLogTag, "CAVeTalk Log: {}", log);
}

void CaveboxListenerCallbacks::HearConfigServoWheels(const cave_talk::Servo &servo_wheel_0,
                                                     const cave_talk::Servo &servo_wheel_1,
                                                     const cave_talk::Servo &servo_wheel_2,
                                                     const cave_talk::Servo &servo_wheel_3)
{
    UNUSED(servo_wheel_0);
    UNUSED(servo_wheel_1);
    UNUSED(servo_wheel_2);
    UNUSED(servo_wheel_3);
}

void CaveboxListenerCallbacks::HearConfigServoCams(const cave_talk::Servo &servo_cam_pan, const cave_talk::Servo &servo_cam_tilt)
{
    UNUSED(servo_cam_pan);
    UNUSED(servo_cam_tilt);
}

void CaveboxListenerCallbacks::HearConfigMotor(const cave_talk::Motor &motor_wheel_0,
                                               const cave_talk::Motor &motor_wheel_1,
                                               const cave_talk::Motor &motor_wheel_2,
                                               const cave_talk::Motor &motor_wheel_3)
{
    UNUSED(motor_wheel_0);
    UNUSED(motor_wheel_1);
    UNUSED(motor_wheel_2);
    UNUSED(motor_wheel_3);
}

void CaveboxListenerCallbacks::HearConfigEncoder(const cave_talk::ConfigEncoder &encoder_wheel_0,
                                                 const cave_talk::ConfigEncoder &encoder_wheel_1,
                                                 const cave_talk::ConfigEncoder &encoder_wheel_2,
                                                 const cave_talk::ConfigEncoder &encoder_wheel_3)
{
    UNUSED(encoder_wheel_0);
    UNUSED(encoder_wheel_1);
    UNUSED(encoder_wheel_2);
    UNUSED(encoder_wheel_3);
}

void CaveboxListenerCallbacks::HearConfigLog(const cave_talk::LogLevel log_level)
{
    UNUSED(log_level);
}

void CaveboxListenerCallbacks::HearConfigWheelSpeedControl(const cave_talk::PID &wheel_0_params,
                                                           const cave_talk::PID &wheel_1_params,
                                                           const cave_talk::PID &wheel_2_params,
                                                           const cave_talk::PID &wheel_3_params,
                                                           const bool enabled)
{
    UNUSED(wheel_0_params);
    UNUSED(wheel_1_params);
    UNUSED(wheel_2_params);
    UNUSED(wheel_3_params);
    UNUSED(enabled);
}

void CaveboxListenerCallbacks::HearConfigSteeringControl(const cave_talk::PID &turn_rate_params, const bool enabled)
{
    UNUSED(turn_rate_params);
    UNUSED(enabled);
}

void CaveboxListenerCallbacks::HearAirQuality(const uint32_t dust_ug_per_m3, const uint32_t gas_ppm, const double temperature_celsius)
{
    UNUSED(dust_ug_per_m3);
    UNUSED(gas_ppm);
    UNUSED(temperature_celsius);
}

InputHandler::InputHandler(std::shared_ptr<cave_talk::Talker> talker) : talker_(talker)
{
}

void InputHandler::HandleButtonDown(const game_controller::Controller *const controller, game_controller::Event &event)
{
    UNUSED(controller);
    UNUSED(event);
}

void InputHandler::HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event)
{
    UNUSED(controller);
    UNUSED(event);
}