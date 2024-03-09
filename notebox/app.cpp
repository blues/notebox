// Copyright 2022 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"

// This is a product configuration JSON structure that enables the Notehub to recognize this
// firmware when it's uploaded, to help keep track of versions and so we only ever download
// firmware builds that are appropriate for this device.
#define QUOTE(x) "\"" x "\""
#define	FIRMWARE_BUILT __DATE__ " " __TIME__
#define	FIRMWARE_VERSION_HEADER "firmware::info:"
#define FIRMWARE_VERSION FIRMWARE_VERSION_HEADER			\
    "[" QUOTE("product") ":" QUOTE(PRODUCT_DISPLAY_NAME)    \
    "," QUOTE("version") ":" QUOTE(PRODUCT_VERSION)         \
    "," QUOTE("ver_major") ":" STRINGIFY(PRODUCT_MAJOR)		\
    "," QUOTE("ver_minor") ":" STRINGIFY(PRODUCT_MINOR)		\
    "," QUOTE("ver_patch") ":" STRINGIFY(PRODUCT_PATCH)		\
    "," QUOTE("built") ":" QUOTE(FIRMWARE_BUILT)			\
    "}"

// Notecard capabilities
bool appHasGPS = false;
bool appHasCellular = false;
bool appHasWiFI = false;
bool appHasNTN = false;
bool appHasLoRa = false;

// Forwards
void appTask(void *param);
const char *notecardInit(void);

// Set up everything we can that does NOT need FreeRTOS running
bool appSetup(void)
{

    // Create the tasks
    xTaskCreate(appTask, TASKNAME_APP, TASKSTACK_APP, NULL, TASKPRI_APP, NULL);

    // Done
    return true;

}

// Worker task for the app.  This task's responsibility, in essence, is to perform
// 100% of the 'polling' duties of the app, which is required because Arduino has
// a synchronous polling model.
void appTask(void *param)
{
    (void) param;

	// Version
	appSignal(1);
    debugf(PRODUCT_VERSION "\n");

    // Show mem available
    appSignal(2);
    debugf("mem available: %d\n", NoteMemAvailable());

	// Initialize the notecard
	while (true) {
	    appSignal(3);
		const char *errstr = notecardInit();
	    if (errstr == NULL) {
			break;
		}
		debugf("%s\n", errstr);
        _delay(750);
    }
    appSignal(4);

    // Create the main task
    xTaskCreate(mainTask, TASKNAME_MAIN, TASKSTACK_MAIN, NULL, TASKPRI_MAIN, NULL);

    // Perform housekeeping and synchronous I/O related duties.  Note that if
	// there was nothing done by the poller, we take a breather to allow other
	// tasks to do their work.  This implies that there will be at least some
	// latency in beginning to do synchronous polling.
    while (true) {
        if (!mainPoll()) {
			_delay(10);
		}
    }

}

// Signal status on the LED
void appSignal(uint32_t count)
{
    for (uint32_t i=0; i<count; i++) {
        digitalWrite(WIRING_LED, HIGH);
        _delay(150);
        digitalWrite(WIRING_LED, LOW);
        _delay(150);
    }
    _delay(1000);
}

// The purpose of this method is to init the notecard and update the profile
// describing the notecard that we keep in nvram.  However, if we fail to open
// the notecard (for example, if it's corrupted) we continue by just using
// the one in nvram to establish the notecard sesion.
const char *notecardInit(void)
{
    J *req, *rsp;

	// Create a reference to the FIRMWARE_VERSION string that will
	// not be optimized away by the GCC coompiler and linker, as the
	// FIRMWARE_VERSION string MUST be present in the firmware image
	// in order to support the Notecard's firmware update mechanisms.
	// You can verify this by searching the .bin file like this:
	//   strings notebox.ino.bin | grep "firmware::info:"
	const char * volatile reference = FIRMWARE_VERSION;

    // Set hub parameters so that we are highly responsive when online, and
	// only ping occasionally when the power goes out.
    req = notecard.newRequest("hub.set");
    JAddStringToObject(req, "product", NOTEHUB_PRODUCT_UID);
    JAddStringToObject(req, "mode", "continuous");
	JAddBoolToObject(req, "uperiodic", true);
    JAddBoolToObject(req, "sync", true);
    JAddIntToObject(req, "inbound", 24*60);
    JAddStringToObject(req, "voutbound", "usb:5;60");
    if (!notecard.sendRequest(req)) { 
        return "notecard not responding";
    }

    // Inform the notehub of the our firmware version.  
    req = notecard.newRequest("dfu.status");
    JAddStringToObject(req, "version", PRODUCT_VERSION);
    if (!notecard.sendRequest(req)) {
		return "error sending our firmware version to Notehub";
	}

	// Set up for Outboard DFU assuming that we know the processor architecture.  Note
	// that this assumes proper wiring
#if defined(ARDUINO_SWAN_R5) || defined(ARDUINO_ARCH_ESP32)
    req = notecard.newRequest("card.dfu");
    JAddStringToObject(req, "mode", "aux");
#if defined(ARDUINO_SWAN_R5)
    JAddStringToObject(req, "name", "stm32");
#elif defined(ARDUINO_ARCH_ESP32)
    JAddStringToObject(req, "name", "esp32");
#endif
    if (!notecard.sendRequest(req)) {
		return "we can't do outboard DFU (card.dfu)";
	}
#endif

	// Ensure we're in an ODFU-compatible AUX mode
    req = notecard.newRequest("card.aux");
    JAddStringToObject(req, "mode", "-");
    if (!notecard.sendRequest(req)) {
		return "we can't do outboard DFU (card.aux)";
	}

	// Make sure we're in LiPo mode for voltage-variable requests
    req = notecard.newRequest("card.voltage");
    JAddStringToObject(req, "mode", "lipo");
    if (!notecard.sendRequest(req)) {
		return "can't set battery type";
	}

    // Query device capabilities
    rsp = notecard.requestAndResponse(notecard.newRequest("card.version"));
    if (rsp == NULL) {
		return "can't obtain notecard capabilities";
	}
	appHasGPS = JGetBool(rsp, "gps");
	appHasCellular = JGetBool(rsp, "cell");
	appHasWiFI = JGetBool(rsp, "wifi");
	appHasNTN = JGetBool(rsp, "ntn");
	appHasLoRa = JGetBool(rsp, "lora");
	JDelete(rsp);

    // Success
    return NULL;

}
