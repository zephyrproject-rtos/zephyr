#ifndef ZEPHYR_DRIVERS_HTIF_H
#define ZEPHYR_DRIVERS_HTIF_H

#include <stdint.h>
#include <zephyr/sys/mutex.h>

#define HTIF_DEV_CONSOLE        1
#define HTIF_CONSOLE_CMD_GETC   0
#define HTIF_CONSOLE_CMD_PUTC   1

#define HTIF_DATA_BITS          48
#define HTIF_DATA_MASK          ((1ULL << HTIF_DATA_BITS) - 1)
#define HTIF_DATA_SHIFT         0
#define HTIF_CMD_BITS           8
#define HTIF_CMD_MASK           ((1ULL << HTIF_CMD_BITS) - 1)
#define HTIF_CMD_SHIFT          48
#define HTIF_DEV_BITS           8
#define HTIF_DEV_MASK           ((1ULL << HTIF_DEV_BITS) - 1)
#define HTIF_DEV_SHIFT          56

// Macros to encode/decode HTIF requests
#define TOHOST_CMD(dev, cmd, payload) \
    (((uint64_t)(dev) << HTIF_DEV_SHIFT) | \
     ((uint64_t)(cmd) << HTIF_CMD_SHIFT) | \
     (uint64_t)(payload))

#define FROMHOST_DEV(val)  (((val) >> HTIF_DEV_SHIFT) & HTIF_DEV_MASK)
#define FROMHOST_CMD(val)  (((val) >> HTIF_CMD_SHIFT) & HTIF_CMD_MASK)
#define FROMHOST_DATA(val) (((val) >> HTIF_DATA_SHIFT) & HTIF_DATA_MASK)

// Global HTIF registers
extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

// Global HTIF mutex for synchronization
extern struct k_mutex htif_lock;

#endif // ZEPHYR_DRIVERS_HTIF_H
