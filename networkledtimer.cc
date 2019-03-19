#include "frontpanel.hh"

NetworkLEDTimer::NetworkLEDTimer(Duration blinktime){
    myBlinkTime = blinktime;
}

void NetworkLEDTimer::start(){
    this->timeOutAfter(myBlinkTime);
}

void NetworkLEDTimer::timeOut(){
    Frontpanel::notifyLedEvent(networkLedId);
}
