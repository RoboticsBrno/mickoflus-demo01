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
#include "RBControl_battery.hpp"

#include "motors.hpp"

// CHANGE THESE so you can find the robot in the Android app
#define OWNER "FrantaFlinta"
#define NAME "FlusMcFlusy"

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
    blufi_init(NAME);   // Init bluetooth-wifi setup via EspBlufi app
    rb_web_start(80);   // Start web server with control page (see data/index.html)

    rb::Manager man;    // Initialize the robot manager

    // Set motor power limits
    man.setMotors()
        .pwmMaxPercent(0, 70)  // left wheel
        .pwmMaxPercent(1, 70)  // right wheel
        .pwmMaxPercent(2, 28)  // turret left/right
        .pwmMaxPercent(3, 45)  // turret up/down
        .set();

    // Initialize the communication protocol
    RbProtocol rb(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, &onPktReceived, &man);
    rb.start();

    printf("%s's mickoflus '%s' started!\n", OWNER, NAME);

    // Turn on the LED according to pressed buttons
    if(man.expander().digitalRead(rb::SW1) != 0) {
        man.leds().yellow();
    } else {
        man.leds().green();
    }

    int i = 0;
    const auto& bat = man.battery();
    while(true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(rb.is_possessed()) {
            // Send text to the android application
            rb.send_log("Tick #%d, battery at %d%%, %dmv\n", i++, bat.pct(), bat.voltageMv());
        }
    }
}
