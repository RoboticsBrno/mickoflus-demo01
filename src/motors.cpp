#include "RBControl_manager.hpp"
#include "rbprotocol.h"

#include "motors.hpp"

static int iabs(int x) {
    return x >= 0 ? x : -x;
}

static float scale_decel(float x) {
    return (1.f - (1.f - x) * (1.f - x));
}

static int scale_range(float x, float omin, float omax, float nmin, float nmax) {
    return ((float(x) - omin) / (omax - omin)) * (nmax - nmin) - nmax;
}

static int scale_range_decel(float x, float omin, float omax, float nmin, float nmax) {
    return scale_decel((float(x) - omin) / (omax - omin)) * (nmax - nmin) - nmax;
}

static int scale_motors(float val) {
    return scale_range(val, RBPROTOCOL_AXIS_MIN, RBPROTOCOL_AXIS_MAX, -100.f, 100.f);
}

void motors_handle_joysticks(rb::Manager& man, rbjson::Object *pkt) {
    const rbjson::Array *data = pkt->getArray("data");
        
    auto builder = man.setMotors();

    // Drive
    {
        const rbjson::Object *joy = data->getObject(0);
        int x = joy->getInt("x");
        int y = joy->getInt("y");

        if(x != 0)
            x = scale_motors(x);
        if(y != 0)
            y = scale_motors(y);

        int r = ((y - (x/2)));
        int l = ((y + (x/2)));
        if(r < 0 && l < 0) {
            int tmp = r; r = l; l = tmp;
        }

        builder.power(MOTOR_LEFT, l).power(MOTOR_RIGHT, r);
    }

    // Turret
    {
        const rbjson::Object *joy = data->getObject(1);
        int x = joy->getInt("x") * -1;
        int y = joy->getInt("y");

        if(iabs(x) < 6000) {
            x = 0;
        } else {
            x = scale_range_decel(x, RBPROTOCOL_AXIS_MIN, RBPROTOCOL_AXIS_MAX, -100, 100);
        }

        if(x != 0 || iabs(y) < 6000) {
            y = 0;
        } else{
            if(y > 0)
                y = scale_range_decel(y, RBPROTOCOL_AXIS_MIN, RBPROTOCOL_AXIS_MAX, -100, 100);
            else
                y = scale_range_decel(y, RBPROTOCOL_AXIS_MIN, RBPROTOCOL_AXIS_MAX, -75, 75);
        }

        builder.power(MOTOR_TURRET_ROTATION, x).power(MOTOR_TURRET_PITCH, y);
    }

    builder.set();
}

void motors_fire_gun(rb::Manager& man) {
    man.setMotors().power(MOTOR_GUN, 100).set();
    man.schedule(3000, [&]()->bool {
        man.setMotors().power(MOTOR_GUN, 0).set();
        return false;
    });
}