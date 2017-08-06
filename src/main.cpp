#include <Arduino.h>
#include "midi.h"
#include <EEPROM.h>
#include <Bounce2.h>

USB usb;
USBHub hub1(&usb);
Midi::Midi midi1(&usb);
Midi::Midi midi2(&usb);
Bounce switch1 = Bounce();
Bounce switch2 = Bounce();
Bounce switch3 = Bounce();

#define MAX_MIDI_DEVICES 2

struct Patch {
    uint8_t programs[MAX_MIDI_DEVICES];
};

struct Patch patches[40];

unsigned long lastSwitch = 0;

void printPatch(uint8_t patchNumber) {
    Serial.print("Patch: ");
    Serial.print(patchNumber);
    Serial.print(" - ");
    Serial.print(patches[patchNumber].programs[0]);
    Serial.print(", ");
    Serial.println(patches[patchNumber].programs[1]);
}
uint8_t currentPatch = 255;
void switchPatch(uint8_t patch, bool force = false) {
    if (patch != currentPatch || force) {
        if (midi1.getAddress() != 0) {
            uint8_t port = (midi1.getPort() - 1) % 2 ;
            Midi::ProgramChangeMessage pc1(patches[patch].programs[port]);
            midi1.send(pc1);
        }

        if (midi2.getAddress() != 0) {
            uint8_t port = (midi2.getPort() - 1) % 2;
            Midi::ProgramChangeMessage pc2(patches[patch].programs[port]);
            midi2.send(pc2);
        }

        currentPatch = patch;
        printPatch(currentPatch);
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    for(int i = 0; i < 5; i++) {
        //EEPROM.put(i * sizeof(Patch), Patch{i, 4-i});
        Patch patch;
        EEPROM.get(i * sizeof(Patch), patch);
        patches[i] = patch;
        Serial.print(i);
        Serial.print(" -> ");
        Serial.print(patch.programs[0]);
        Serial.print(", ");
        Serial.println(patch.programs[1]);
    }

    if (usb.Init() == -1) {
        while (1);
    }
    delay( 200 );

    pinMode(2, INPUT);
    digitalWrite(2, HIGH);
    switch1.attach(2);
    switch1.interval(5);

    pinMode(3, INPUT);
    digitalWrite(3, HIGH);
    switch2.attach(3);
    switch2.interval(5);

    pinMode(4, INPUT);
    digitalWrite(4, HIGH);
    switch3.attach(4);
    switch3.interval(5);

    lastSwitch = millis();
}

uint8_t initCount = 0;

void loop() {
    switch1.update();
    switch2.update();
    switch3.update();

    usb.Task();
    if ( usb.getUsbTaskState() == USB_STATE_RUNNING )
    {
        unsigned long current = millis();
        if (initCount < 3 ) {
            if ((current - lastSwitch) > 1000) {
                switchPatch(0, true);
                lastSwitch = current;
                initCount++;
            }
        }
        else {
            if (switch1.rose()) {
                switchPatch(0);
            }

            if (switch2.rose()) {
                switchPatch(1);
            }

            if (switch3.rose()) {
                switchPatch(2);
            }
        }
    }
}
