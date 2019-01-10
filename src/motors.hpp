#pragma once

#include "RBControl_pinout.hpp"

namespace rb {
    class Manager;
};

namespace rbjson {
    class Object;
};

#define MOTOR_LEFT rb::MotorId::M1
#define MOTOR_RIGHT rb::MotorId::M5
#define MOTOR_TURRET_ROTATION rb::MotorId::M2

void motors_handle_joysticks(rb::Manager& man, rbjson::Object *pkt);