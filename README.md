# Mickoflus-demo01 [![Build Status](https://travis-ci.org/RoboticsBrno/mickoflus-demo01.svg?branch=master)](https://travis-ci.org/RoboticsBrno/mickoflus-demo01)

[English version follows]

Toto je ukázkový program pro Míčkoflus. Může řídit motory, věž, střílet a vypisovat nějaké informace do logu.

### Struktura
Míčkoflus je řízen [RBController Android App](https://play.google.com/store/apps/details?id=com.tassadar.rbcontroller). Aplikace si stáhne řídící HTML stránku z ESP, která je umístěná v `data/index.html`. Můžeš ji modifikovat jak uznáš za vhodné, ale pamatuj, že potom musíš zmáčknout obnovovací tlačítko v android aplikaci.

Hlavní kód je v `src/main.cpp`.

## English version

This is a demo project for Míčkoflus. It can control the motors, turret, shoot and print some info into log.

### Structure
The Míčkoflus is controlled by the [RBController Android App](https://play.google.com/store/apps/details?id=com.tassadar.rbcontroller). It downloads the control HTML page from the esp, and the page is located in `data/index.html`. You can modify it freely, but remember to push the refresh button in android app after doing so.

The main esp code is in `src/main.cpp`.
