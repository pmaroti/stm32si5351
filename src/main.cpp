
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Wire.h"

#include "si5351.h"


#define GETLONG(_p,_d,_r,_e) _p = strtok(NULL, _d);_r = atol(_p);if(_r==0L) goto _e


Si5351 si5351;
USBSerial usb;
unsigned long lastUpdate;
unsigned long delayMs;
unsigned long fromFreq;
unsigned long toFreq;
unsigned long stepFreq;
unsigned long oldFreq;
int onOff;
char command[256];
int pos;

const char *delimiters  = ",;";
const char *defaultError = "available: freq, sweep";
const char *freqError = "freq,<frequency>";
const char *sweepError = "sweep,<from>,<to>,<step>,<delay>";


unsigned long calculateFreq();

void setup() {
    bool i2c_found;
    int i;

    // Set up the built-in LED pin as an output:
    pinMode(0, INPUT_ANALOG);
    pinMode(PC13, OUTPUT);
    usb.begin(9600);
    usb.setTimeout(100000L);

    for(i=0;i<3;i++) {
        usb.println("Initialization");
        delay(1000);
    }

    i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if(!i2c_found) {
        while(1) {
            usb.println("Device not found on I2C bus!");
            delay(2000);
        }
    }
    usb.println("Device found!");
    
    // Set CLK0 to output 
    si5351.set_freq(1400000000ULL, SI5351_CLK0);
    lastUpdate = millis();

    oldFreq = 1400000000ULL;
    fromFreq = 1400000000ULL;
    toFreq = 1400000000ULL;
    stepFreq = 0L;
    delayMs=500L;
    
    // Query a status update and wait a bit to let the Si5351 populate the
    // status flags correctly.
    si5351.update_status();
    onOff=0;
    pos=0;
}

void loop() {
    unsigned long f;
    char *pch;
    const char *usage;
    unsigned long rl,rl1,rl2,rl3;

    while (usb.available()) {
        char c = usb.read();
        switch (c) {
            case '\n':
                command[pos]='\n';
                usb.print("received command: ");
                usb.println(command);

                pch = strtok(command, delimiters);

                usb.print("function: ");
                usb.println(pch);

                usage=defaultError;

                if(strcmp(pch,"freq")==0) {
                    usage = freqError;
                    GETLONG(pch,delimiters,rl,error);    //fromFreq

                    fromFreq = rl;
                    toFreq = rl;
                    stepFreq = 0L;
                    delayMs = 1000L;
                    lastUpdate = 0L; // force update

                    usb.print("frequency: ");
                    usb.println(fromFreq);

                } else if(strcmp(pch,"sweep")==0) {
                    usage = sweepError;

                    GETLONG(pch,delimiters,rl,error);    //fromFreq
                    GETLONG(pch,delimiters,rl1,error);   //toFreq
                    GETLONG(pch,delimiters,rl2,error);   //stepFreq
                    GETLONG(pch,delimiters,rl3,error);   //delayMs

                    if(rl>rl1) {
                        goto error;
                    }

                    fromFreq = rl;
                    toFreq = rl1;
                    stepFreq = rl2;
                    delayMs = rl3;

                    usb.print("from:");
                    usb.println(fromFreq);
                    
                    usb.print("to:");
                    usb.println(toFreq);

                    usb.print("step:");
                    usb.println(stepFreq);

                    usb.print("delay:");
                    usb.println(delayMs);

                } else {
error:
                    usb.println("ERROR");
                    usb.println(usage);
                }
                pos=0;
                break;
            default:
                command[pos++]=c;
                break;
        }
    }

    if((millis()-lastUpdate) > delayMs) {
        lastUpdate = millis();

        usb.print(oldFreq);
        usb.print(',');
        usb.println(analogRead(0));

        f = calculateFreq();
        if(f != oldFreq) {
            si5351.set_freq(f, SI5351_CLK0);
            oldFreq = f;
        }

        if(onOff==0) {
            digitalWrite(PC13,1);
            onOff=1;
        } else {
            digitalWrite(PC13,0);
            onOff=0;
        }
    }
}

unsigned long calculateFreq() {
    unsigned long nf = oldFreq + stepFreq;
    if (nf > toFreq) nf = toFreq;
    if (nf < fromFreq) nf = fromFreq;
    return (nf);
}


 /*
// --------------------------------------
// i2c_scanner
//
// Version 1
//    This program (or code that looks like it)
//    can be found in many places.
//    For example on the Arduino.cc forum.
//    The original author is not know.
// Version 2, Juni 2012, Using Arduino 1.0.1
//     Adapted to be as simple as possible by Arduino.cc user Krodal
// Version 3, Feb 26  2013
//    V3 by louarnold
// Version 4, March 3, 2013, Using Arduino 1.0.3
//    by Arduino.cc user Krodal.
//    Changes by louarnold removed.
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
// Version 5, March 28, 2013
//    As version 4, but address scans now to 127.
//    A sensor seems to use address 120.
// 
//
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
//

#include <Wire.h>

USBSerial usb;


void setup()
{
  int i;
  Wire.begin(1);

  usb.begin(9600);

  for(i=0;i<35;i++) {
    usb.println("Initialization");
    delay(1000);
  }
  usb.println("\nI2C Scanner");
}


void loop()
{
  byte error, address;
  int nDevices;

  usb.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    usb.print("Testing address ");
    usb.println(address);
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      usb.print("I2C device found at address 0x");
      if (address<16) 
        usb.print("0");
      usb.print(address,HEX);
      usb.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      usb.print("Unknow error at address 0x");
      if (address<16) 
        usb.print("0");
      usb.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    usb.println("No I2C devices found\n");
  else
    usb.println("done\n");

  delay(5000);           // wait 5 seconds for next scan
}

*/