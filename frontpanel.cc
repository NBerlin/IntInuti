/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

#define NET_LED 0x8
#define CD_LED 0x20
#define STATUS_LED 0x10

/****************** FrontPanel DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//

byte LED::writeOutRegisterShadow = 0x38;

FrontPanel::FrontPanel():
 Job(),
 mySemaphore(Semaphore::createQueueSemaphore("FP",0)),
 myNetworkLED(networkLedId),
 myCDLED(cdLedId),
 myStatusLED(statusLedId)
{
  //this->myNetworkLED = new LED::LED((byte)0B00001000);
  //this->myCDLED = new LED::LED((byte)0B00100000);
  //this->myStatusLED = new LED::LED((byte)0B00010000);

  //this->netLedEvent, this->cdLedEvent, this->statusLedEvent = false;

  cout << "Frontpanel created" << endl;
  Job::schedule(this);
}

FrontPanel::FrontPanel& FrontPanel::instance(){
  static FrontPanel::FrontPanel myInstance;
  return myInstance;
}

void FrontPanel::packetReceived() {
  myNetworkLED.on();
  //cout << "Received pack" << endl;
  myNetworkLEDTimer->start(); //INTE helt hundra på att den initats
}

void FrontPanel::notifyLedEvent(uword theLedId){
  switch(theLedId){
    case networkLedId:
      netLedEvent = true;
      mySemaphore->signal();
    break;

    case cdLedId:
      cdLedEvent = true;
      mySemaphore->signal();
    break;

    case statusLedId:
      statusLedEvent = true;
      mySemaphore->signal();
    break;

    default:
      cout << "Shit's broken yo" << endl;
    break;
  }

}
//----------------------------------------------------------------------------
//
void
FrontPanel::doit()
{
  myNetworkLEDTimer = new NetworkLEDTimer::NetworkLEDTimer(5);
  myCDLEDTimer = new CDLEDTimer::CDLEDTimer(50);
  myStatusLEDTimer = new StatusLEDTimer::StatusLEDTimer(500);
  while(true == true || false == false){
    mySemaphore->wait();
    if(cdLedEvent){
      myCDLED.toggle();
      cdLedEvent = false;
    }

    if(statusLedEvent){
      myStatusLED.toggle();
      cout << "Core " << ax_coreleft_total() << endl;
      statusLedEvent = false;
    }

    if(netLedEvent){
      myNetworkLED.off();
      netLedEvent = false;
    }
    // do shit with lights
  }
}

// Vi injicerar in resten här

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

NetworkLEDTimer::NetworkLEDTimer(Duration blinktime){
    myBlinkTime = blinktime;
}

void NetworkLEDTimer::start(){
    this->timeOutAfter(myBlinkTime);
}

void NetworkLEDTimer::timeOut(){
    FrontPanel::instance().notifyLedEvent(FrontPanel::networkLedId);
}

CDLEDTimer::CDLEDTimer(Duration blinkPeriod){
  this->timerInterval(blinkPeriod);
  this->startPeriodicTimer();
}

void CDLEDTimer::timerNotify(){
FrontPanel::instance().notifyLedEvent(FrontPanel::cdLedId);
}

StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod){
  this->timerInterval(blinkPeriod);
this->startPeriodicTimer();
}

void StatusLEDTimer::timerNotify(){
  FrontPanel::instance().notifyLedEvent(FrontPanel::statusLedId);
}

/****************** END OF FILE FrontPanel.cc ********************************/
