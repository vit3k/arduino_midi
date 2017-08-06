#include <Arduino.h>
#include "midi.h"
#include <EEPROM.h>
#include <Bounce2.h>

uint8_t getDeviceName(uint8_t addr, char* name, uint8_t &nameLength);

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

bool currentState[MAX_MIDI_DEVICES] = {false, false};

void updateState() {
    if (midi1.getAddress() != 0 && !currentState[0]) {
        currentState[0] = true;
        char* name;
        uint8_t nameLength;
        getDeviceName(midi1.getAddress(), name, nameLength);
        Serial.write(name, nameLength);
    }
}

void loop() {
    switch1.update();
    switch2.update();
    switch3.update();

    usb.Task();
    if ( usb.getUsbTaskState() == USB_STATE_RUNNING )
    {
        updateState();
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

//  function to get single string description
uint8_t getstrdescr( uint8_t addr, uint8_t idx, char* string)
{
  uint8_t buf[ 256 ];
  uint8_t rcode;
  uint8_t length;
  uint8_t i;
  uint16_t langid;
  rcode = usb.getStrDescr( addr, 0, 1, 0, 0, buf );  //get language table length
  if ( rcode ) {
    return rcode;
  }
  length = buf[0];      //length is the first byte
  rcode = usb.getStrDescr( addr, 0, length, 0, 0, buf );  //get language table
  if ( rcode ) {
    return rcode;
  }
  langid = (buf[3] << 8) | buf[2];
  rcode = usb.getStrDescr( addr, 0, 1, idx, langid, buf );
  if ( rcode ) {
    return rcode ;
  }
  length = buf[ 0 ];
  rcode = usb.getStrDescr( addr, 0, length, idx, langid, buf );
  if ( rcode ) {
    return rcode;
  }
  string = new char[length/2 - 2];
  for ( i = 2; i < length; i += 2 ) {   //string is UTF-16LE encoded
    string[i/2 - 2] = (char) buf[i];
  }
  string[i/2 - 2] = (char)0;
  return ( rcode );
}

uint8_t getDeviceName(uint8_t addr, char* name) {
    USB_DEVICE_DESCRIPTOR buf;
    uint8_t rcode;
    rcode = usb.getDevDescr(addr, 0, DEV_DESCR_LEN, ( uint8_t *)&buf);
    if ( rcode ) {
      return rcode;
    }
    char* manufacturer;
    char* product;

    if ( buf.iManufacturer > 0 ) {
        getstrdescr( addr, buf.iManufacturer, manufacturer );   // get manufacturer string
    }
    if ( buf.iProduct > 0 ) {
        getstrdescr( addr, buf.iProduct, product );   // get manufacturer string
    }
    uint8_t i = 0;

    name = new char[manufacturerLength + productLength];
    for(; i < manufacturerLength; i++) {
        name[i] = manufacturer[i];
    }

    for(; i < productLength; i++) {
        name[i] = product[i];
    }

    return 0;
}
