#pragma once

#include "RBControl_pinout.hpp"

namespace rb {
    class Manager;
};

namespace rbjson {
    class Object;
};

#define MOTOR_LEFT rb::MotorId::M1
#define MOTOR_RIGHT rb::MotorId::M2
#define MOTOR_TURRET_ROTATION rb::MotorId::M3
#define MOTOR_TURRET_PITCH rb::MotorId::M4
#define MOTOR_GUN rb::MotorId::M5

void motors_handle_joysticks(rb::Manager& man, rbjson::Object *pkt);
void motors_fire_gun(rb::Manager& man);