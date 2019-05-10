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

// CHANGE THESE so you can find the robot in the Android app
#define OWNER "FrantaFlinta"
#define NAME "FlusMcFlusy"

// CHANGE THESE to your WiFi's settings
#define WIFI_NAME "Technika"
#define WIFI_PASSWORD "materidouska"

void setup() {
    // Initialize the robot manager
    rb::Manager man;

    // Set the battery measuring coefficient.
    // Measure voltage at battery connector and
    // coef = voltageMeasureAtBatteriesInMilliVolts / raw
    auto& batt = man.battery();
    batt.setCoef(9.185f);

    // Connect to the WiFi network
    // If the button 1 is not pressed: connect to WIFI_NAME
    // else create an AP.
    if(man.expander().digitalRead(rb::SW1) == 0) {
        man.leds().yellow();
        rb::WiFi::connect(WIFI_NAME, WIFI_PASSWORD);
    } else {
        man.leds().green();
        rb::WiFi::startAp("Flus" OWNER "-" NAME, "flusflus");
    }

    rb_web_start(80);   // Start web server with control page (see data/index.html)

    // Set motor power limits
    man.setMotors()
        .pwmMaxPercent(MOTOR_LEFT, 100)
        .pwmMaxPercent(MOTOR_RIGHT, 100)
        .pwmMaxPercent(MOTOR_TURRET_ROTATION, 30)
        .pwmMaxPercent(rb::MotorId::M2, 100)
        .set();

    auto& servos = man.initSmartServoBus(3);
    servos.limit(0,  0_deg, 220_deg );
    servos.limit(1, 85_deg, 210_deg );
    servos.limit(2, 75_deg, 160_deg);

    float p1 = servos.pos(0);
    float p2 = servos.pos(1);
    float p3 = servos.pos(2);
    printf("%f\n", p1);
    printf("%f\n", p2);
    printf("%f\n", p3);

    bool isGrabbing = false;

    // Initialize the communication protocol
    rb::Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, [&](const std::string& command, rbjson::Object *pkt) {
        if(command == "joy") {
            motors_handle_joysticks(man, pkt);
        } else if(command == "arm0") {
            const rbjson::Array *angles = pkt->getArray("a");
            auto &bus = man.servoBus();
            //printf("%f %f\n", angles->getDouble(0, 0), angles->getDouble(1, 0));
            bus.set(0, angles->getDouble(0, 0), 150, 0.07f);
            bus.set(1, angles->getDouble(1, 0), 150, 0.07f);
        } else if(command == "grab") {
            isGrabbing = !isGrabbing;
            man.servoBus().set(2, isGrabbing ? 75 : 160);
        }
    });

    prot.start();

    //printf("%s's mickoflus '%s' started!\n", OWNER, NAME);

    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    //printf("\n\nBATTERY CALIBRATION INFO: %d (raw) * %.2f (coef) = %dmv\n\n\n", batt.raw(), batt.coef(), batt.voltageMv());

    int iter = 0;
    man.schedule(1000, [&]() -> bool {
        if(prot.is_possessed()) {
            // Send text to the android application
            prot.send_log("Tick #%d, battery at %d%%, %dmv\n", iter++, batt.pct(), batt.voltageMv());
        }
        return true;
    });

    Serial.begin(115200);

    char buff[32] = {
        0x55, 0x55, 3, 0, (uint8_t)p1, (uint8_t)p2, (uint8_t)p3,
    };

    Serial.write((uint8_t*)buff, 7);

    while(true) {
        while(Serial.read() != 0x55) {
            vTaskDelay(100);
        }

        if(Serial.read() != 0x55)
            continue;

        int cmd = Serial.read();
        int len = Serial.read();
        Serial.readBytes(buff, len);

        switch(cmd) {
            case 0: {
                int id = buff[0];
                float angle = ((int)buff[1]) & 0xFF;
                servos.set(id, angle, 80);
                break;
            }
        }
    }
}

void loop() {

}
