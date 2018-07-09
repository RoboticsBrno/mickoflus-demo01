#pragma once

namespace rb {
    class Manager;
};

namespace rbjson {
    class Object;
};

void motors_handle_joysticks(rb::Manager *man, rbjson::Object *pkt);
void motors_fire_gun(rb::Manager *man);