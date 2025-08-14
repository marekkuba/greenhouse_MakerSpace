#include "scheduler.h"
#include "globals.h"

bool timeToPublish() {
    unsigned long now = millis();
    if (now - previousMillis >= PUBLISH_INTERVAL) {
        previousMillis = now;
        return true;
    }
    return false;
}