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

    if(data->size() < 2) {
        return;
    }

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

        builder.power(MOTOR_TURRET_ROTATION, x);
    }


    builder.set();
}
