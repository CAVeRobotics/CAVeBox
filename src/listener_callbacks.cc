#include "listener_callbacks.h"

#include <memory>
#include <string>

#define ASIO_STANDALONE
#include "client_ws.hpp"
#include <nlohmann/json.hpp>

#include "logger.h"
#include "talker.h"

#define UNUSED(x) (void)(x)

static const std::string kLogTag("CAVEBOX");

namespace cavebox
{

ListenerCallbacks::ListenerCallbacks(std::shared_ptr<Talker> talker, const std::string &plotting_endpoint) : talker_(talker), client_(plotting_endpoint)
{
    LOGGER_LOG_DEBUG(std::cout, kLogTag, "Connecting to {}", plotting_endpoint);
    client_.on_open = [this](std::shared_ptr<WsClient::Connection> connection)
    {
        this->client_connected_.store(true);
        this->client_connection_ = connection;

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Plotting client opened connection: {:#x}", reinterpret_cast<std::uintptr_t>(connection.get()));
    };

    client_.on_close = [this](std::shared_ptr<WsClient::Connection> connection, int status, const std::string &)
    {
        this->client_connected_.store(false);

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Plotting client closed connection: {:#x}, status code: {}", reinterpret_cast<std::uintptr_t>(connection.get()), status);
    };

    client_.on_error = [](std::shared_ptr<WsClient::Connection> connection, const SimpleWeb::error_code &error_code)
    {
        LOGGER_LOG_ERROR(std::cerr, kLogTag, "Plotting client error: {}, connection: {:#x}", error_code.message(), reinterpret_cast<std::uintptr_t>(connection.get()));
    };

    client_thread_ = std::thread([this]()
    {
        this->client_.start();

        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Plotting client stopped");
    });
}

ListenerCallbacks::~ListenerCallbacks()
{
    client_.stop();
    client_thread_.join();
}

void ListenerCallbacks::HearOogaBooga(const cave_talk::Say ooga_booga)
{
    switch (ooga_booga)
    {
    case cave_talk::Say::SAY_OOGA:
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Heard Ooga");
        talker_->SpeakOogaBooga(cave_talk::Say::SAY_BOOGA);
        if (!connected_.load())
        {
            talker_->SpeakOogaBooga(cave_talk::Say::SAY_OOGA);
        }
        break;
    case cave_talk::Say::SAY_BOOGA:
        LOGGER_LOG_DEBUG(std::cout, kLogTag, "Heard Booga");
        if (!connected_.load())
        {
            connected_.store(true);

            LOGGER_LOG_INFO(std::cout, kLogTag, "Connected");
        }
        break;
    default:
        break;
    }
}

void ListenerCallbacks::HearMovement(const CaveTalk_MetersPerSecond_t speed, const CaveTalk_RadiansPerSecond_t turn_rate)
{
    UNUSED(speed);
    UNUSED(turn_rate);
}

void ListenerCallbacks::HearCameraMovement(const CaveTalk_Radian_t pan, const CaveTalk_Radian_t tilt)
{
    UNUSED(pan);
    UNUSED(tilt);
}

void ListenerCallbacks::HearLights(const bool headlights)
{
    UNUSED(headlights);
}

void ListenerCallbacks::HearArm(const bool arm)
{
    UNUSED(arm);
}

void ListenerCallbacks::HearOdometry(const cave_talk::Imu &IMU,
                                     const cave_talk::Encoder &encoder_wheel_0,
                                     const cave_talk::Encoder &encoder_wheel_1,
                                     const cave_talk::Encoder &encoder_wheel_2,
                                     const cave_talk::Encoder &encoder_wheel_3,
                                     const cave_talk::Pose &pose)
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

    LOGGER_LOG_VERBOSE(std::cout,
                       kLogTag,
                       "Pose: {}, {}, {}",
                       pose.x_meters(),
                       pose.y_meters(),
                       pose.heading_radians());

    if (client_connected_.load())
    {
        nlohmann::json odometry_json;
        odometry_json["odometry"]["imu"]["acceleration"] = {
            {"x", IMU.accel().x_meters_per_second_squared()},
            {"y", IMU.accel().y_meters_per_second_squared()},
            {"z", IMU.accel().z_meters_per_second_squared()},
        };
        odometry_json["odometry"]["imu"]["angular_rate"] = {
            {"roll", IMU.gyro().roll_radians_per_second()},
            {"pitch", IMU.gyro().pitch_radians_per_second()},
            {"yaw", IMU.gyro().yaw_radians_per_second()},
        };
        odometry_json["odometry"]["imu"]["quaternion"] = {
            {"w", IMU.quat().w()},
            {"x", IMU.quat().x()},
            {"y", IMU.quat().y()},
            {"z", IMU.quat().z()},
        };
        odometry_json["odometry"]["encoders"]["0"] = {
            {"pulses", encoder_wheel_0.total_pulses()},
            {"rate", encoder_wheel_0.rate_radians_per_second()},
        };
        odometry_json["odometry"]["encoders"]["1"] = {
            {"pulses", encoder_wheel_1.total_pulses()},
            {"rate", encoder_wheel_1.rate_radians_per_second()},
        };
        odometry_json["odometry"]["encoders"]["2"] = {
            {"pulses", encoder_wheel_2.total_pulses()},
            {"rate", encoder_wheel_2.rate_radians_per_second()},
        };
        odometry_json["odometry"]["encoders"]["3"] = {
            {"pulses", encoder_wheel_3.total_pulses()},
            {"rate", encoder_wheel_3.rate_radians_per_second()},
        };
        odometry_json["odometry"]["pose"] = {
            {"x", pose.x_meters()},
            {"y", pose.y_meters()},
            {"heading", pose.heading_radians()},
        };
        client_connection_->send(odometry_json.dump());
    }
}

void ListenerCallbacks::HearLog(const char *const log)
{
    LOGGER_LOG_INFO(std::cout, kLogTag, "CAVeTalk Log: {}", log);
}

void ListenerCallbacks::HearConfigServoWheels(const cave_talk::Servo &servo_wheel_0,
                                              const cave_talk::Servo &servo_wheel_1,
                                              const cave_talk::Servo &servo_wheel_2,
                                              const cave_talk::Servo &servo_wheel_3)
{
    UNUSED(servo_wheel_0);
    UNUSED(servo_wheel_1);
    UNUSED(servo_wheel_2);
    UNUSED(servo_wheel_3);
}

void ListenerCallbacks::HearConfigServoCams(const cave_talk::Servo &servo_cam_pan, const cave_talk::Servo &servo_cam_tilt)
{
    UNUSED(servo_cam_pan);
    UNUSED(servo_cam_tilt);
}

void ListenerCallbacks::HearConfigMotor(const cave_talk::Motor &motor_wheel_0,
                                        const cave_talk::Motor &motor_wheel_1,
                                        const cave_talk::Motor &motor_wheel_2,
                                        const cave_talk::Motor &motor_wheel_3)
{
    UNUSED(motor_wheel_0);
    UNUSED(motor_wheel_1);
    UNUSED(motor_wheel_2);
    UNUSED(motor_wheel_3);
}

void ListenerCallbacks::HearConfigEncoder(const cave_talk::ConfigEncoder &encoder_wheel_0,
                                          const cave_talk::ConfigEncoder &encoder_wheel_1,
                                          const cave_talk::ConfigEncoder &encoder_wheel_2,
                                          const cave_talk::ConfigEncoder &encoder_wheel_3)
{
    UNUSED(encoder_wheel_0);
    UNUSED(encoder_wheel_1);
    UNUSED(encoder_wheel_2);
    UNUSED(encoder_wheel_3);
}

void ListenerCallbacks::HearConfigLog(const cave_talk::LogLevel log_level)
{
    UNUSED(log_level);
}

void ListenerCallbacks::HearConfigWheelSpeedControl(const cave_talk::PID &wheel_0_params,
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

void ListenerCallbacks::HearConfigSteeringControl(const cave_talk::PID &turn_rate_params, const bool enabled)
{
    UNUSED(turn_rate_params);
    UNUSED(enabled);
}

void ListenerCallbacks::HearAirQuality(const uint32_t dust_ug_per_m3, const uint32_t gas_ppm, const double temperature_celsius)
{
    UNUSED(dust_ug_per_m3);
    UNUSED(gas_ppm);
    UNUSED(temperature_celsius);
}

void ListenerCallbacks::HearRelativeMove(const cave_talk::RelativeMoveType type, const CaveTalk_Meter_t position, const CaveTalk_Radian_t pose)
{
    /* TODO */
    UNUSED(position);
    UNUSED(pose);

    if (cave_talk::RelativeMoveType::RELATIVE_MOVE_TYPE_ACK == type)
    {
        relative_move_ack_.store(true);
    }
}

void ListenerCallbacks::HearWaypoint(const cave_talk::WaypointType type, const CaveTalk_Meter_t x, const CaveTalk_Meter_t y, const CaveTalk_Radian_t heading)
{
    /* TODO */
    UNUSED(x);
    UNUSED(y);
    UNUSED(heading);

    if (cave_talk::WaypointType::WAYPOINT_TYPE_ACK == type)
    {
        waypoint_ack_.store(true);
    }
}


bool ListenerCallbacks::IsConnected(void) const
{
    return connected_.load();
}

bool ListenerCallbacks::IsRelativeMoveComplete(void)
{
    bool complete = relative_move_ack_.load();

    if (complete)
    {
        relative_move_ack_.store(false);
    }

    return complete;
}

bool ListenerCallbacks::IsWaypointReached(void)
{
    bool reached = waypoint_ack_.load();

    if (reached)
    {
        waypoint_ack_.store(false);
    }

    return reached;
}


} // namespace cavebox
