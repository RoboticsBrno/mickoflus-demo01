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

        rbjson::Array *data = pkt->getArray("data");
        rbjson::Object *joy0 = data->getObject(0);
        
        int x = joy0->getInt("x");
        int y = joy0->getInt("y");

        x = ((float(x) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;
        y = ((float(y) - (-32767.f)) / (32767.f - (-32767.f))) * 200.f - 100.f;

        const int r = ((y - (x/2)));
        const int l = ((y + (x/2)));

        //printf("%d %d | %d %d\n", x, y, l, r);

        lmotor.power(l);
        rmotor.power(r);
        ctx->motors.update();
    } else if(command == "fire") {
        printf("\n\nFIRE THE MISSILESS\n\n");
    }
}

extern "C" void app_main() {
    blufi_init("flus");

    rb_web_start(80);

    struct ctx_t ctx;
    
    RbProtocol rb("Vojta", "Flus", "The very best flus", &onPktReceived, &ctx);
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