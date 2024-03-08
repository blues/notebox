// Copyright 2022 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

// Global defs
#include "app.h"

// main.cpp
void mainTask(void *param);
bool mainPoll(void);

// b64.cpp
int b64decode_len(char *bufcoded);
int b64decode(uint8_t *bufout, char *bufcoded, int *processed);
int b64encode_len(int len);
int b64encode(char *encoded, uint8_t *string, int len);
