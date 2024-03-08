// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

// App definitions
#define APP_MAIN
#include "app.h"

// Arduino entry point
void setup()
{

	// Trace
    pinMode(WIRING_LED, OUTPUT);
    digitalWrite(WIRING_LED, HIGH);

    // Initialize debug IO, noting that if USB is plugged in debug will remain NULL
#ifndef NODEBUG
    debug.begin(115200);
	uint32_t expires = millis() + 2500;
    while (!debug && millis() < expires);
    debug.printf("*** %s ***\n", appVersion());
#endif

    // Initialize the RTOS support (see NoteRTOS.h)
    _setup();

    // Initialize I2C for the display
    Wire.begin();
	
    // Initialize Notecard library (without doing any I/O on this task)
#ifndef NODEBUG
    notecard.setDebugOutputStream(debug);
#endif
	notecard.setFnNoteMutex(_lock_note, _unlock_note);
	notecard.setFnI2cMutex(_lock_wire, _unlock_wire);

	// Initialize the notecard transport
#ifdef SERIAL_NOTECARD
    notecard.begin(notecardSerial, 9600);
#else
    notecard.begin();
#endif

    // Initialize the app and create tasks as needed
    appSetup();

	// Start the scheduler
	_setup_completed();

}

// Not used in freertos (and must not block)
void loop()
{
}
