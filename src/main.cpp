
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Wire.h"

#include "si5351.h"


#define GETUINT64(_p,_d,_r,_e) _p = strtok(NULL, _d);_r = strtoull(_p, NULL, 10);if(_r==0L) goto _e


Si5351 si5351;
USBSerial usb;
uint64_t lastUpdate;
uint64_t delayMs;
uint64_t fromFreq;
uint64_t toFreq;
uint64_t stepFreq;
uint64_t oldFreq;
int onOff;
char command[256];
int pos;

const char *delimiters  = ",;";
const char *defaultError = "available: freq, sweep";
const char *freqError = "freq,<frequency>";
const char *sweepError = "sweep,<from>,<to>,<step>,<delay>";


uint64_t calculateFreq();

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
    uint64_t f;
    char *pch;
    const char *usage;
    uint64_t rl,rl1,rl2,rl3;

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
                    GETUINT64(pch,delimiters,rl,error);    //fromFreq

                    fromFreq = rl;
                    toFreq = rl;
                    stepFreq = 0L;
                    delayMs = 1000L;
                    lastUpdate = 0L; // force update

                    usb.print("frequency: ");
                    usb.println(fromFreq);

                } else if(strcmp(pch,"sweep")==0) {
                    usage = sweepError;

                    GETUINT64(pch,delimiters,rl,error);    //fromFreq
                    GETUINT64(pch,delimiters,rl1,error);   //toFreq
                    GETUINT64(pch,delimiters,rl2,error);   //stepFreq
                    GETUINT64(pch,delimiters,rl3,error);   //delayMs

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
        usb.print(f);
        usb.print(',');
        usb.println(analogRead(0));
    }
}

uint64_t calculateFreq() {
    uint64_t nf = oldFreq + stepFreq;
    if (nf > toFreq) nf = toFreq;
    if (nf < fromFreq) nf = fromFreq;
    return (nf);
}