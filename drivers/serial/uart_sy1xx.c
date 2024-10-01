/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#define DT_DRV_COMPAT sensry_sy1xx_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <udma.h>
#include <pad_ctrl.h>

struct sy1xx_uart_config {
	uint32_t base;
	uint32_t inst;
};

typedef struct {
	uint16_t data_len;
	uint8_t *data;
} uartTransfer_t;

typedef enum {
	DRIVERS_UART_STOP_1,
	DRIVERS_UART_STOP_1_5,
	DRIVERS_UART_STOP_2
} uart_stop_t;

typedef enum {
	DRIVERS_UART_PAR_NONE,
	DRIVERS_UART_PAR_EVEN,
	DRIVERS_UART_PAR_ODD,
	DRIVERS_UART_PAR_MARK,
	DRIVERS_UART_PAR_SPACE
} uart_parity_t;

typedef struct {
	uint32_t baudrate;
	uart_stop_t stopbits;
	uart_parity_t parity;
} uartConfig_t;

#define DEVICE_MAX_BUFFER_SIZE (512)

struct sy1xx_uart_data {
	uint8_t write[DEVICE_MAX_BUFFER_SIZE];
	uint8_t read[DEVICE_MAX_BUFFER_SIZE];
};

/* prototypes */
static int32_t drivers_uart_read(const struct device *dev, uartTransfer_t *request);
static int32_t drivers_uart_write(const struct device *dev, uartTransfer_t *request);

static int32_t drivers_uart_configure(const struct device *dev, uartConfig_t *uart_cfg)
{
	struct sy1xx_uart_config *config = (struct sy1xx_uart_config *)dev->config;

	if (uart_cfg->baudrate == 0) {
		return -1;
	}

	/*
	 * The counter in the UDMA will count from 0 to div included
	 * and then will restart from 0, so we must give div - 1 as
	 * divider
	 */
	int32_t divider = soc_get_peripheral_clock() / uart_cfg->baudrate - 1;

	/*
	 * [31:16]: clock divider (from SoC clock)
	 * [9]: RX enable
	 * [8]: TX enable
	 * [3]: stop bits  0 = 1 stop bit
	 *                 1 = 2 stop bits
	 * [2:1]: bits     00 = 5 bits
	 *                 01 = 6 bits
	 *                 10 = 7 bits
	 *                 11 = 8 bits
	 * [0]: parity
	 */

	/* default: both tx and rx enabled; 8N1 configuration; 1 stop bits */
	volatile uint32_t setup = 0x0306 | uart_cfg->parity;

	setup |= ((divider) << 16);
	UDMA_WRITE_REG(config->base, UDMA_SETUP_REG, setup);

	/* start initial reading request to get the dma running */
	uint8_t dummy_data[10];

	uartTransfer_t dummy_request = {
		.data_len = 10,
		.data = (uint8_t *)dummy_data,
	};

	drivers_uart_read(dev, &dummy_request);
	return 0;
}

/**
 * @return
 *  - < 0: Error
 *  -   0: OK
 *  - > 0: Busy
 */
int32_t drivers_uart_read(const struct device *dev, uartTransfer_t *request)
{
	struct sy1xx_uart_config *config = (struct sy1xx_uart_config *)dev->config;
	struct sy1xx_uart_data *data = (struct sy1xx_uart_data *)dev->data;

	if (request == 0) {
		return -1;
	}

	uint32_t max_read_size = request->data_len;

	request->data_len = 0;

	if (max_read_size > DEVICE_MAX_BUFFER_SIZE) {
		return -3;
	}

	int32_t ret = 0;

	/* rx is ready */
	int32_t remaining_bytes = UDMA_READ_REG(config->base, UDMA_RX_SIZE_REG);
	int32_t bytes_transferred = (DEVICE_MAX_BUFFER_SIZE - remaining_bytes);

	if (bytes_transferred > 0) {
		/* copy data to the user buffer */
		uint32_t copy_len =
			bytes_transferred > max_read_size ? max_read_size : bytes_transferred;
		for (uint32_t i = 0; i < copy_len; i++) {
			request->data[i] = data->read[i];
		}

		/* update actual read length */
		request->data_len = bytes_transferred;

		/* stop and restart receiving */
		UDMA_CANCEL_RX(config->base);

		/* start another read request, with maximum buffer size */
		UDMA_START_RX(config->base, (int32_t)data->read, DEVICE_MAX_BUFFER_SIZE, 0);

		/* return: some data received */
		ret = 0;

	} else {
		/* return: (busy) stay in receiving mode */
		ret = 1;
	}

	return ret;
}

/**
 * @return
 *  - < 0: Error
 *  -   0: OK
 *  - > 0: Busy
 */
int32_t drivers_uart_write(const struct device *dev, uartTransfer_t *request)
{
	struct sy1xx_uart_config *config = (struct sy1xx_uart_config *)dev->config;
	struct sy1xx_uart_data *data = (struct sy1xx_uart_data *)dev->data;

	if (request == 0) {
		return -1;
	}

	if (request->data_len == 0) {
		return -1;
	}

	if (request->data_len > DEVICE_MAX_BUFFER_SIZE) {
		/* more data than possible requested */
		return -2;
	}

	if (0 == UDMA_IS_FINISHED_TX(config->base)) {
		/* writing not finished => busy */
		return 1;
	}

	uint32_t remaining_bytes = UDMA_GET_REMAINING_TX(config->base);

	if (remaining_bytes != 0) {
		UDMA_CANCEL_TX(config->base);
		return -3;
	}

	/* copy the data to transmission buffer */
	for (uint32_t i = 0; i < request->data_len; i++) {
		data->write[i] = request->data[i];
	}

	/* start new transmission */
	UDMA_START_TX(config->base, (uint32_t)data->write, request->data_len, 0);

	/* success */
	return 0;
}

/*
 * it should be avoided to read single characters only
 */
static int sensry_uart_poll_in(const struct device *dev, unsigned char *c)
{
	uartTransfer_t request = {
		.data_len = 1,
		.data = c,
	};

	if (0 == drivers_uart_read(dev, &request)) {
		return 0;
	}

	return -1;
}

/*
 * it should be avoided to write single characters only
 */
static void sensry_uart_poll_out(const struct device *dev, unsigned char c)
{
	uartTransfer_t request = {
		.data_len = 1,
		.data = &c,
	};

	while (1) {
		if (0 == drivers_uart_write(dev, &request)) {
			break;
		}
	}
}

static int sensry_uart_err_check(const struct device *dev)
{
	int err = 0;

	return err;
}

static int sy1xx_uart_init(const struct device *dev)
{
	struct sy1xx_uart_config *config = (struct sy1xx_uart_config *)dev->config;
	struct sy1xx_uart_data *data = (struct sy1xx_uart_data *)dev->data;

	for (uint32_t i = 0; i < DEVICE_MAX_BUFFER_SIZE; i++) {
		data->write[i] = 0xa5;
		data->read[i] = 0xb4;
	}

	/* UDMA clock enable */
	drivers_udma_enable_clock(DRIVERS_UDMA_UART, config->inst);

	/* PAD config */
	uint32_t pad_config_tx =
		PAD_CONFIG(0, PAD_SMT_DISABLE, PAD_SLEW_LOW, PAD_PULLUP_DIS, PAD_PULLDOWN_DIS,
			   PAD_DRIVE_2PF, PAD_PMOD_NORMAL, PAD_DIR_OUTPUT);

	uint32_t pad_config_rx =
		PAD_CONFIG(8, PAD_SMT_DISABLE, PAD_SLEW_LOW, PAD_PULLUP_DIS, PAD_PULLDOWN_DIS,
			   PAD_DRIVE_2PF, PAD_PMOD_NORMAL, PAD_DIR_INPUT);

	uint32_t pad_config_cts =
		PAD_CONFIG(16, PAD_SMT_DISABLE, PAD_SLEW_LOW, PAD_PULLUP_EN, PAD_PULLDOWN_DIS,
			   PAD_DRIVE_2PF, PAD_PMOD_NORMAL, PAD_DIR_INPUT);

	uint32_t pad_config_rts =
		PAD_CONFIG(24, PAD_SMT_DISABLE, PAD_SLEW_LOW, PAD_PULLUP_DIS, PAD_PULLDOWN_DIS,
			   PAD_DRIVE_2PF, PAD_PMOD_NORMAL, PAD_DIR_OUTPUT);

	sys_write32((pad_config_tx | pad_config_rx | pad_config_cts | pad_config_rts),
		    PAD_CONFIG_ADDR_UART + (config->inst * 4 + 0));

	uartConfig_t default_config = {
		.baudrate = 1000000,
		.parity = DRIVERS_UART_PAR_NONE,
		.stopbits = DRIVERS_UART_STOP_1,
	};

	UDMA_CANCEL_RX(config->base);
	UDMA_CANCEL_TX(config->base);

	drivers_uart_configure(dev, &default_config);

	return 0;
}

static const struct uart_driver_api sy1xx_uart_driver_api = {

	.poll_in = sensry_uart_poll_in,
	.poll_out = sensry_uart_poll_out,
	.err_check = sensry_uart_err_check,

};

#define SYS1XX_UART_INIT(n)                                                                        \
                                                                                                   \
	static const struct sy1xx_uart_config sy1xx_uart_##n##_cfg = {                             \
		.base = (uint32_t)DT_INST_REG_ADDR(n),                                             \
		.inst = (uint32_t)DT_INST_PROP(n, instance),                                       \
	};                                                                                         \
                                                                                                   \
	static struct sy1xx_uart_data __attribute__((section(".udma_access")))                     \
	__aligned(4) sy1xx_uart_##n##_data = {                                                     \
                                                                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sy1xx_uart_init, NULL, &sy1xx_uart_##n##_data,                   \
			      &sy1xx_uart_##n##_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &sy1xx_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SYS1XX_UART_INIT)
