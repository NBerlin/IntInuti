#include "frontpanel.hh"

CDLEDTimer::CDLEDTimer(Duration blinkPeriod){
this->startPeriodicTimer(blinkPeriod);
}
void CDLEDTimer::timerNotify(){
Frontpanel::notifyLedEvent(cdLedId);
}
