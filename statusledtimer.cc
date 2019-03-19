#include "frontpanel.hh"

StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod){
this->startPeriodicTimer(blinkPeriod);
}
StatusLEDTimer::timerNotify(){
  Frontpanel::notifyLedEvent(statusLedId);
}
