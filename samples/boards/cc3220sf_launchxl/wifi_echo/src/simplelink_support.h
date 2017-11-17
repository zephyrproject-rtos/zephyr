/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/* This method of passing SSID and Password on the command line is in lieu
 * of a more powerful, complex method of device provisioning.
 * TI SimpleLink supports a few options for provisioning, the most common
 * in use being AP mode provisioning for IoT headless devices.
 * This should be added in the future; until then, at least this manual
 * method avoids storing one's WiFi password into a Kconfig file.
 * To build:
 *  % export CFLAGS="-DNET_SSID="<SSID_Name>" -DNET_PASS="<password>""
 *  % mkdir build && cd build
 *  % cmake .. -DBOARD=cc3220sf_launchxl
 *  % make
 */
#ifdef NET_SSID
#define SSID_NAME       STRINGIFY(NET_SSID)
#else
#define SSID_NAME	"DummySSID"
#endif

#ifdef NET_PASS
#define SECURITY_KEY    STRINGIFY(NET_PASS)
#else
#define SECURITY_KEY	"DummyPassword"
#endif

extern void simplelink_init(void);
