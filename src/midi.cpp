#include "midi.h"
#include <usbhub.h>
#include <usbh_midi.h>

namespace Midi {
    void Midi::Setup() {

    }

    ProgramChange::ProgramChange(uint8_t* rawData) {
        this->rawData = rawData;
        program = rawData[1];
    }

    ProgramChange::ProgramChange(uint8_t program) {
        this->program = program;
        rawData = new uint8_t[2];
        rawData[0] = 0b11000000;
        rawData[1] = program & 0b01111111;
    }


    Message Message::Parse(uint8_t* rawData, uint8_t length) {
        switch( (rawData[0] & 0xF0) >> 4) {
            case 0b1100:
            return ProgramChange(rawData);
        }
        return nullptr;
    }

    void Midi::Poll() {
            uint16_t bytesRcvd = 0;
            uint8_t size = 0;

            uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
            uint8_t readPtr = 0;
            uint8_t status = midi->RecvData(&bytesRcvd, recvBuf);

            if (status != 0) return;

            uint8_t* p = recvBuf;
            while (readPtr < MIDI_EVENT_PACKET_SIZE)  {
                if (*p == 0 && *(p + 1) == 0) break; //data end

                MidiSysEx::Status rc = sysExData.set(p);
                switch (rc) {
                case MidiSysEx::nonsysex :  //No SysEx message send data to Serial MIDI
                    p++;
                    size = midi->lookupMsgSize(*p);
                    //print_hex(p, size);
                    //messageQueue.push(MidiMessage::Parse(p, size));
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

    void Midi::Send(Message msg) {
            midi->SendData(msg.rawData);
    }
}
