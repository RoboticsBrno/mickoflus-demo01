#include <esp_log.h>
#include <lwip/sockets.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "blufi.h"
#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl_manager.hpp"

#include "motors.hpp"

#define OWNER "Robocamp"
#define NAME "FlusOne"

void onPktReceived(void *cookie, const std::string& command, rbjson::Object *pkt) {
    auto man = (rb::Manager*)cookie;
    if(command == "joy") {
        motors_handle_joysticks(man, pkt);
    } else if(command == "fire") {
        printf("\n\nFIRE THE MISSILESS\n\n");
        motors_fire_gun(man);
    }
}

extern "C" void app_main() {
    blufi_init(NAME);

    rb_web_start(80);

    rb::Manager man;
    man.setMotors()
        .pwmMaxPercent(0, 70)
        .pwmMaxPercent(1, 70)
        .pwmMaxPercent(2, 28)
        .pwmMaxPercent(3, 45)
        .set();

    RbProtocol rb(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, &onPktReceived, &man);
    rb.start();

    printf("%s's mickoflus '%s' started!\n", OWNER, NAME);

    int i = 0;
    while(true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(rb.is_possessed()) {
            rb.send_log("Tick #%d\n", i++);
        }
    }
}