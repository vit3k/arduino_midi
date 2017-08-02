#include <Arduino.h>
#include "midi.h"
#include <EEPROM.h>
#include <Bounce2.h>

#define MAX_MIDI_DEVICES 2

struct Patch {
    uint8_t programs[MAX_MIDI_DEVICES];
};

struct Patch patches[40];

USB usb;
USBHub hub1(&usb);
Midi::Midi midi1(&usb);
Midi::Midi midi2(&usb);
Bounce switch1 = Bounce();
Bounce switch2 = Bounce();
Bounce switch3 = Bounce();

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
        Serial.println(midi1.getAddress());
        if (midi1.getAddress() != 0) {
            Midi::ProgramChangeMessage pc1(patches[patch].programs[0]);
            midi1.send(pc1);
        }

        Serial.println(midi2.getAddress());
        if (midi2.getAddress() != 0) {
            Midi::ProgramChangeMessage pc2(patches[patch].programs[1]);
            midi2.send(pc2);
        }

        currentPatch = patch;
        printPatch(currentPatch);
    }
}
bool editMode = false;

void setup() {
    Serial.begin(115200);
    delay(200);
    for(int i = 0; i < 5; i++) {
        EEPROM.put(i * sizeof(Patch), Patch{i, 4-i});
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

    Serial.println(midi1.getAddress());
    Serial.println(midi2.getAddress());
}

bool initialized = false;
uint8_t initCount = 0;

void loop() {
    //midi1.Poll();
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
                initialized = true;
                lastSwitch = current;
                initCount++;
            }
        }

        else {
            if (switch2.read() == HIGH && switch3.read() == HIGH && (current - lastSwitch) > 1000) {
                editMode = !editMode;
                lastSwitch = current;
                Serial.print("Edit mode: ");
                Serial.println(editMode);
            }
            else if (editMode && (
                    (switch1.read() == HIGH && switch2.read() == LOW && switch3.read() == LOW)
                    || (switch1.read() == LOW && switch2.read() == HIGH && switch3.read() == LOW)
                    || (switch1.read() == LOW && switch2.read() == LOW && switch3.read() == HIGH)
                    )) {
                if (switch3.rose()) {
                    currentPatch++;
                    if (currentPatch > 2) {
                        currentPatch = 0;
                    }
                    printPatch(currentPatch);
                }
                if (switch2.rose()) {
                    auto patch = patches[currentPatch].programs[0];
                    patch++;
                    if (patch > 4) {
                        patch = 0;
                    }
                    patches[currentPatch].programs[0] = patch;
                    printPatch(currentPatch);
                }
                if (switch1.rose()) {
                    EEPROM.put(currentPatch * sizeof(Patch), patches[currentPatch]);
                    printPatch(currentPatch);
                    Serial.println("Stored");
                }
            }
            else if (!editMode
                && (
                    (switch1.read() == HIGH && switch2.read() == LOW && switch3.read() == LOW)
                    || (switch1.read() == LOW && switch2.read() == HIGH && switch3.read() == LOW)
                    || (switch1.read() == LOW && switch2.read() == LOW && switch3.read() == HIGH)
                    )
                )
            {
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
}
