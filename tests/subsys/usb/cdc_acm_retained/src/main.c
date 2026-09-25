/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Each stage runs in its own boot: it checks the TX FIFO the driver restored,
 * reads it with the virtual host and resets into the next stage. FIFOs left by
 * another image are written before the driver initializes.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>
#include <sample_usbd.h>

#include <usbh_device.h>

#define CDC_ACM_NODE DT_NODELABEL(cdc_acm_uart0)
#define REGION_NODE  DT_PHANDLE(CDC_ACM_NODE, memory_region)
#define TX_FIFO_SIZE DT_PROP(CDC_ACM_NODE, tx_fifo_size)

#define FIFO_MAGIC    0x43444346U
#define FIFO_MAX_SIZE (DT_REG_SIZE(REGION_NODE) - sizeof(struct retained_fifo))
#define BOOT_MAGIC    0x424f4f54U

#define FIRST_LEN  1000U
#define OTHER_SIZE 3000U
#define OTHER_LEN  700U

/* Longer than the delay between the echo mitigation ZLP and the first data */
#define READ_TIMEOUT K_MSEC(CONFIG_USBD_CDC_ACM_TX_DELAY_MS + 100)

/* The driver's persisted layout */
struct retained_fifo {
	uint32_t magic;
	uint32_t size;
	uint32_t rd;
	uint32_t wr;
	uint8_t data[];
};

BUILD_ASSERT(FIRST_LEN > USBD_MAX_BULK_MPS && FIRST_LEN < TX_FIFO_SIZE);
BUILD_ASSERT(OTHER_SIZE > TX_FIFO_SIZE && OTHER_SIZE <= FIFO_MAX_SIZE);

struct seed {
	uint32_t magic;
	uint32_t size;
	uint32_t rd;
	uint32_t wr;
};

enum stage {
	STAGE_FIRST,
	STAGE_KEPT_PARTIAL,
	STAGE_KEPT_FULL,
	STAGE_OTHER_SIZE,
	STAGE_REINIT,
};

static const struct seed reinit_seeds[] = {
	{FIFO_MAGIC, OTHER_SIZE, OTHER_LEN, OTHER_LEN},
	{FIFO_MAGIC, FIFO_MAX_SIZE + 1, 0, 100},
	{FIFO_MAGIC, TX_FIFO_SIZE, 2 * TX_FIFO_SIZE, 100},
	{FIFO_MAGIC, TX_FIFO_SIZE, 2 * TX_FIFO_SIZE - 50, 2 * TX_FIFO_SIZE + 50},
	{FIFO_MAGIC, TX_FIFO_SIZE, 0, TX_FIFO_SIZE + 1},
};

#define STAGE_LAST (STAGE_REINIT + ARRAY_SIZE(reinit_seeds) - 1)

static struct {
	uint32_t magic;
	uint32_t stage;
} boot __noinit;

static volatile struct retained_fifo *const fifo =
	(volatile struct retained_fifo *)DT_REG_ADDR(REGION_NODE);
static const struct device *const uart = DEVICE_DT_GET(CDC_ACM_NODE);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
static struct usbd_context *usbd;
static struct usb_device *udev;
static uint8_t bulk_in;
static K_SEM_DEFINE(xfer_sync, 0, 1);
static uint8_t rx[FIFO_MAX_SIZE];

static uint8_t pattern(const uint32_t stage, const uint32_t i)
{
	return i % 251U + stage;
}

static void seed(const struct seed *const s, const uint32_t len)
{
	fifo->magic = s->magic;
	fifo->size = s->size;
	fifo->rd = s->rd;
	fifo->wr = s->wr;

	for (uint32_t k = 0; k < len; k++) {
		fifo->data[(s->rd + k) % s->size] = pattern(boot.stage, k);
	}
}

static int seed_fifo(void)
{
	const struct seed bad_magic = {~FIFO_MAGIC, TX_FIFO_SIZE, 0, 100};
	const struct seed other_size = {FIFO_MAGIC, OTHER_SIZE, 2 * OTHER_SIZE - 200,
					OTHER_LEN - 200};

	if (boot.magic != BOOT_MAGIC) {
		boot.magic = BOOT_MAGIC;
		boot.stage = STAGE_FIRST;
	}

	if (boot.stage == STAGE_FIRST) {
		seed(&bad_magic, 0);
	} else if (boot.stage == STAGE_OTHER_SIZE) {
		seed(&other_size, OTHER_LEN);
	} else if (boot.stage >= STAGE_REINIT && boot.stage <= STAGE_LAST) {
		seed(&reinit_seeds[boot.stage - STAGE_REINIT], 0);
	}

	return 0;
}

/* Before the driver restores the FIFO */
SYS_INIT(seed_fifo, PRE_KERNEL_1, 0);

static int xfer_cb(struct usb_device *const dev, struct uhc_transfer *const xfer)
{
	k_sem_give(&xfer_sync);

	return 0;
}

/* Length of one IN transfer, or -EAGAIN if the device sent nothing */
static int read_xfer(uint8_t *const dst, const size_t size)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(udev, bulk_in, xfer_cb, NULL);
	zassert_not_null(xfer, "Failed to allocate transfer");
	buf = usbh_xfer_buf_alloc(udev, USBD_MAX_BULK_MPS);
	zassert_not_null(buf, "Failed to allocate buffer");
	zassert_ok(usbh_xfer_buf_add(udev, xfer, buf));
	zassert_ok(usbh_xfer_enqueue(udev, xfer));

	if (k_sem_take(&xfer_sync, READ_TIMEOUT) != 0) {
		zassert_ok(usbh_xfer_dequeue(udev, xfer));
		k_sem_take(&xfer_sync, K_FOREVER);
	}

	if (xfer->err == -ECONNRESET) {
		ret = -EAGAIN;
	} else {
		zassert_ok(xfer->err, "Transfer failed");
		zassert_true(buf->len <= size, "Device sent more than expected");
		memcpy(dst, buf->data, buf->len);
		ret = buf->len;
	}

	usbh_xfer_buf_free(udev, buf);
	usbh_xfer_free(udev, xfer);

	return ret;
}

/* Read until the device has nothing more to send */
static size_t host_read(void)
{
	size_t n = 0;
	int len;

	while ((len = read_xfer(&rx[n], sizeof(rx) - n)) >= 0) {
		n += len;
	}

	return n;
}

static void check_data(const size_t len, const uint32_t stage, const uint32_t from)
{
	for (size_t k = 0; k < len; k++) {
		zassert_equal(rx[k], pattern(stage, from + k), "Wrong byte %zu", k);
	}
}

static void check_fifo(const uint32_t size, const uint32_t rd, const uint32_t wr)
{
	zassert_equal(fifo->magic, FIFO_MAGIC, "Magic 0x%08x", fifo->magic);
	zassert_equal(fifo->size, size, "Size %u, expected %u", fifo->size, size);
	zassert_equal(fifo->rd, rd, "rd %u, expected %u", fifo->rd, rd);
	zassert_equal(fifo->wr, wr, "wr %u, expected %u", fifo->wr, wr);
}

static void write_data(const uint32_t stage, const uint32_t len)
{
	for (uint32_t i = 0; i < len; i++) {
		uart_poll_out(uart, pattern(stage, i));
	}
}

static void reset(void)
{
	boot.stage++;
	sys_reboot(SYS_REBOOT_WARM);
}

/* Data leaves the FIFO only when its transfer completes */
static void stage_first(void)
{
	check_fifo(TX_FIFO_SIZE, 0, 0);
	zassert_equal(host_read(), 0, "Invalid FIFO was sent");

	write_data(STAGE_FIRST, FIRST_LEN);
	k_msleep(10);
	check_fifo(TX_FIFO_SIZE, 0, FIRST_LEN);

	zassert_equal(read_xfer(rx, sizeof(rx)), USBD_MAX_BULK_MPS);
	check_data(USBD_MAX_BULK_MPS, STAGE_FIRST, 0);
	zassert_true(WAIT_FOR(fifo->rd == USBD_MAX_BULK_MPS, USEC_PER_SEC, k_msleep(1)),
		     "rd %u after the first transfer", fifo->rd);

	/* Reset with the next transfer in flight */
	k_msleep(10);
	reset();
}

static void stage_kept_partial(void)
{
	check_fifo(TX_FIFO_SIZE, USBD_MAX_BULK_MPS, FIRST_LEN);
	zassert_equal(host_read(), FIRST_LEN - USBD_MAX_BULK_MPS);
	check_data(FIRST_LEN - USBD_MAX_BULK_MPS, STAGE_FIRST, USBD_MAX_BULK_MPS);
	check_fifo(TX_FIFO_SIZE, FIRST_LEN, FIRST_LEN);

	/* Fill it, wrapping around the end of the storage */
	write_data(STAGE_KEPT_PARTIAL, TX_FIFO_SIZE);
	check_fifo(TX_FIFO_SIZE, FIRST_LEN, FIRST_LEN + TX_FIFO_SIZE);
	reset();
}

static void stage_kept_full(void)
{
	check_fifo(TX_FIFO_SIZE, FIRST_LEN, FIRST_LEN + TX_FIFO_SIZE);
	zassert_equal(host_read(), TX_FIFO_SIZE);
	check_data(TX_FIFO_SIZE, STAGE_KEPT_PARTIAL, 0);
	check_fifo(TX_FIFO_SIZE, FIRST_LEN + TX_FIFO_SIZE, FIRST_LEN + TX_FIFO_SIZE);
	reset();
}

/* Left by an image with a larger tx-fifo-size */
static void stage_other_size(void)
{
	check_fifo(OTHER_SIZE, 2 * OTHER_SIZE - 200, OTHER_LEN - 200);
	zassert_equal(host_read(), OTHER_LEN);
	check_data(OTHER_LEN, STAGE_OTHER_SIZE, 0);
	check_fifo(OTHER_SIZE, OTHER_LEN - 200, OTHER_LEN - 200);
	reset();
}

static void stage_reinit(void)
{
	check_fifo(TX_FIFO_SIZE, 0, 0);
	zassert_equal(host_read(), 0, "Invalid FIFO was sent");

	if (boot.stage < STAGE_LAST) {
		reset();
	}
}

ZTEST(cdc_acm_retained, test_tx_fifo_across_resets)
{
	TC_PRINT("Stage %u\n", boot.stage);

	switch (boot.stage) {
	case STAGE_FIRST:
		stage_first();
		break;
	case STAGE_KEPT_PARTIAL:
		stage_kept_partial();
		break;
	case STAGE_KEPT_FULL:
		stage_kept_full();
		break;
	case STAGE_OTHER_SIZE:
		stage_other_size();
		break;
	default:
		zassert_true(boot.stage <= STAGE_LAST, "Unexpected stage %u", boot.stage);
		stage_reinit();
		break;
	}
}

static bool device_configured(void)
{
	udev = usbh_device_get_any(&uhs_ctx);

	return udev != NULL && udev->state == USB_STATE_CONFIGURED;
}

static void *cdc_acm_retained_setup(void)
{
	zassert_ok(usbh_init(&uhs_ctx), "Failed to initialize USB host");
	zassert_ok(usbh_enable(&uhs_ctx), "Failed to enable USB host");
	zassert_ok(uhc_bus_reset(uhs_ctx.dev), "Failed to signal bus reset");
	zassert_ok(uhc_bus_resume(uhs_ctx.dev), "Failed to signal bus resume");
	zassert_ok(uhc_sof_enable(uhs_ctx.dev), "Failed to enable SoF generator");

	usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(usbd, "Failed to setup USB device");
	zassert_ok(usbd_init(usbd), "Failed to initialize device support");
	zassert_ok(usbd_enable(usbd), "Failed to enable device support");

	zassert_true(WAIT_FOR(device_configured(), USEC_PER_SEC, k_msleep(1)),
		     "Device not configured");

	for (uint8_t i = 1; i < ARRAY_SIZE(udev->ep_in); i++) {
		const struct usb_ep_descriptor *const desc = udev->ep_in[i].desc;

		if (desc != NULL &&
		    (desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			bulk_in = desc->bEndpointAddress;
		}
	}

	zassert_not_equal(bulk_in, 0, "No bulk IN endpoint");

	return NULL;
}

static void cdc_acm_retained_teardown(void *f)
{
	/* A rerun starts from the first stage */
	boot.magic = 0;
}

ZTEST_SUITE(cdc_acm_retained, NULL, cdc_acm_retained_setup, NULL, NULL, cdc_acm_retained_teardown);
