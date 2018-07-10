# Mickoflus-demo01
This is a demo project for mickoflus. It can control the motors, turret, shoot and print some info into log.

## Structure
The mickoflus is controlled by the (RBController Android App)[https://play.google.com/store/apps/details?id=com.tassadar.rbcontroller]. It downloads the control html page from the esp, and the page is located in `data/index.html`. You can modify it freely, but remember to push the refresh button in android app after doing so.

The main esp code is in `src/main.cpp`.