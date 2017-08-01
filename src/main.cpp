#include <Arduino.h>
#include "midi.h"

USB usb;
USBHub hub(&usb);
Midi::Midi midi1(&usb);
Midi::Midi midi2(&usb);

void print_hex(uint8_t* buffer, uint8_t size);

unsigned long lastSwitch = 0;
void setup() {
  Serial.begin(115200);

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
    //midi2.Poll();
  usb.Task();
  if ( usb.getUsbTaskState() == USB_STATE_RUNNING )
  {
    unsigned long current = millis();
    if ( (current - lastSwitch) >= 2000) {
      Midi::ProgramChange pc(channel);
      midi1.Send(pc);
      midi2.Send(pc);
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

void print_hex(uint8_t* buffer, uint8_t size) {
  for(uint8_t i = 0; i < size; i++) {
    if (buffer[i] < 0x10) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.print("\n");
}
