#include <Arduino.h>
#include "midi.h"
#include <EEPROM.h>
#include <Bounce2.h>
#include "pgmstrings.h"

USB usb;
USBHub hub1(&usb);
Midi::Midi midi1(&usb);
Midi::Midi midi2(&usb);
Bounce switch1 = Bounce();
Bounce switch2 = Bounce();
Bounce switch3 = Bounce();

/* Print a string from Program Memory directly to save RAM */
void printProgStr(const char* str)
{
  char c;
  if (!str) return;
  while ((c = pgm_read_byte(str++)))
    Serial.print(c);
}

/* prints hex numbers with leading zeroes */
// copyright, Peter H Anderson, Baltimore, MD, Nov, '07
// source: http://www.phanderson.com/arduino/arduino_display.html
void print_hex(int v, int num_places)
{
  int mask = 0, n, num_nibbles, digit;

  for (n = 1; n <= num_places; n++) {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask; // truncate v to specified number of places

  num_nibbles = num_places / 4;
  if ((num_places % 4) != 0) {
    ++num_nibbles;
  }
  do {
    digit = ((v >> (num_nibbles - 1) * 4)) & 0x0f;
    Serial.print(digit, HEX);
  }
  while (--num_nibbles);
}

//  function to get single string description
uint8_t getstrdescr( uint8_t addr, uint8_t idx )
{
  uint8_t buf[ 256 ];
  uint8_t rcode;
  uint8_t length;
  uint8_t i;
  uint16_t langid;
  rcode = usb.getStrDescr( addr, 0, 1, 0, 0, buf );  //get language table length
  if ( rcode ) {
    Serial.println("Error retrieving LangID table length");
    return ( rcode );
  }
  length = buf[ 0 ];      //length is the first byte
  rcode = usb.getStrDescr( addr, 0, length, 0, 0, buf );  //get language table
  if ( rcode ) {
    Serial.print("Error retrieving LangID table ");
    return ( rcode );
  }
  langid = (buf[3] << 8) | buf[2];
  rcode = usb.getStrDescr( addr, 0, 1, idx, langid, buf );
  if ( rcode ) {
    Serial.print("Error retrieving string length ");
    return ( rcode );
  }
  length = buf[ 0 ];
  rcode = usb.getStrDescr( addr, 0, length, idx, langid, buf );
  if ( rcode ) {
    Serial.print("Error retrieving string ");
    return ( rcode );
  }
  for ( i = 2; i < length; i += 2 ) {   //string is UTF-16LE encoded
    Serial.print((char) buf[i]);
  }
  return ( rcode );
}

// function to get all string descriptors
uint8_t getallstrdescr(uint8_t addr)
{
  uint8_t rcode = 0;
  usb.Task();
  if ( usb.getUsbTaskState() >= USB_STATE_CONFIGURING ) { // state configuring or higher
    USB_DEVICE_DESCRIPTOR buf;
    rcode = usb.getDevDescr( addr, 0, DEV_DESCR_LEN, ( uint8_t *)&buf );
    if ( rcode ) {
      return ( rcode );
    }
    Serial.println("String Descriptors:");
    if ( buf.iManufacturer > 0 ) {
      Serial.print("Manufacturer:\t\t");
      rcode = getstrdescr( addr, buf.iManufacturer );   // get manufacturer string
      if ( rcode ) {
        Serial.println( rcode, HEX );
      }
      Serial.print("\r\n");
    }
    if ( buf.iProduct > 0 ) {
      Serial.print("Product:\t\t");
      rcode = getstrdescr( addr, buf.iProduct );        // get product string
      if ( rcode ) {
        Serial.println( rcode, HEX );
      }
      Serial.print("\r\n");
    }
    if ( buf.iSerialNumber > 0 ) {
      Serial.print("Serial:\t\t\t");
      rcode = getstrdescr( addr, buf.iSerialNumber );   // get serial string
      if ( rcode ) {
        Serial.println( rcode, HEX );
      }
      Serial.print("\r\n");
    }
  }
  return rcode;
}

uint8_t getdevdescr( uint8_t addr, uint8_t &num_conf )
{
  USB_DEVICE_DESCRIPTOR buf;
  uint8_t rcode;
  rcode = usb.getDevDescr( addr, 0, DEV_DESCR_LEN, ( uint8_t *)&buf );
  if ( rcode ) {
    return ( rcode );
  }
  printProgStr(Dev_Header_str);
  printProgStr(Dev_Length_str);
  print_hex( buf.bLength, 8 );
  printProgStr(Dev_Type_str);
  print_hex( buf.bDescriptorType, 8 );
  printProgStr(Dev_Version_str);
  print_hex( buf.bcdUSB, 16 );
  printProgStr(Dev_Class_str);
  print_hex( buf.bDeviceClass, 8 );
  printProgStr(Dev_Subclass_str);
  print_hex( buf.bDeviceSubClass, 8 );
  printProgStr(Dev_Protocol_str);
  print_hex( buf.bDeviceProtocol, 8 );
  printProgStr(Dev_Pktsize_str);
  print_hex( buf.bMaxPacketSize0, 8 );
  printProgStr(Dev_Vendor_str);
  print_hex( buf.idVendor, 16 );
  printProgStr(Dev_Product_str);
  print_hex( buf.idProduct, 16 );
  printProgStr(Dev_Revision_str);
  print_hex( buf.bcdDevice, 16 );
  printProgStr(Dev_Mfg_str);
  print_hex( buf.iManufacturer, 8 );
  printProgStr(Dev_Prod_str);
  print_hex( buf.iProduct, 8 );
  printProgStr(Dev_Serial_str);
  print_hex( buf.iSerialNumber, 8 );
  printProgStr(Dev_Nconf_str);
  print_hex( buf.bNumConfigurations, 8 );
  num_conf = buf.bNumConfigurations;
  return ( 0 );
}

void PrintAllDescriptors(UsbDevice *pdev)
{
  Serial.println("\r\n");
  print_hex(pdev->address.devAddress, 8);
  Serial.println("\r\n--");
  getallstrdescr(pdev->address.devAddress);
  uint8_t numConf;
  getdevdescr( pdev->address.devAddress, numConf );
}

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
unsigned long lastCheckDevices = millis();
void loop() {
    //midi1.Poll();
    switch1.update();
    switch2.update();
    switch3.update();

    usb.Task();
    if ( usb.getUsbTaskState() == USB_STATE_RUNNING )
    {


        unsigned long current = millis();
        if (current - lastCheckDevices > 5000) {
            //usb.ForEachUsbDevice(&PrintAllDescriptors);
            lastCheckDevices = current;
        }
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
