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

struct ctx_t {
    ctx_t() { }
    ~ctx_t() {}

    rb::Motors motors;
};

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

        int x1 = joy1->getInt("x");
        int y1 = joy1->getInt("y");

        x0 = ((float(x0) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        y0 = ((float(y0) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        x1 = ((float(x1) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        y1 = ((float(y1) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;

        const int r = ((y0 - (x0/2)));
        const int l = ((y0 + (x0/2)));

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
    blufi_init("FlusOne");

    rb_web_start(80);

    struct ctx_t ctx;

    ctx.motors.motor(2).pwmMaxPercent(30);
    ctx.motors.motor(3).pwmMaxPercent(30);
    
    RbProtocol rb("Robocamp", "FlusOne", "The very best flus", &onPktReceived, &ctx);
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