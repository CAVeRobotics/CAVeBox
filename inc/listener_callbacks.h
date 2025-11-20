#ifndef LISTENER_CALLBACKS_H
#define LISTENER_CALLBACKS_H

#include <atomic>
#include <memory>
#include <string>

#define ASIO_STANDALONE
#include "client_ws.hpp"

#include "cave_talk.h"

#include "talker.h"

namespace cavebox
{

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

class ListenerCallbacks : public cave_talk::ListenerCallbacks
{
    public:
        ListenerCallbacks(std::shared_ptr<Talker> talker, const std::string &plotting_endpoint);
        ListenerCallbacks(ListenerCallbacks &listener_callbacks)                  = delete;
        ListenerCallbacks(ListenerCallbacks &&listener_callbacks)                 = delete;
        ListenerCallbacks &operator=(const ListenerCallbacks &listener_callbacks) = delete;
        ListenerCallbacks &operator=(ListenerCallbacks &&listener_callbacks)      = delete;
        ~ListenerCallbacks();
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
        void HearRelativeMove(const cave_talk::RelativeMoveType type, const CaveTalk_Meter_t position, const CaveTalk_Radian_t pose);
        bool IsConnected(void) const;

    private:
        std::shared_ptr<Talker> talker_;
        WsClient client_;
        std::atomic_bool connected_        = false;
        std::atomic_bool client_connected_ = false;
        std::shared_ptr<WsClient::Connection> client_connection_;
        std::thread client_thread_;
};

} // namespace cavebox

#endif // LISTENER_CALLBACKS_h