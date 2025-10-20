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

    switch (static_cast<game_controller::Button>(event.cbutton.button))
    {
    case game_controller::Button::BUTTON_A:
        armed_ = !armed_;
        talker_->SpeakArm(armed_);
        LOGGER_LOG_INFO(std::cout, kLogTag, "Armed: {}", armed_);
    default:
        break;
    }
}

void InputHandler::HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event)
{
    UNUSED(controller);

    switch (static_cast<game_controller::JoystickAxis>(event.jaxis.axis))
    {
    case game_controller::JoystickAxis::LEFT_X:
        turn_rate_.store(Map(event.jaxis.value, INT16_MIN, INT16_MAX, -1, 1));
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "LEFT X {}, turn rate {}", event.jaxis.value, turn_rate_.load());
        break;
    case game_controller::JoystickAxis::TRIGGER_LEFT:
        speed_.store(Map(-event.jaxis.value, INT16_MIN, INT16_MAX, -1, 1)); // TODO adjust mapping -[0, 1]?
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "TRIGGER LEFT {}, speed {}", event.jaxis.value, speed_.load());
        break;
    case game_controller::JoystickAxis::TRIGGER_RIGHT:
        speed_.store(Map(event.jaxis.value, INT16_MIN, INT16_MAX, -1, 1)); // TODO adjust mapping [0, 1]?
        LOGGER_LOG_VERBOSE(std::cout, kLogTag, "TRIGGER RIGHT {}, speed {}", event.jaxis.value, speed_.load());
        break;
    default:
        break;
    }
}

void InputHandler::HandleCameraCommand(const double pan_radians, const double tilt_radians)
{
    pan_.store(pan_radians);
    pan_.store(tilt_radians);
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