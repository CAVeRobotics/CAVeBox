#ifndef TALKER_H
#define TALKER_H

#include <functional>
#include <mutex>

#include "cave_talk.h"

#include "queue.h"

namespace cavebox
{

// TODO message rate limiting
class Talker : public cave_talk::Talker
{
    public:
        explicit Talker(CaveTalk_Error_t (*send)(const void *const data, const size_t size));
        Talker(Talker &talker)                  = delete;
        Talker(Talker &&talker)                 = delete;
        Talker &operator=(const Talker &talker) = delete;
        Talker &operator=(Talker &&talker)      = delete;
        CaveTalk_Error_t Run(void);
        void SpeakOogaBooga(const cave_talk::Say ooga_booga);
        void SpeakMovement(const CaveTalk_MetersPerSecond_t speed, const CaveTalk_RadiansPerSecond_t turn_rate);
        void SpeakCameraMovement(const CaveTalk_Radian_t pan, const CaveTalk_Radian_t tilt);
        void SpeakLights(const bool headlights);
        void SpeakArm(const bool arm);
        void SpeakOdometry(const cave_talk::Imu &IMU, const cave_talk::Encoder &encoder_wheel_0, const cave_talk::Encoder &encoder_wheel_1, const cave_talk::Encoder &encoder_wheel_2, const cave_talk::Encoder &encoder_wheel_3);
        void SpeakLog(const char *const log);
        void SpeakConfigServoWheels(const cave_talk::Servo &servo_wheel_0, const cave_talk::Servo &servo_wheel_1, const cave_talk::Servo &servo_wheel_2, const cave_talk::Servo &servo_wheel_3);
        void SpeakConfigServoCams(const cave_talk::Servo &servo_cam_pan, const cave_talk::Servo &servo_cam_tilt);
        void SpeakConfigMotor(const cave_talk::Motor &motor_wheel_0, const cave_talk::Motor &motor_wheel_1, const cave_talk::Motor &motor_wheel_2, const cave_talk::Motor &motor_wheel_3);
        void SpeakConfigEncoder(const cave_talk::ConfigEncoder &encoder_wheel_0, const cave_talk::ConfigEncoder &encoder_wheel_1, const cave_talk::ConfigEncoder &encoder_wheel_2, const cave_talk::ConfigEncoder &encoder_wheel_3);
        void SpeakConfigLog(const cave_talk::LogLevel log_level);
        void SpeakConfigWheelSpeedControl(const cave_talk::PID &wheel_0_params, const cave_talk::PID &wheel_1_params, const cave_talk::PID &wheel_2_params, const cave_talk::PID &wheel_3_params, const bool enabled);
        void SpeakConfigSteeringControl(const cave_talk::PID &turn_rate_params, const bool enabled);
        void SpeakAirQuality(const uint32_t dust_ug_per_m3, const uint32_t gas_ppm, const double temperature_celsius);

    private:
        Queue<std::function<CaveTalk_Error_t(void)>> speak_queue_;
        std::mutex speak_mutex_;
};

} // namespace cavebox

#endif // TALKER_H