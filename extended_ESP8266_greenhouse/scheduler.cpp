#include "scheduler.h"
#include "globals.h"

bool timeToPublish() {
    unsigned long now = millis();
    if (now - previousMillis < PUBLISH_INTERVAL) {
        delay(10);
        return false;
    }
    previousMillis = now;
    return true;
}