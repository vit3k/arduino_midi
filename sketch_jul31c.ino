#include <Arduino.h>

#include <QueueArray.h>

#include <usbh_midi.h>
#include <usbhub.h>

#define USBH_MIDI_SYSEX_ENABLE


USB Usb;
USBH_MIDI  Midi(&Usb);
MidiSysEx sysExData;

enum MidiType {
  ProgramChange, SysEx
};

class MidiMessage {
public:
  uint8_t* rawData;
  MidiMessage(uint8_t* rawData): rawData(rawData) {};
  MidiMessage() {};
  ~MidiMessage() { delete rawData; };
  static MidiMessage Parse(uint8_t* rawData, uint8_t length);
};

class MidiProgramChange : public MidiMessage {
public:
  uint8_t program = 0;
  MidiProgramChange(uint8_t* rawData);
  MidiProgramChange(uint8_t program);
};


class MidiSysExMessage : public MidiMessage {
public:
  uint8_t length;
};

MidiMessage MidiMessage::Parse(uint8_t* rawData, uint8_t length) {
  switch( (rawData[0] & 0xF0) >> 4) {
    case 0b1100:
      return MidiProgramChange(rawData);
  }
}
MidiProgramChange::MidiProgramChange(uint8_t* rawData) {
  this->rawData = rawData;
  program = rawData[1];
}

MidiProgramChange::MidiProgramChange(uint8_t program) {
  this->program = program;
  rawData = new uint8_t[2];
  rawData[0] = 0b11000000;
  rawData[1] = program & 0b01111111;
}
void doDelay(uint32_t t1, uint32_t t2, uint32_t delayTime);

QueueArray<MidiMessage> messageQueue;

void print_hex(uint8_t* buffer, uint8_t size) {
  for(uint8_t i = 0; i < size; i++) {
    if (buffer[i] < 0x10) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.print("\n");
}
void poll() {
  uint16_t bytesRcvd = 0;
  uint8_t size = 0;

  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t readPtr = 0;
  uint8_t status = Midi.RecvData(&bytesRcvd, recvBuf);

  if (status != 0) return;

  uint8_t* p = recvBuf;
  while (readPtr < MIDI_EVENT_PACKET_SIZE)  {
    if (*p == 0 && *(p + 1) == 0) break; //data end

    MidiSysEx::Status rc = sysExData.set(p);
    switch (rc) {
      case MidiSysEx::nonsysex :  //No SysEx message send data to Serial MIDI
        p++;
        size = Midi.lookupMsgSize(*p);
        //print_hex(p, size);
        messageQueue.push(MidiMessage::Parse(p, size));
        p += 3;
        break;
      case MidiSysEx::done :      //SysEx end. send data to Serial MIDI
        //resp = sysExData.get();
        //size = sysExData.getSize();
        /* FALLTHROUGH */
      case MidiSysEx::overflow :  //SysEx buffer over. ignore and flush buffer.
        sysExData.clear();
        /* FALLTHROUGH */
      default:
        p += 4;
        break;
    }
    readPtr += 4;
  }
}

unsigned long lastSwitch = 0;
void setup() {
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
  lastSwitch = millis();
}

uint8_t channel = 0;
int stage = 0;


void loop() {

  uint8_t msg[4];

  Usb.Task();

  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING )
  {
    poll();

    unsigned long current = millis();
    if ( (current - lastSwitch) >= 2000) {
      MidiProgramChange pc(channel);
      Midi.SendData(pc.rawData);
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
