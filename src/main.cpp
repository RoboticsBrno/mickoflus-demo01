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

static float scale_decel(float x) {
    return (1.f - (1.f - x) * (1.f - x));
}

void onPktReceived(void *cookie, const std::string& command, rbjson::Object *pkt) {
    struct ctx_t *ctx = (ctx_t*)cookie;
    if(command == "joy") {
        rb::Motor& lmotor = ctx->motors.motor(0);
        rb::Motor& rmotor = ctx->motors.motor(1);
        rb::Motor& horturret = ctx->motors.motor(2);
        rb::Motor& verturret = ctx->motors.motor(3);

        rbjson::Array *data = pkt->getArray("data");
        rbjson::Object *joy0 = data->getObject(0);
        rbjson::Object *joy1 = data->getObject(1);
        
        int x0 = joy0->getInt("x");
        int y0 = joy0->getInt("y");

        int x1 = joy1->getInt("x") * -1;
        int y1 = joy1->getInt("y");

        if(x0 != 0)
            x0 = ((float(x0) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        if(y0 != 0)
            y0 = ((float(y0) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;

        if(x1 > -6000 && x1 < 6000)
            x1 = 0;
        else
            x1 = scale_decel((float(x1) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;

        if(y1 > -6000 && y1 < 6000)
            y1 = 0;
        else if(y1 > 0)
            y1 = scale_decel((float(y1) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        else
            y1 = scale_decel((float(y1) - (-32767.f)) / (32767.f - (-32767.f))) * 100.f - 50.f;

        int r = ((y0 - (x0/2)));
        int l = ((y0 + (x0/2)));
        if(r < 0 && l < 0) {
            int tmp = r;
            r = l;
            l = tmp;
        }

        //printf("%d %d | %d %d\n", x, y, l, r);

        lmotor.power(l);
        rmotor.power(r);
        horturret.power(x1);
        verturret.power(y1);
        ctx->motors.update();
    } else if(command == "fire") {
        rb::Motor& gun = ctx->motors.motor(4);
        gun.power(100);
        ctx->motors.update();

        printf("\n\nFIRE THE MISSILESS\n\n");

        vTaskDelay(3000 / portTICK_PERIOD_MS);
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
    ctx.motors.motor(3).pwmMaxPercent(35);
    
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