#define DT_DRV_COMPAT ucb_htif

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/htif.h>
#ifdef CONFIG_MULTITHREADING
#include <zephyr/sys/mutex.h>
#endif
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <string.h>  /* for memset() if needed */

LOG_MODULE_REGISTER(uart_htif, CONFIG_UART_LOG_LEVEL);

/*
 * New configuration flag for optional busy-wait optimization.
 *
 * If CONFIG_UART_HTIF_USE_YIELD_SLEEP is defined, then the driver will either
 * call k_yield() (if CONFIG_COOP_ENABLED is set) or k_sleep(K_MSEC(1)) in the
 * waiting loops. If not defined, the loops remain tight.
 */
#ifdef CONFIG_UART_HTIF_USE_YIELD_SLEEP
	#ifdef CONFIG_COOP_ENABLED
		#define HTIF_WAIT_SLEEP() k_yield()
	#else
		#define HTIF_WAIT_SLEEP() k_sleep(K_MSEC(1))
	#endif
#else
	#define HTIF_WAIT_SLEEP() /* do nothing */
#endif

/* HTIF Memory-Mapped Registers */
volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

/* HTIF Mutex for thread safety */
#ifdef CONFIG_MULTITHREADING
struct k_mutex htif_lock;
#endif

/* HTIF Constants */
#define HTIF_DEV_CONSOLE        1
#define HTIF_CONSOLE_CMD_GETC   0
#define HTIF_CONSOLE_CMD_PUTC   1
#ifdef CONFIG_UART_HTIF_SYSCALL_PRINT
	/* New command code for full-string output via a pointer */
	#define HTIF_CONSOLE_CMD_PUTS 64
#endif

#define HTIF_DATA_BITS          48
#define HTIF_DATA_MASK          ((1ULL << HTIF_DATA_BITS) - 1)
#define HTIF_DATA_SHIFT         0
#define HTIF_CMD_BITS           8
#define HTIF_CMD_MASK           ((1ULL << HTIF_CMD_BITS) - 1)
#define HTIF_CMD_SHIFT          48
#define HTIF_DEV_BITS           8
#define HTIF_DEV_MASK           ((1ULL << HTIF_DEV_BITS) - 1)
#define HTIF_DEV_SHIFT          56

/* Macros to encode/decode HTIF requests */
#define TOHOST_CMD(dev, cmd, payload) \
	(((uint64_t)(dev) << HTIF_DEV_SHIFT) | \
	 ((uint64_t)(cmd) << HTIF_CMD_SHIFT) | \
	 (uint64_t)(payload))

#define FROMHOST_DEV(val)  (((val) >> HTIF_DEV_SHIFT) & HTIF_DEV_MASK)
#define FROMHOST_CMD(val)  (((val) >> HTIF_CMD_SHIFT) & HTIF_CMD_MASK)
#define FROMHOST_DATA(val) (((val) >> HTIF_DATA_SHIFT) & HTIF_DATA_MASK)

/*
 * Optimized wait loop: Wait until the HTIF tohost register is ready,
 * optionally yielding or sleeping to reduce CPU load.
 */
static inline void htif_wait_for_ready(void)
{
	while (tohost != 0) {
		if (fromhost != 0) {
			fromhost = 0;  /* Acknowledge any pending responses */
		}
		HTIF_WAIT_SLEEP();
	}
}

/*
 * --- Optional Buffered Output Feature ---
 *
 * If CONFIG_UART_HTIF_BUFFERED_OUTPUT is enabled, we buffer characters until a newline
 * or until the buffer is full, then flush the entire buffer with one "syscall".
 *
 * The buffer size is configurable by CONFIG_UART_HTIF_BUFFERED_OUTPUT_SIZE (default value assumed here).
 */
#ifdef CONFIG_UART_HTIF_BUFFERED_OUTPUT

#ifndef CONFIG_UART_HTIF_BUFFERED_OUTPUT_SIZE
#define CONFIG_UART_HTIF_BUFFERED_OUTPUT_SIZE 64
#endif

#define BUF_SIZE CONFIG_UART_HTIF_BUFFERED_OUTPUT_SIZE

/* Global buffer and length, shared by the HTIF device instance */
static char htif_output_buf[BUF_SIZE];
static int htif_output_buflen = 0;

#define SYS_write 64

#define SYSCALL1(n, a0) \
    htif_syscall((a0), 0, 0, (n))

#define SYSCALL2(n, a0, a1) \
    htif_syscall((a0), (a1), 0, (n))

#define SYSCALL3(n, a0, a1, a2) \
    htif_syscall((a0), (a1), (a2), (n))

#define fence(p, s) \
    __asm__ __volatile__ ("fence " #p ", " #s : : : "memory")
static inline void rmb(void) { fence(r, r); }
static inline void wmb(void) { fence(w, w); }

long htif_syscall(uint64_t a0, uint64_t a1, uint64_t a2, unsigned long n)
{
    volatile uint64_t buf[8];
    uint64_t sc;

    buf[0] = n;
    buf[1] = a0;
    buf[2] = a1;
    buf[3] = a2;

    sc = TOHOST_CMD(0, 0, (uintptr_t)&buf);
    k_mutex_lock(&htif_lock, K_FOREVER);
    wmb();
    tohost = sc;
    while (fromhost == 0);
    fromhost = 0;
    k_mutex_unlock(&htif_lock);

    rmb();
    return buf[0];
}

static ssize_t _write(int fd, const void *ptr, size_t len)
{
    return SYSCALL3(SYS_write, fd, (uintptr_t)ptr, len);
}

/* Flush the global output buffer.
 * When CONFIG_UART_HTIF_SYSCALL_PRINT is enabled, use _write() to send the entire buffer
 * as a SYS_write syscall so that the FESVR reads the pointer and length, then outputs to host stdout.
 */
static void uart_htif_buffer_flush(void)
{
#ifdef CONFIG_UART_HTIF_SYSCALL_PRINT
    /* Flush the buffer using the syscall-based interface */
    _write(0, htif_output_buf, htif_output_buflen);
#else
    for (int i = 0; i < htif_output_buflen; i++) {
        htif_wait_for_ready();
        tohost = TOHOST_CMD(HTIF_DEV_CONSOLE, HTIF_CONSOLE_CMD_PUTC, htif_output_buf[i]);
    }
#endif
    htif_output_buflen = 0;
}


static void uart_htif_poll_out(const struct device *dev, unsigned char ch)
{
#ifdef CONFIG_MULTITHREADING
    k_mutex_lock(&htif_lock, K_FOREVER);
#endif

    htif_output_buf[htif_output_buflen++] = ch;
    if ((ch == '\n') || (htif_output_buflen >= BUF_SIZE)) {
        uart_htif_buffer_flush();
    }

#ifdef CONFIG_MULTITHREADING
    k_mutex_unlock(&htif_lock);
#endif
}

#else /* CONFIG_UART_HTIF_BUFFERED_OUTPUT not defined */

static void uart_htif_poll_out(const struct device *dev, unsigned char out_char)
{
#ifdef CONFIG_MULTITHREADING
    k_mutex_lock(&htif_lock, K_FOREVER);
#endif

    htif_wait_for_ready();
    tohost = TOHOST_CMD(HTIF_DEV_CONSOLE, HTIF_CONSOLE_CMD_PUTC, out_char);

#ifdef CONFIG_MULTITHREADING
    k_mutex_unlock(&htif_lock);
#endif
}

#endif /* CONFIG_UART_HTIF_BUFFERED_OUTPUT */


/*
 * Receive a character (blocking).
 */
static int uart_htif_poll_in(const struct device *dev, unsigned char *p_char)
{
	int ch;
#ifdef CONFIG_MULTITHREADING
	k_mutex_lock(&htif_lock, K_FOREVER);
#endif

	/* Check if there's already a character in fromhost */
	if (fromhost != 0) {
		if (FROMHOST_DEV(fromhost) == HTIF_DEV_CONSOLE &&
		    FROMHOST_CMD(fromhost) == HTIF_CONSOLE_CMD_GETC) {
			*p_char = (char)(FROMHOST_DATA(fromhost) & 0xFF);
			fromhost = 0;  /* Acknowledge receipt */
#ifdef CONFIG_MULTITHREADING
			k_mutex_unlock(&htif_lock);
#endif
			return 0;
		}
	}

	/* Request a character */
	htif_wait_for_ready();
	tohost = TOHOST_CMD(HTIF_DEV_CONSOLE, HTIF_CONSOLE_CMD_GETC, 0);

	/* Wait for response */
	while (fromhost == 0) {
		HTIF_WAIT_SLEEP();
	}

	/* Extract received character */
	ch = FROMHOST_DATA(fromhost);
	fromhost = 0;  /* Acknowledge receipt */

#ifdef CONFIG_MULTITHREADING
	k_mutex_unlock(&htif_lock);
#endif

	if (ch == -1) {
		return -1;
	}

	*p_char = (char)(ch & 0xFF);
	return 0;
}

/*
 * Initialization function.
 */
static int uart_htif_init(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	k_mutex_init(&htif_lock);
#endif
	return 0;
}

static const struct uart_driver_api uart_htif_driver_api = {
	.poll_in  = uart_htif_poll_in,
	.poll_out = uart_htif_poll_out,
	.err_check = NULL,
};

/* Register the UART device with Zephyr */
DEVICE_DT_DEFINE(DT_NODELABEL(htif), uart_htif_init,
			NULL, NULL, NULL,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&uart_htif_driver_api);
