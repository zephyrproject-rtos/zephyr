#define DT_DRV_COMPAT ucb_htif 

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/htif.h>
#include <zephyr/sys/mutex.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_htif, CONFIG_UART_LOG_LEVEL);

// HTIF Memory-Mapped Registers
volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

// HTIF Mutex for thread safety
struct k_mutex htif_lock;

// HTIF Constants
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

// Ensure HTIF is ready before writing
static inline void htif_wait_for_ready(void) {
    while (tohost != 0) {
        if (fromhost != 0) {
            fromhost = 0;  // Acknowledge any pending responses
        }
    }
}

// Function to transmit a character using HTIF (with OpenSBI logic)
static void uart_htif_poll_out(const struct device *dev, unsigned char out_char) {
    k_mutex_lock(&htif_lock, K_FOREVER);

    htif_wait_for_ready();
    tohost = TOHOST_CMD(HTIF_DEV_CONSOLE, HTIF_CONSOLE_CMD_PUTC, out_char);

    k_mutex_unlock(&htif_lock);
}

// Function to receive a character (blocking)
static int uart_htif_poll_in(const struct device *dev, unsigned char *p_char) {
    int ch;

    k_mutex_lock(&htif_lock, K_FOREVER);

    // Check if there's already a character in fromhost
    if (fromhost != 0) {
        if (FROMHOST_DEV(fromhost) == HTIF_DEV_CONSOLE &&
            FROMHOST_CMD(fromhost) == HTIF_CONSOLE_CMD_GETC) {
            *p_char = (char)(FROMHOST_DATA(fromhost) & 0xFF);
            fromhost = 0;  // Acknowledge receipt
            k_mutex_unlock(&htif_lock);
            return 0;
        }
    }

    // Request a character
    htif_wait_for_ready();
    tohost = TOHOST_CMD(HTIF_DEV_CONSOLE, HTIF_CONSOLE_CMD_GETC, 0);

    // Wait for response
    while (fromhost == 0);

    // Extract received character
    ch = FROMHOST_DATA(fromhost);
    fromhost = 0;  // Acknowledge receipt

    k_mutex_unlock(&htif_lock);

    if (ch == -1) {
        return -1;  // No character available
    }

    *p_char = (char)(ch & 0xFF);
    return 0;  // Success
}

static int uart_htif_init(const struct device *dev) {
    k_mutex_init(&htif_lock);
    return 0;
}

static const struct uart_driver_api uart_htif_driver_api = {
    .poll_in  = uart_htif_poll_in,
    .poll_out = uart_htif_poll_out,
    .err_check = NULL,
};

// Register the UART device with Zephyr
DEVICE_DT_DEFINE(DT_NODELABEL(htif), uart_htif_init,
                 NULL, NULL, NULL,
                 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
                 &uart_htif_driver_api);
