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
#include "RBControl_arm.hpp"

#include "motors.hpp"

// CHANGE THESE so you can find the robot in the Android app
#define OWNER "FrantaFlinta"
#define NAME "FlusMcFlusy"

// CHANGE THESE to your WiFi's settings
#define WIFI_NAME "Technika"
#define WIFI_PASSWORD "materidouska"

//#define FAKE_ARM 1

using namespace rb;

static std::unique_ptr<Arm> buildArm() {
    ArmBuilder builder;
    builder.body(60, 110).armOffset(0, 20);

    auto b0 = builder.bone(0, 110);
    b0.relStops(-95_deg, 0_deg);
    b0.calcServoAng([](Angle absAngle, Angle) -> Angle {
        return Angle::Pi + absAngle + 30_deg;
    });
    b0.calcAbsAng([](Angle servoAng) -> Angle {
        return servoAng - Angle::Pi - 30_deg;
    });

    auto b1 = builder.bone(1, 135);
    b1.relStops(30_deg, 165_deg)
        .absStops(-20_deg, Angle::Pi)
        .baseRelStops(40_deg, 160_deg);
    b1.calcServoAng([](Angle absAngle, Angle) -> Angle {
        absAngle = Arm::clamp(absAngle + Angle::Pi*1.5);
        return Angle::Pi + absAngle + 25_deg;
    });
    b1.calcAbsAng([](Angle servoAng) -> Angle {
        auto a = servoAng - Angle::Pi - 25_deg;
        return Arm::clamp(a - Angle::Pi*1.5);
    });

    return builder.build();
}

static void sendArmInfo(Protocol& prot, const Arm::Definition& def) {
    auto info = std::make_unique<rbjson::Object>();
    info->set("height", def.body_height);
    info->set("radius", def.body_radius);
    info->set("off_x", def.arm_offset_x);
    info->set("off_y", def.arm_offset_y);

    auto *bones = new rbjson::Array();
    info->set("bones", bones);

    auto& servo = Manager::get().servoBus();
    for(const auto& b : def.bones) {
#if !FAKE_ARM
        const auto pos = servo.pos(b.servo_id);
#else
        const auto pos = Angle::rad(-3.14/2);
#endif
        if(pos.isNaN()) {
            ESP_LOGE("RBControl", "Rejecting arminfo, servo %d returned NaN position!", b.servo_id);
            return;
        }

        auto *info_b = new rbjson::Object();
        info_b->set("len", b.length);
        info_b->set("angle", b.calcAbsAng(pos).rad());
        info_b->set("rmin", b.rel_min.rad());
        info_b->set("rmax", b.rel_max.rad());
        info_b->set("amin", b.abs_min.rad());
        info_b->set("amax", b.abs_max.rad());
        info_b->set("bmin", b.base_rel_min.rad());
        info_b->set("bmax", b.base_rel_max.rad());
        bones->push_back(info_b);
    }
    prot.send_mustarrive("arminfo", info.release());
}

void setup() {
    // Initialize the robot manager
    auto& man = Manager::get();
    man.install();

    // Set the battery measuring coefficient.
    // Measure voltage at battery connector and
    // coef = voltageMeasureAtBatteriesInMilliVolts / raw
    auto& batt = man.battery();
    batt.setCoef(9.185f);

    // Connect to the WiFi network
    // If the button 1 is not pressed: connect to WIFI_NAME
    // else create an AP.
    if(man.expander().digitalRead(SW1) != 0) {
        man.leds().yellow();
        WiFi::connect(WIFI_NAME, WIFI_PASSWORD);
    } else {
        man.leds().green();
        WiFi::startAp("Flus" OWNER "-" NAME, "flusflus", 12);
    }

    // Set motor power limits
    man.setMotors()
        .pwmMaxPercent(MOTOR_LEFT, 100)
        .pwmMaxPercent(MOTOR_RIGHT, 100)
        .set();

    // Set-up arm and servos
    auto& servos = man.initSmartServoBus(3);
#if !FAKE_ARM
    servos.setAutoStop(2);
#endif
    servos.limit(0,  0_deg, 220_deg);
    servos.limit(1, 85_deg, 210_deg);
    servos.limit(2, 75_deg, 160_deg);

    auto arm = buildArm();
    bool isGrabbing = servos.pos(2).deg() < 150;

    // Start web server with control page (see data/index.html)
    man.monitorTask(rb_web_start(80));

    // Initialize the communication protocol
    Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, [&](const std::string& command, rbjson::Object *pkt) {
        //printf("Commmand %s\n", command.c_str());
        if(command == "joy") {
            motors_handle_joysticks(man, pkt);
        } else if(command == "arm") {
            const auto& b = arm->bones();
            const double x = pkt->getDouble("x");
            const double y = pkt->getDouble("y");

            bool res = arm->solve(x, y);
            printf("%f %f %d | %f %f | %f %f || %f %f | %f %f | %d %d\n", x, y, (int)res,
                b[0].absAngle.rad(), b[1].absAngle.rad(),
                b[0].absAngle.deg(), b[1].absAngle.deg(),
                b[0].servoAng().rad(), b[1].servoAng().rad(),
                b[0].servoAng().deg(), b[1].servoAng().deg(),
                b[1].x, b[1].y);
#if !FAKE_ARM
            arm->setServos(200);
#endif
        } else if(command == "grab") {
            isGrabbing = !isGrabbing;
            man.servoBus().set(2, isGrabbing ? 75_deg : 160_deg, 200.f, 1.f);
        } else if(command == "arminfo") {
            sendArmInfo(prot, arm->definition());
        }
    });

    prot.start();
    man.monitorTask(prot.getTaskRecv());
    man.monitorTask(prot.getTaskSend());

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

    float p1 = servos.pos(0).deg();
    float p2 = servos.pos(1).deg();
    float p3 = servos.pos(2).deg();
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
                servos.set(id, Angle::deg(angle), 80);
                break;
            }
        }
    }
}

void loop() {

}
