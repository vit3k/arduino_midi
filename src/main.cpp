#include <Arduino.h>
#include "midi.h"
#include <EEPROM.h>

#define MAX_MIDI_DEVICES 2

struct Patch {
    uint8_t programs[MAX_MIDI_DEVICES];
};

struct Patch patches[40];

USB usb;
Midi::Midi midi1(&usb);

unsigned long lastSwitch = 0;
void setup() {
    Serial.begin(115200);

    delay(200);
    for(int i = 0; i < 40; i++) {
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

    lastSwitch = millis();
}

uint8_t channel = 0;
int stage = 0;

void loop() {

    //midi1.Poll();
    usb.Task();
    if ( usb.getUsbTaskState() == USB_STATE_RUNNING )
    {
        unsigned long current = millis();
        if ( (current - lastSwitch) >= 2000) {
            Midi::ProgramChangeMessage pc1(patches[channel].programs[0]);
            midi1.send(pc1);
            //midi2.Send(pc);
            channel++;
            if (channel >= 5)
            {
                channel = 0;
            }
            lastSwitch = current;
        }
    }

    /*Usb.Task();
    if ( Usb.getUsbTaskState() == USB_STATE_RUNNING )
    {
    if (stage == 0) {
    uint8_t outBuf[64];
    uint8_t msg1[6] = { 0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7 };
    Midi.SendSysEx(msg1, 6);
    delay(200);
    uint8_t size = receive(outBuf);
    print_hex(outBuf, size);
    stage++;
}
else if (stage == 1) {
//uint8_t outBuf[64];
//uint8_t size = receive(outBuf);
//print_hex(outBuf, size);
}
}*/
}
