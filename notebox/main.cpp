// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"

// AUX serial, for receiving notifications
bool notecardAuxInitialized = false;
bool consoleUARTInitialized = false;
bool consoleUSBInitialized = false;
HardwareSerial notecardAux(WIRING_RX_FM_NOTECARD_AUX_TX, WIRING_TX_TO_NOTECARD_AUX_RX);
HardwareSerial consoleUART(WIRING_RX_FM_CONSOLE_TX, WIRING_TX_TO_CONSOLE_RX);
#ifdef NODEBUG
HardwareSerial consoleUSB(WIRING_RX_FM_CONSOLE_TX, WIRING_TX_TO_CONSOLE_RX);
#endif

// Environment handling
int64_t environmentModifiedTime = 0;
bool refreshEnvironmentVars(void);
void updateEnvironment(J *body);

// Main task for the app
void mainTask(void *param)
{
    (void) param;

	// Initialize console ports.  We'll treat the console and USB as equals
    consoleUSB.begin(CONSOLE_USB_SPEED);
	uint32_t expires = millis() + 2500;
    while (!consoleUSB && millis() < expires);
	if (consoleUSB) {
		consoleUSBInitialized = true;
	}

    consoleUART.begin(CONSOLE_UART_SPEED);
	expires = millis() + 2500;
    while (!consoleUART && millis() < expires);
	if (consoleUART) {
		consoleUARTInitialized = true;
	}

    notecardAux.begin(NOTECARD_AUX_SPEED);
	expires = millis() + 2500;
    while (!notecardAux && millis() < expires);
	if (notecardAux) {
		notecardAuxInitialized = true;
	}
	
    // Subscribe to inbound notifications
    J *req = NoteNewRequest("card.aux.serial");
	JAddStringToObject(req, "mode", "notify,signals,env,motion,file");
    if (!notecard.sendRequest(req)) {
		debugf("can't subscribe to notifications from notecard\n");
		while (true) {
			appSignal(5);
		}
    }

    // Load the environment vars for the first time
    refreshEnvironmentVars();

	// Loop while debugging, sending a web request to the notehub
	while (true) {
		static int count = 0;

		// Send a JSON request
		J *body = JCreateObject();
		JAddIntToObject(body, "count", ++count);
	    J *req = NoteNewCommand("web.post");
		JAddStringToObject(req, "content", "application/json");
		JAddStringToObject(req, "route", NOTEHUB_ROUTE_ALIAS);
		JAddBoolToObject(req, "live", true);
		JAddIntToObject(req, "seconds", 2);
		JAddItemToObject(req, "body", body);
		if (!notecard.sendRequest(req)) {
			debugf("web request failure\n");
		}

		// Delay
		_delay(15000);

	}

}

// Re-load all env vars, returning the modified time
bool refreshEnvironmentVars()
{

    // Read all env vars from the notecard in one transaction
    J *rsp = notecard.requestAndResponse(notecard.newRequest("env.get"));
    if (rsp == NULL) {
        return false;
    }
    if (notecard.responseError(rsp)) {
        notecard.deleteResponse(rsp);
        return false;
    }

    // Update the env modified time
    environmentModifiedTime = JGetNumber(rsp, "time");

    // Update the environment
    J *body = JGetObject(rsp, "body");
    if (body != NULL) {
        updateEnvironment(body);
    }

    // Done
    notecard.deleteResponse(rsp);
    return true;

}

// Update the environment from the body
void updateEnvironment(J *body)
{
	(void) body;
}

// Main polling loop which performs tasks that map synchronous I/O to asynchronous
bool mainPoll(void)
{
	bool didSomething = false;

	// Do AUX processing
	if (notecardAuxInitialized) {

	    // See if we've got any data available on the serial port
	    if (notecardAux.available()) {
		    J *notification;
	        String receivedString;

	        // Receive a JSON object over the serial line
			debugf("AUX available!\n");
	        receivedString = notecardAux.readStringUntil('\n');
			debugf("GOT STRING? %d\n", receivedString != NULL);
	        if (receivedString != NULL) {

		        // Parse the JSON object
		        const char *JSON = receivedString.c_str();
		        debugf("notification: %s\n", JSON);
		        notification = JParse(JSON);
		        if (notification != NULL) {

					// Note that we did something to the caller
					didSomething = true;

				    // Get notification type, and ignore if not an "env" notification
				    const char *notificationType = JGetString(notification, "type");
					for (;;) {

						if (strEQL(notificationType, "env")) {
						    environmentModifiedTime = JGetNumber(notification, "modified");
						    J *body = JGetObject(notification, "body");
						    if (body != NULL) {
						        updateEnvironment(body);
						    }
							break;
						}

						if (strEQL(notificationType, "signal")) {
					        debugf("notify: processing '%s'\n", notificationType);
							break;
						}

						// Unrecognized
				        debugf("notify: '%s'\n", notificationType);
						break;

					}
				    JDelete(notification);
				}
			}
		}
	}

	// Do Console UART processing
	if (consoleUARTInitialized) {
		while (consoleUART.available()) {
			debugf("CONSOLE UART: '%c'\n", consoleUART.read());
		}
	}

	// Do Console USB processing
	if (consoleUSBInitialized) {
		while (consoleUSB.available()) {
			debugf("CONSOLE USB:  '%c'\n", consoleUSB.read());
		}
	}

	// Done
    return didSomething;

}
