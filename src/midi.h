#include <usbh_midi.h>
#include <usbhub.h>

namespace Midi {

    class Message {
    public:
        uint8_t* rawData;
        Message(uint8_t* _rawData): rawData(_rawData) {};
        Message() {};
        ~Message() { delete rawData; };
        static Message Parse(uint8_t* rawData, uint8_t length);
    };

    class ProgramChange : public Message {
    public:
        uint8_t program = 0;
        ProgramChange(uint8_t* rawData);
        ProgramChange(uint8_t program);
    };


    class Midi {
        USBH_MIDI* midi;
        USB* usb;
        MidiSysEx sysExData;
    public:
        Midi(USB* usb): midi(new USBH_MIDI(usb)), usb(usb) {};
        ~Midi() { delete midi; }
        void Setup();
        Message Parse(uint8_t* rawData, uint8_t length);
        void Poll();
        void Send(Message msg);
        bool Ready();
    };
}
