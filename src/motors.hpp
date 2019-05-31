#pragma once

#include "RBControl_pinout.hpp"

namespace rb {
    class Manager;
};

namespace rbjson {
    class Object;
};

#define MOTOR_LEFT rb::MotorId::M1
#define MOTOR_RIGHT rb::MotorId::M6


void motors_handle_joysticks(rb::Manager& man, rbjson::Object *pkt);