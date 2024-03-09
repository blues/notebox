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
//HardwareSerial consoleUSB(WIRING_RX_FM_CONSOLE_TX, WIRING_TX_TO_CONSOLE_RX);
#endif

// Environment handling
int64_t environmentModifiedTime = 0;
bool refreshEnvironmentVars(void);
void updateEnvironment(J *body);

// Forwards
void sendMessageToNotecard(const char *message);
String trimEnd(String str);
uint32_t uniqueId(void);

// The most recently received message IDs
uint32_t receivedMessageID[100] = {0};

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

    // Subscribe to inbound notifications for signals & environment.
	// Make sure the notecard knows how large our serial buffer is, so
	// that it does flow control when sending thigns back to us.  We
	// use the Arduino RX buffer size minus one because some platforms
	// there is a stall when we hit buffer full.
    J *req = NoteNewRequest("card.aux.serial");
	JAddStringToObject(req, "mode", "notify,-all,signals,env");
    JAddIntToObject(req, "max", SERIAL_RX_BUFFER_SIZE-1);
    JAddIntToObject(req, "ms", 25);
    if (!notecard.sendRequest(req)) {
		debugf("can't subscribe to notifications from notecard\n");
		while (true) {
			appSignal(5);
		}
    }

    // Load the environment vars for the first time
    refreshEnvironmentVars();

	// Do nothing.  In a more typical app, this is where the main
	// processing would go.
	while (true) {
		_delay(150000);
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
#ifndef NODEBUG
	char *jsonTemp = JPrint(body);
	if (jsonTemp != NULL) {
		debugf("%s\n", jsonTemp);
		_free(jsonTemp);
	}
#endif
}

// Main polling loop which performs tasks that map synchronous I/O to asynchronous
bool mainPoll(void)
{
	bool didSomething = false;

	// Do AUX processing
	if (notecardAuxInitialized && notecardAux.available()) {

		// Receive a JSON object over the serial line
		String receivedString = notecardAux.readStringUntil('\n');
		if (receivedString != NULL) {

			// Parse the JSON object
			const char *JSON = receivedString.c_str();
			J *notification = JParse(JSON);
			if (notification != NULL) {

				// Note that we did something to the caller
				didSomething = true;

				// Get notification type, and ignore if not an "env" notification
				const char *notificationType = JGetString(notification, "type");
				for (;;) {

					// If we've received something with a recognized command, process
					// it, else send the entire JSON to it.
					if (strEQL(notificationType, "")) {
						const char *hostCmd = JGetString(notification, "class");
						if (strEQL(hostCmd, "log")) {
							const char *message = JGetString(notification, "message");
							if (message[0] != '\0') {
								consoleUART.write(message, strlen(message));
								consoleUART.write("\n", 1);
								consoleUSB.write(message, strlen(message));
								consoleUSB.write("\n", 1);
							}
						} else {
							char *jsonTemp = NULL;
							uint32_t messageID = JGetInt(notification, "id");
							if (messageID == 0) {
								messageID = uniqueId();
								JAddIntToObject(notification, "id", messageID);
								jsonTemp = JPrint(notification);
								if (jsonTemp != NULL) {
									JSON = jsonTemp;
								}
							}
							bool assigned = false;
							for (unsigned i=0; i<(sizeof(receivedMessageID)/sizeof(receivedMessageID[0])); i++) {
								if (receivedMessageID[i] == 0 && !assigned) {
									receivedMessageID[i] = messageID;
									assigned = true;
								} else if (receivedMessageID[i] == messageID) {
									receivedMessageID[i] = 0;
								}
							}
							consoleUART.write(JSON, strlen(JSON));
							consoleUART.write("\n", 1);
							consoleUSB.write(JSON, strlen(JSON));
							consoleUSB.write("\n", 1);
							if (jsonTemp != NULL) {
								_free(jsonTemp);
							}
						}

						break;
					}

					if (strEQL(notificationType, "env")) {
						environmentModifiedTime = JGetNumber(notification, "modified");
						J *body = JGetObject(notification, "body");
						if (body != NULL) {
							updateEnvironment(body);
						}
						break;
					}

					debugf("notify: ignoring '%s'\n", notificationType);
					break;

				}
				JDelete(notification);
			}
		}
	}

	// Do Console UART processing
	if (consoleUARTInitialized && consoleUART.available()) {
        String receivedString = consoleUART.readStringUntil('\n');
		sendMessageToNotecard(trimEnd(receivedString).c_str());
		didSomething = true;
	}

	// Do Console USB processing
	if (consoleUSBInitialized && consoleUSB.available()) {
        String receivedString = consoleUSB.readStringUntil('\n');
		sendMessageToNotecard(trimEnd(receivedString).c_str());
		didSomething = true;
	}

	// Done
    return didSomething;

}

// Trim control chars from the end of the string
String trimEnd(String str) {
  int i = str.length() - 1;
  while (i >= 0 && str[i] < ' ') {
    i--;
  }
  return str.substring(0, i + 1);
}

// Send a message to the notecard
void sendMessageToNotecard(const char *message)
{
	if (message[0] == '\0') {
		return;
	}
	bool isReply = false;
	bool isJSON = (message[0] == '{');
	J *body = NULL;
	if (isJSON) {
		body = JParse(message);
		if (body == NULL) {
			isJSON = false;
		} else {
			uint32_t messageID = JGetInt(body, "id");
			if (messageID != 0) {
				for (unsigned i=0; i<(sizeof(receivedMessageID)/sizeof(receivedMessageID[0])); i++) {
					if (receivedMessageID[i] == messageID) {
						isReply = true;
						receivedMessageID[i] = 0;
						break;
					}
				}
			}
		}
	}
	if (!isJSON) {
		body = JCreateObject();
		JAddNumberToObject(body, "id", uniqueId());
		JAddStringToObject(body, "class", "log");
		JAddStringToObject(body, "message", message);
	}
	if (isReply) {
		J *req = notecard.newCommand("hub.signal");
		JAddBoolToObject(req, "live", true);
		JAddIntToObject(req, "seconds", 2);
		JAddItemToObject(req, "body", body);
		if (!notecard.sendRequest(req)) {
			debugf("signal send failure");
		}
	} else {
	    J *req = NoteNewCommand("web.post");
		JAddStringToObject(req, "content", "application/json");
		JAddStringToObject(req, "route", NOTEHUB_ROUTE_ALIAS);
		JAddBoolToObject(req, "live", true);
		JAddIntToObject(req, "seconds", 2);
		JAddItemToObject(req, "body", body);
		if (!notecard.sendRequest(req)) {
			debugf("web request failure\n");
		}
	}
}

// uniqueID uses the time to get a unique identifier, pushing
// the time forward as much as needed until the ID is unique.
uint32_t uniqueId()
{
	static uint32_t lastIssuedUniqueId = 0;
	if (NoteTimeValidST()) {
		uint32_t now = NoteTimeST();
		if (lastIssuedUniqueId < now) {
			lastIssuedUniqueId = now;
			return lastIssuedUniqueId;
		}
	}
	return ++lastIssuedUniqueId;
}

