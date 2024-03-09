// Copyright 2022 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Notecard.h>
#include <string.h>
#include "wiring.h"
#include "NoteRTOS.h"

#pragma once

// If you want to comment this out, please make sure that you enable USB Support
// on the Arduino tools menu, so the output comes out of USB
#define NODEBUG

// Version of the product
#define	PRODUCT_DISPLAY_NAME		"Notebox"
#define PRODUCT_PROGRAMMATIC_NAME   "notebox"
#define PRODUCT_MAJOR				1
#define PRODUCT_MINOR				1
#define PRODUCT_PATCH				1
#define PRODUCT_VERSION PRODUCT_PROGRAMMATIC_NAME "-v" STRINGIFY(PRODUCT_MAJOR)

// C trickery for converting a #defined number to a #defined string
#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

// Notehub definitions
#define	NOTEHUB_PRODUCT_UID			"com.blues.notebox"
#define	NOTEHUB_ROUTE_ALIAS			"incoming"

// Define this if using USB serial
#if !defined(NODEBUG)
#define debug Serial
#endif

// Define the debug output stream device, as well as a method enabling us
// to determine whether or not the Serial device is available for app usage.
#if !defined(NODEBUG)
#if defined(debug)
#define	serialIsAvailable() false
#else // Using serial
#define	serialIsAvailable() true
#ifdef APP_MAIN
HardwareSerial debug(PIN_SERIAL_LP1_RX, PIN_SERIAL_LP1_TX);
#else
extern HardwareSerial debug;
#endif
#endif
#endif // NODEBUG

// If no debug, eliminate the possibility that it will be referenced
#if defined(NODEBUG)
#define debugf(format, ...)
#else
#define	debugf(format, ...) debug.printf(format, ##__VA_ARGS__)
#endif

// Notecard definition
#ifdef APP_MAIN
Notecard notecard;
#else
extern Notecard notecard;
#endif

// Notecard serial ports
#ifdef SERIAL_NOTECARD
extern HardwareSerial notecardSerial;
#endif
extern HardwareSerial notecardAux;

// Fallthrough checking, in case developers like to protect their switch statements
#if !defined(__fallthrough) && __cplusplus > 201703L && defined(__has_cpp_attribute)
#if __has_cpp_attribute(fallthrough)
#define __fallthrough [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define __fallthrough [[gnu::fallthrough]]
#endif
#endif
#if !defined(__fallthrough) && defined(__has_attribute)
#if __has_attribute(__fallthrough__)
#define __fallthrough __attribute__((__fallthrough__))
#endif
#endif
#if !defined(__fallthrough) && defined(__GNUC__)
#define __fallthrough __attribute__((fallthrough))
#endif
#if !defined(__fallthrough)
#define __fallthrough ((void)0)
#endif

// Utility functions
#define streql(a,b) (0 == strcmp(a,b))
#define strEQL(a,b) (((uint8_t *)(a))[sizeof(b "")-1] != '\0' ? false : 0 == memcmp(a,b,sizeof(b)))
#define strNULL(x) (((x) == NULL) || ((x)[0] == '\0'))
#define memeql(a,b,c) (0 == memcmp(a,b,c))
#define memeqlstr(a,b) (0 == memcmp(a,b,strlen(b)))

// app.cpp
bool appSetup(void);
void appSignal(uint32_t count);
const char *appVersion(void);
extern bool appHasGPS;
extern bool appHasCellular;
extern bool appHasWiFI;
extern bool appHasNTN;
extern bool appHasLoRa;

// Tasks in the app
#define TASKNAME_APP				"app"
#define TASKSTACK_APP				2000
#define TASKPRI_APP		            ( configMAX_PRIORITIES - 4 )        // normal, in the middle
#define TASKNAME_MAIN				"main"
#define TASKSTACK_MAIN				2000
#define TASKPRI_MAIN				( configMAX_PRIORITIES - 4 )        // normal, in the middle

// main.cpp
void mainTask(void *param);
bool mainPoll(void);
#ifdef SERIAL_NOTECARD
extern HardwareSerial notecardSerial;
#endif

