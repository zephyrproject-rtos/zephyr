/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HCI_VIRT_BOTTOM_H
#define HCI_VIRT_BOTTOM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* These functions run in the native simulator runner context. */
void hci_virt_open(void);
const char *hci_virt_bd_addr(void);
bool hci_virt_poll(void);
/* Return a byte count, zero when no data is ready, or a negative errno. */
int32_t hci_virt_read(void *buf, size_t len);
/* Return zero on success or a negative errno; failed writes disconnect the host. */
int hci_virt_write(const void *buf, size_t len);
void hci_virt_btsnoop_capture(const void *buf, size_t len, bool tx);

#endif /* HCI_VIRT_BOTTOM_H */
