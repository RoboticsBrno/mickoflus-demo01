#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include <Arduino.h>

#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl_manager.hpp"
#include "RBControl_battery.hpp"
#include "RBControl_wifi.hpp"

#include "motors.hpp"

using namespace std::chrono_literals; // to use ms/s/h etc.
#include "lx16a.hpp"

// CHANGE THESE so you can find the robot in the Android app
#define OWNER "FrantaFlinta"
#define NAME "FlusMcFlusy"

// CHANGE THESE to your WiFi's settings
#define WIFI_NAME "domov"
#define WIFI_PASSWORD "Monty2aTara"

void onPktReceived(rb::Protocol& protocol, void *cookie, const std::string& command, rbjson::Object *pkt) {
    auto man = (rb::Manager*)cookie;
    if(command == "joy") {
        motors_handle_joysticks(man, pkt);
    } else if(command == "arm") {
        const rbjson::Array *data = pkt->getArray("data");
        const rbjson::Array *angles = data->getObject(0)->getArray("a");
        man->servoBus().set(0, angles->getDouble(0, 0), 130);
        man->servoBus().set(1, angles->getDouble(1, 0), 130);
    }
}

extern "C" void app_main() {
    // Initialize the robot manager
    rb::Manager man;

    auto& servos = man.initServoBus(2);
    servos.limit(0,  0_deg, 180_deg );
    servos.limit(1,  6_deg, 240_deg );

    // Set the battery measuring coefficient.
    // Measure voltage at battery connector and
    // coef = voltageMeasureAtBatteriesInMilliVolts / raw
    auto& batt = man.battery();
    batt.setCoef(10);

    // Connect to the WiFi network
    // If the button 1 is not pressed: connect to WIFI_NAME
    // else create an AP.
    if(man.expander().digitalRead(rb::SW1) != 0) {
        man.leds().yellow();
        rb::WiFi::connect(WIFI_NAME, WIFI_PASSWORD);
    } else {
        man.leds().green();
        rb::WiFi::startAp("Flus" OWNER "-" NAME, "flusflus");
    }

    rb_web_start(80);   // Start web server with control page (see data/index.html)

    // Set motor power limits
    man.setMotors()
        .pwmMaxPercent(MOTOR_LEFT, 70)  // left wheel
        .pwmMaxPercent(MOTOR_RIGHT, 70)  // right wheel
        .set();

    // Initialize the communication protocol
    rb::Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, &onPktReceived, &man);
    prot.start();

    printf("%s's mickoflus '%s' started!\n", OWNER, NAME);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("\n\nBATTERY CALIBRATION INFO: %d (raw) * %.2f (coef) = %dmv\n\n\n", batt.raw(), batt.coef(), batt.voltageMv());

    //servos.set(0, 180, 100);
    //servos.set(1, 0, 100);

    int i = 0;
    while(true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if(prot.is_possessed()) {
            // Send text to the android application
            prot.send_log("Tick #%d, battery at %d%%, %dmv\n", i++, batt.pct(), batt.voltageMv());
        }
    }
}
