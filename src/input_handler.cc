#include "input_handler.h"

#include <memory>

#include "game_controller.h"
#include "logger.h"
#include "talker.h"

#define UNUSED(x) (void)(x)

static const std::string kLogTag("CAVEBOX");

namespace cavebox
{

static double Map(const double value, const double in_min, const double in_max, const double out_min, const double out_max);

InputHandler::InputHandler(std::shared_ptr<Talker> talker) : talker_(talker), speed_(0.0), turn_rate_(0.0)
{
}

void InputHandler::HandleButtonDown(const game_controller::Controller *const controller, game_controller::Event &event)
{
    UNUSED(controller);
    cave_talk::PID pid;

    switch (static_cast<game_controller::Button>(event.cbutton.button))
    {
    case game_controller::Button::BUTTON_A:
        armed_ = !armed_;
        talker_->SpeakArm(armed_);
        LOGGER_LOG_INFO(std::cout, kLogTag, "Armed: {}", armed_);
        break;
    case game_controller::Button::BUTTON_B:
        control_ = !control_;
        talker_->SpeakConfigWheelSpeedControl(pid, pid, pid, pid, control_);
        LOGGER_LOG_INFO(std::cout, kLogTag, "Control: {}", control_);
        break;
    default:
        break;
    }
}

void InputHandler::HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event)
{
    UNUSED(controller);
    double value;

    switch (static_cast<game_controller::JoystickAxis>(event.jaxis.axis))
    {
    case game_controller::JoystickAxis::LEFT_X:
        value = Map(-event.jaxis.value, INT16_MIN, INT16_MAX, -10, 10);
        if (abs(value) < 1.0)
        {
            value = 0.0;
        }
        turn_rate_.store(value);
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "LEFT X {}, turn rate {}", event.jaxis.value, turn_rate_.load());
        break;
    case game_controller::JoystickAxis::TRIGGER_LEFT:
        value = Map(event.jaxis.value, 0, INT16_MAX, 0, 0.8);
        if (abs(value) < 0.30)
        {
            value = 0.0;
        }
        speed_.store(-value);
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "TRIGGER LEFT {}, speed {}", event.jaxis.value, speed_.load());
        break;
    case game_controller::JoystickAxis::TRIGGER_RIGHT:
        value = Map(event.jaxis.value, 0, INT16_MAX, 0, 0.8);
        if (abs(value) < 0.30)
        {
            value = 0.0;
        }
        speed_.store(value);
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "TRIGGER RIGHT {}, speed {}", event.jaxis.value, speed_.load());
        break;
    default:
        break;
    }
}

void InputHandler::HandleCameraCommand(const double pan_radians, const double tilt_radians)
{
    pan_.store(pan_radians);
    tilt_.store(tilt_radians);
}

void InputHandler::HandleDriveCommand(const double speed_meters_per_second, const double turn_rate_radians_per_second)
{
    speed_.store(speed_meters_per_second);
    turn_rate_.store(turn_rate_radians_per_second);
}

double InputHandler::GetSpeed(void) const
{
    return speed_.load();
}

double InputHandler::GetTurnRate(void) const
{
    return turn_rate_.load();
}

double InputHandler::GetPan(void) const
{
    return pan_.load();
}

double InputHandler::GetTilt(void) const
{
    return tilt_.load();
}

static double Map(const double value, const double in_min, const double in_max, const double out_min, const double out_max)
{
    double capped_value = value;

    if (value < in_min)
    {
        capped_value = in_min;
    }
    if (value > in_max)
    {
        capped_value = in_max;
    }

    return (capped_value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
} // namespace cavebox