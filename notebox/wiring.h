// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#define	WIRING_TX_TO_CONSOLE_RX			PIN_SERIAL_TX	// Console's RX is wired to F_TX
#define	WIRING_RX_FM_CONSOLE_TX			PIN_SERIAL_RX	// Console's TX is wired to F_RX
#define	CONSOLE_UART_SPEED				115200

#define	WIRING_TX_TO_NOTECARD_AUX_RX	A4				// AUX_RX is wired to F_A4 (PC4, USART3_TX, AF7) 
#define	WIRING_RX_FM_NOTECARD_AUX_TX	A5				// AUX_TX is wired to F_A5 (PC5, USART3_RX, AF7)
#define	NOTECARD_AUX_SPEED				115200

#define	consoleUSB						Serial			// Assumes that Generic Serial supercedes UART
#define	CONSOLE_USB_SPEED				115200

#define	WIRING_LED						LED_BUILTIN
