/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Default certificates used in tests, inherited from the echo samples. */

static const unsigned char ca[] = {
#include "ca.inc"
};

static const unsigned char server[] = {
#include "server.inc"
};

static const unsigned char server_privkey[] = {
#include "server_privkey.inc"
};
