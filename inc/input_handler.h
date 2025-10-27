#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <atomic>
#include <memory>

#include "game_controller.h"
#include "talker.h"

namespace cavebox
{

class InputHandler : public game_controller::InputHandler
{
    public:
        InputHandler(std::shared_ptr<Talker> talker);
        void HandleButtonDown(const game_controller::Controller *const controller, game_controller::Event &event);
        void HandleAxisMotion(const game_controller::Controller *const controller, game_controller::Event &event);
        void HandleCameraCommand(const double pan_radians, const double tilt_radians);
        void HandleDriveCommand(const double speed_meters_per_second, const double turn_rate_radians_per_second);
        double GetSpeed(void) const;
        double GetTurnRate(void) const;
        double GetPan(void) const;
        double GetTilt(void) const;

    private:
        std::shared_ptr<Talker> talker_;
        std::atomic<double> speed_;
        std::atomic<double> turn_rate_;
        std::atomic<double> pan_;
        std::atomic<double> tilt_;
        bool armed_ = false;
};

} // namespace cavebox

#endif // INPUT_HANDLER_H