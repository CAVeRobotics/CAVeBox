#include "talker.h"

#include <functional>
#include <mutex>
#include <string>

#include "cave_talk.h"

namespace cavebox
{

Talker::Talker(CaveTalk_Error_t (*send)(const void *const data, const size_t size)) : cave_talk::Talker(send)
{
}

CaveTalk_Error_t Talker::Run(void)
{
    CaveTalk_Error_t error = CAVE_TALK_ERROR_NONE;

    std::lock_guard<std::mutex> lock(speak_mutex_);

    while (!speak_queue_.Empty() && (CAVE_TALK_ERROR_NONE == error))
    {
        error = speak_queue_.Front()();
        speak_queue_.Pop();
    }

    return error;
}

void Talker::SpeakOogaBooga(const cave_talk::Say ooga_booga)
{
    speak_queue_.Push([ =, this]()
    {
        return cave_talk::Talker::SpeakOogaBooga(ooga_booga);
    });
}

void Talker::SpeakMovement(const CaveTalk_MetersPerSecond_t speed, const CaveTalk_RadiansPerSecond_t turn_rate)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakMovement(speed, turn_rate);
    });
}

void Talker::SpeakCameraMovement(const CaveTalk_Radian_t pan, const CaveTalk_Radian_t tilt)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakCameraMovement(pan, tilt);
    });
}

void Talker::SpeakLights(const bool headlights)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakLights(headlights);
    });
}

void Talker::SpeakArm(const bool arm)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakArm(arm);
    });
}

void Talker::SpeakOdometry(const cave_talk::Imu &IMU, const cave_talk::Encoder &encoder_wheel_0, const cave_talk::Encoder &encoder_wheel_1, const cave_talk::Encoder &encoder_wheel_2, const cave_talk::Encoder &encoder_wheel_3)
{
    speak_queue_.Push([ =, this, IMU = IMU, encoder_wheel_0 = encoder_wheel_0, encoder_wheel_1 = encoder_wheel_1, encoder_wheel_2 = encoder_wheel_2, encoder_wheel_3 = encoder_wheel_3]()
    {
        return cave_talk::Talker::SpeakOdometry(IMU, encoder_wheel_0, encoder_wheel_1, encoder_wheel_2, encoder_wheel_3);
    });
}

void Talker::SpeakLog(const char *const log)
{
    speak_queue_.Push([ =, this, log = std::string(log)]()
    {
        return cave_talk::Talker::SpeakLog(log.c_str());
    });
}

void Talker::SpeakConfigServoWheels(const cave_talk::Servo &servo_wheel_0, const cave_talk::Servo &servo_wheel_1, const cave_talk::Servo &servo_wheel_2, const cave_talk::Servo &servo_wheel_3)
{
    speak_queue_.Push([ =, this, servo_wheel_0 = servo_wheel_0, servo_wheel_1 = servo_wheel_1, servo_wheel_2 = servo_wheel_2, servo_wheel_3 = servo_wheel_3]()
    {
        return cave_talk::Talker::SpeakConfigServoWheels(servo_wheel_0, servo_wheel_1, servo_wheel_2, servo_wheel_3);
    });
}

void Talker::SpeakConfigServoCams(const cave_talk::Servo &servo_cam_pan, const cave_talk::Servo &servo_cam_tilt)
{
    speak_queue_.Push([ =, this, servo_cam_pan = servo_cam_pan, servo_cam_tilt = servo_cam_tilt]()
    {
        return cave_talk::Talker::SpeakConfigServoCams(servo_cam_pan, servo_cam_tilt);
    });
}

void Talker::SpeakConfigMotor(const cave_talk::Motor &motor_wheel_0, const cave_talk::Motor &motor_wheel_1, const cave_talk::Motor &motor_wheel_2, const cave_talk::Motor &motor_wheel_3)
{
    speak_queue_.Push([ =, this, motor_wheel_0 = motor_wheel_0, motor_wheel_1 = motor_wheel_1, motor_wheel_2 = motor_wheel_2, motor_wheel_3 = motor_wheel_3]()
    {
        return cave_talk::Talker::SpeakConfigMotor(motor_wheel_0, motor_wheel_1, motor_wheel_2, motor_wheel_3);
    });
}

void Talker::SpeakConfigEncoder(const cave_talk::ConfigEncoder &encoder_wheel_0, const cave_talk::ConfigEncoder &encoder_wheel_1, const cave_talk::ConfigEncoder &encoder_wheel_2, const cave_talk::ConfigEncoder &encoder_wheel_3)
{
    speak_queue_.Push([ =, this, encoder_wheel_0 = encoder_wheel_0, encoder_wheel_1 = encoder_wheel_1, encoder_wheel_2 = encoder_wheel_2, encoder_wheel_3 = encoder_wheel_3]()
    {
        return cave_talk::Talker::SpeakConfigEncoder(encoder_wheel_0, encoder_wheel_1, encoder_wheel_2, encoder_wheel_3);
    });
}

void Talker::SpeakConfigLog(const cave_talk::LogLevel log_level)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakConfigLog(log_level);
    });
}

void Talker::SpeakConfigWheelSpeedControl(const cave_talk::PID &wheel_0_params, const cave_talk::PID &wheel_1_params, const cave_talk::PID &wheel_2_params, const cave_talk::PID &wheel_3_params, const bool enabled)
{
    speak_queue_.Push([ =, this, wheel_0_params = wheel_0_params, wheel_1_params = wheel_1_params, wheel_2_params = wheel_2_params, wheel_3_params = wheel_3_params]()
    {
        return cave_talk::Talker::SpeakConfigWheelSpeedControl(wheel_0_params, wheel_1_params, wheel_2_params, wheel_3_params, enabled);
    });
}

void Talker::SpeakConfigSteeringControl(const cave_talk::PID &turn_rate_params, const bool enabled)
{
    speak_queue_.Push([ =, this, turn_rate_params = turn_rate_params]()
    {
        return cave_talk::Talker::SpeakConfigSteeringControl(turn_rate_params, enabled);
    });
}

void Talker::SpeakAirQuality(const uint32_t dust_ug_per_m3, const uint32_t gas_ppm, const double temperature_celsius)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakAirQuality(dust_ug_per_m3, gas_ppm, temperature_celsius);
    });
}

void Talker::SpeakRelativeMove(const cave_talk::RelativeMoveType type, const CaveTalk_Meter_t position, const CaveTalk_Radian_t pose)
{
    speak_queue_.Push([ =, this ]()
    {
        return cave_talk::Talker::SpeakRelativeMove(type, position, pose);
    });
}

} // namespace cavebox