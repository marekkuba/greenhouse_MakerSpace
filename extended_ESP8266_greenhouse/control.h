#pragma once
#include "mappings.h"  // for Binding struct

// This is the Optimized Object
struct RuntimeBinding {
    ParamBinding config;      // Copy of the configuration (Pins, logic type, etc.)
    Parameter* target;   // Direct memory address of the Parameter in the Model
};

// The list that the loop will actually use
extern std::vector<RuntimeBinding> activeBindings;

// Functions
void resolveBindings();   // Call this ONCE in setup()
void readSensors();       // Call this in loop()
void runControlLogic();   // Call this in loop()