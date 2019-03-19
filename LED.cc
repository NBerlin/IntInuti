#include "frontpanel.hh"

LED::LED(byte theLedNumber){
    // Skapa LED
    myLedBit = 1 << (2 + theLedNumber);
    iAmOn = false;
}

void LED::off(){
    writeOutRegisterShadow = writeOutRegisterShadow | myLedBit;
    *(volatile byte*)0x80000000 = writeOutRegisterShadow;
    iAmOn = false;
}

void LED::on(){
    writeOutRegisterShadow = writeOutRegisterShadow & (myLedBit ^ 0xff);
    *(volatile byte*)0x80000000 = writeOutRegisterShadow;
    iAmOn = true;
}

void LED::toggle(){
    if(iAmOn){
        off();
    } else {
        on();
    }
}
