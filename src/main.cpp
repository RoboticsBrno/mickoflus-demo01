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

#include <Servo.h>

#include "motors.hpp"

// CHANGE THESE so you can find the robot in the Android app
#define OWNER "FrantaFlinta"
#define NAME "FlusMcFlusy"

// CHANGE THESE to your WiFi's settings
#define WIFI_NAME "Technika"
#define WIFI_PASSWORD "materidouska"


static int iabs(int x) {
    return x >= 0 ? x : -x;
}

extern "C" void app_main() {
    // Initialize the robot manager
    rb::Manager man;

    auto& servos = man.initSmartServoBus(3);
    //servos.limit(0,  0_deg, 180_deg );
    servos.limit(0, 140_deg, 240_deg );
    servos.limit(2, 0_deg, 180_deg);

    {
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pin_bit_mask = (1ULL << 26);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
    }

    Servo servo_claw;
    servo_claw.attach(26);

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

    bool isGrabbing = false;
    int baseTarget = 0;

    // Initialize the communication protocol
    rb::Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, [&](const std::string& command, rbjson::Object *pkt) {
        if(command == "joy") {
            motors_handle_joysticks(man, pkt);
        } else if(command == "arm0") {
            const rbjson::Array *angles = pkt->getArray("a");
            auto &bus = man.servoBus();
            //printf("%f %f\n", angles->getDouble(1, 0), angles->getDouble(2, 0));
            baseTarget = 180 - angles->getDouble(0, 0);
            bus.set(0, angles->getDouble(1, 0), 130, 0.07f);
            bus.set(2, angles->getDouble(2, 0), 130, 0.07f);
        } else if(command == "grab") {
            servo_claw.write(isGrabbing ? 180 : 90);
            isGrabbing = !isGrabbing;
        }
    });

    prot.start();

    printf("%s's mickoflus '%s' started!\n", OWNER, NAME);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("\n\nBATTERY CALIBRATION INFO: %d (raw) * %.2f (coef) = %dmv\n\n\n", batt.raw(), batt.coef(), batt.voltageMv());


    /*adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_6);

    static const int ADC_SAMPLES = 8;
    man.schedule(10, [&]() -> bool {
        uint32_t adc_reading = 0;
        for (int i = 0; i < ADC_SAMPLES; i++) {
            adc_reading += adc1_get_raw(ADC1_CHANNEL_5);
        }
        adc_reading /= ADC_SAMPLES;

        int cur = float(adc_reading)/11.372f;
        //printf("%u %.1f %.1f\n", adc_reading, cur, baseTarget);
        const int diff = iabs(cur - baseTarget);
        int pwr = 0;
        if(diff > 100) {
            pwr = 100;
        } else if(diff > 20) {
            pwr = 100;
        } else if(diff > 5) {
            pwr = 80;
        }
        if(baseTarget < cur)
            pwr = -pwr;
        printf("set %d %d %d\n", cur, baseTarget, pwr);
        man.setMotors().power(rb::MotorId::M2, pwr).set();
        return true;
    });*/

    int i = 0;
    while(true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if(prot.is_possessed()) {
            // Send text to the android application
            prot.send_log("Tick #%d, battery at %d%%, %dmv\n", i++, batt.pct(), batt.voltageMv());
        }
    }
}
