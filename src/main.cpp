#include <esp_log.h>
#include <lwip/sockets.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "blufi.h"
#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl_motors.hpp"

#define NAME "FlusOne"

struct ctx_t {
    ctx_t() { }
    ~ctx_t() {}

    rb::Motors motors;
};

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

void onPktReceived(void *cookie, const std::string& command, rbjson::Object *pkt) {
    struct ctx_t *ctx = (ctx_t*)cookie;
    if(command == "joy") {
        const rbjson::Array *data = pkt->getArray("data");
        
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

            ctx->motors.motor(0).power(l);
            ctx->motors.motor(1).power(r);
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

            ctx->motors.motor(2).power(x);
            ctx->motors.motor(3).power(y);
        }

        ctx->motors.update();
    } else if(command == "fire") {
        rb::Motor& gun = ctx->motors.motor(4);
        gun.power(100);
        ctx->motors.update();

        printf("\n\nFIRE THE MISSILESS\n\n");

        vTaskDelay(2500 / portTICK_PERIOD_MS);
        gun.power(0);
        ctx->motors.update();
    }
}

extern "C" void app_main() {
    blufi_init(NAME);

    rb_web_start(80);

    struct ctx_t ctx;

    ctx.motors.motor(0).pwmMaxPercent(70);
    ctx.motors.motor(1).pwmMaxPercent(70);
    ctx.motors.motor(2).pwmMaxPercent(25);
    ctx.motors.motor(3).pwmMaxPercent(50);
    
    RbProtocol rb("Robocamp", NAME, "Compiled at " __DATE__ " " __TIME__, &onPktReceived, &ctx);
    rb.start();

    printf("Hello world!\n");

    int i = 0;
    while(true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(rb.is_possessed()) {
            rb.send_log("Tick #%d\n", i++);
        }
    }
}