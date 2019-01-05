#include "RBControl_manager.hpp"
#include "rbprotocol.h"

#include "motors.hpp"

static int scale_range(float x, float omin, float omax, float nmin, float nmax) {
    return ((float(x) - omin) / (omax - omin)) * (nmax - nmin) - nmax;
}

static int scale_motors(float val) {
    return scale_range(val, RBPROTOCOL_AXIS_MIN, RBPROTOCOL_AXIS_MAX, -100.f, 100.f);
}

void motors_handle_joysticks(rb::Manager *man, rbjson::Object *pkt) {
    const rbjson::Array *data = pkt->getArray("data");

    auto builder = man->setMotors();

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

    builder.set();
}
