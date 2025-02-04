#define DT_DRV_COMPAT ucb_htif 

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/htif.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

// HTIF Memory-Mapped Registers
volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

LOG_MODULE_REGISTER(uart_htif, CONFIG_UART_LOG_LEVEL);


// Function to transmit a character
static void uart_htif_poll_out(const struct device *dev, unsigned char out_char) {
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) |
             ((uint64_t)HTIF_CMD_WRITE << 48) |
             (uint64_t)out_char;

    while (tohost != 0);  // Wait for FESVR to process the write
}

// Function to receive a character (blocking)
static int uart_htif_poll_in(const struct device *dev, unsigned char *p_char) {
    // Send a read request
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) | ((uint64_t)HTIF_CMD_READ << 48);

    // Wait until FESVR sends a response
    while (fromhost == 0);

    *p_char = (char)(fromhost & 0xFF);  // Extract received character
    fromhost = 0;  // Clear fromhost

    return 0;  // Success
}

static int uart_htif_init(const struct device *dev) {
    // LOG_INF("HTIF UART Initialized!");
    return 0;
}


static DEVICE_API(uart, uart_htif_driver_api) = {
	.poll_in          = uart_htif_poll_in,
	.poll_out         = uart_htif_poll_out,
	.err_check        = NULL,
};

// Register the UART device with Zephyr
DEVICE_DT_DEFINE(DT_NODELABEL(htif), uart_htif_init,
                 NULL, NULL, NULL,
                 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
                 &uart_htif_driver_api);
