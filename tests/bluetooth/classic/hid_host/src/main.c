/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test has no ZTEST cases of its own. With CONFIG_ZTEST=y the ztest
 * subsystem provides main() and keeps the image running so the pytest harness
 * can drive the classic shell. The HID Host shell commands (hid_host ...) are
 * registered by the classic shell subsystem enabled via CONFIG_BT_HID_HOST=y;
 * this translation unit only gives the build a source file to compile.
 */

#include <zephyr/kernel.h>
