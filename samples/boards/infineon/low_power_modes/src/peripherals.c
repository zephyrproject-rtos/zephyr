/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_APP_ROLE_DUT)

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/can.h>
#include <zephyr/audio/dmic.h>
#include <string.h>

#include "peripherals.h"

/*
 * Per-peripheral presence guards: each peripheral compiles in only when its
 * devicetree node is present, so this file adapts to boards with a different
 * subset of peripherals.
 */
#define PERIPH_HAS_PWM     DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
#define PERIPH_HAS_COUNTER DT_NODE_HAS_STATUS(DT_ALIAS(counter0), okay)
#define PERIPH_HAS_SPI     DT_NODE_HAS_STATUS(DT_NODELABEL(loopback_dev), okay)
#define PERIPH_HAS_I2C     DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
#define PERIPH_HAS_DMIC    DT_NODE_HAS_STATUS(DT_NODELABEL(dmic0), okay)
#define PERIPH_HAS_I2S     DT_NODE_HAS_STATUS(DT_ALIAS(i2s_tx), okay)
#define PERIPH_HAS_SDHC    DT_NODE_HAS_STATUS(DT_NODELABEL(sdhc1), okay)
#define PERIPH_HAS_UART    DT_NODE_HAS_STATUS(DT_ALIAS(uart_test), okay)
#define PERIPH_HAS_DMA     DT_NODE_HAS_STATUS(DT_NODELABEL(dma0), okay)
#define PERIPH_HAS_CAN     DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_canbus), okay)

#if PERIPH_HAS_PWM
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#endif

#if PERIPH_HAS_COUNTER
static const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(counter0));
#endif

#if PERIPH_HAS_SPI
#define SPI_LOOPBACK_NODE DT_NODELABEL(loopback_dev)
static const struct spi_dt_spec spi_loopback =
	SPI_DT_SPEC_GET(SPI_LOOPBACK_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB);

/* SPI loopback self-test: sends a pattern over a MOSI-to-MISO jumper and checks
 * it returns. Run before/after DeepSleep.
 */
static int spi_loopback_test(void)
{
	static const uint8_t tx_data[] = {0xA5, 0x5A, 0x00, 0xFF, 0x12, 0x34, 0x56, 0x78};
	uint8_t rx_data[sizeof(tx_data)] = {0};
	const struct spi_buf tx_buf = {.buf = (void *)tx_data, .len = sizeof(tx_data)};
	const struct spi_buf rx_buf = {.buf = rx_data, .len = sizeof(rx_data)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&spi_loopback, &tx_set, &rx_set);
	if (ret < 0) {
		printk("SPI: transceive failed (%d)\n", ret);
		return ret;
	}

	if (memcmp(tx_data, rx_data, sizeof(tx_data)) != 0) {
		printk("SPI loopback: MISMATCH (check the MOSI-to-MISO jumper)\n");
		return -EIO;
	}

	printk("SPI loopback: OK (%u bytes matched)\n", (unsigned int)sizeof(tx_data));
	return 0;
}
#endif /* PERIPH_HAS_SPI */

#if PERIPH_HAS_I2C
/* I2C single-register probe at a fixed address, repeated after each DeepSleep
 * wake. If no device answers it reports no-response consistently.
 */
#define I2C_BUS_NODE DT_NODELABEL(i2c0)
static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_BUS_NODE);
#define I2C_PROBE_ADDR 0x68U
#define I2C_PROBE_REG  0x00U

static int i2c_probe_test(void)
{
	uint8_t chip_id = 0;
	int ret;

	ret = i2c_reg_read_byte(i2c_dev, I2C_PROBE_ADDR, I2C_PROBE_REG, &chip_id);
	if (ret < 0) {
		printk("I2C probe: addr 0x%02x no response (%d)\n", I2C_PROBE_ADDR, ret);
		return ret;
	}

	printk("I2C probe: addr 0x%02x reg 0x%02x = 0x%02x\n", I2C_PROBE_ADDR, I2C_PROBE_REG,
	       chip_id);
	return 0;
}
#endif /* PERIPH_HAS_I2C */

#if PERIPH_HAS_DMIC
/* PDM microphone stream via DMA. One mono stream is a liveness check run
 * before/after each DeepSleep.
 */
#define DMIC_NODE DT_NODELABEL(dmic0)
static const struct device *const dmic_dev = DEVICE_DT_GET(DMIC_NODE);
#define DMIC_SAMPLE_RATE      16000U
#define DMIC_SAMPLE_BITS      16U
#define DMIC_HW_CHAN_IDX      1U
#define DMIC_BYTES_PER_SAMPLE (DMIC_SAMPLE_BITS / 8U)
/* One block holds 100 ms of mono audio. */
#define DMIC_BLOCK_SIZE       (DMIC_BYTES_PER_SAMPLE * (DMIC_SAMPLE_RATE / 10U))
#define DMIC_BLOCK_COUNT      4U
#define DMIC_READ_TIMEOUT     1000
K_MEM_SLAB_DEFINE_STATIC(dmic_mem_slab, DMIC_BLOCK_SIZE, DMIC_BLOCK_COUNT, 4);

/* DMIC self-test: configure a mono stream, start it, read one 100 ms block,
 * then stop. Liveness check: if configure+START are accepted the block and its
 * DMA channel came up; a read that only times out still counts as alive.
 */
static int dmic_test(void)
{
	struct pcm_stream_cfg stream = {
		.pcm_width = DMIC_SAMPLE_BITS,
		.mem_slab = &dmic_mem_slab,
	};
	struct dmic_cfg cfg = {
		.io = {
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc = 40,
			.max_pdm_clk_dc = 60,
		},
		.streams = &stream,
		.channel = {
			.req_num_streams = 1,
			.req_num_chan = 1,
		},
	};
	void *buffer;
	uint32_t size;
	int ret;

	cfg.channel.req_chan_map_lo = dmic_build_channel_map(0, DMIC_HW_CHAN_IDX, PDM_CHAN_LEFT);
	cfg.streams[0].pcm_rate = DMIC_SAMPLE_RATE;
	cfg.streams[0].block_size = DMIC_BLOCK_SIZE;

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		printk("DMIC: configure failed (%d)\n", ret);
		return ret;
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		printk("DMIC: START trigger failed (%d)\n", ret);
		return ret;
	}

	/* The block is alive from here: configure and START were accepted. */
	ret = dmic_read(dmic_dev, 0, &buffer, &size, DMIC_READ_TIMEOUT);
	if (ret == 0) {
		printk("DMIC: alive, captured %u bytes\n", size);
		k_mem_slab_free(&dmic_mem_slab, buffer);
	} else if (ret == -EAGAIN) {
		printk("DMIC: alive (configured + streaming), no block in %d ms\n",
		       DMIC_READ_TIMEOUT);
	} else {
		printk("DMIC: read error (%d)\n", ret);
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		printk("DMIC: STOP trigger failed (%d)\n", ret);
		return ret;
	}

	return 0;
}
#endif /* PERIPH_HAS_DMIC */

#if PERIPH_HAS_I2S
/* I2S transmitter via DMA in controller mode, so it self-clocks one block with
 * no external codec. Run before/after each DeepSleep.
 */
#define I2S_NODE DT_ALIAS(i2s_tx)
static const struct device *const i2s_dev = DEVICE_DT_GET(I2S_NODE);
#define I2S_SAMPLE_RATE   44100U
#define I2S_WORD_SIZE     16U
#define I2S_CHANNELS      2U
#define I2S_SAMPLES       64U
/* One stereo block of 16-bit samples. */
#define I2S_BLOCK_SIZE    (I2S_SAMPLES * I2S_CHANNELS * sizeof(int16_t))
#define I2S_BLOCK_COUNT   4U
#define I2S_WRITE_TIMEOUT 1000
K_MEM_SLAB_DEFINE_STATIC(i2s_mem_slab, I2S_BLOCK_SIZE, I2S_BLOCK_COUNT, 4);

/* I2S self-test: configure TX as controller, queue one block of silence, start,
 * then DROP. DROP returns the stream to READY synchronously (unlike
 * DRAIN/STOP), so it is never left RUNNING/STOPPING - which would veto every
 * DeepSleep and reject the next i2s_configure().
 */
static int i2s_test(void)
{
	struct i2s_config i2s_cfg = {
		.word_size = I2S_WORD_SIZE,
		.channels = I2S_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
		.frame_clk_freq = I2S_SAMPLE_RATE,
		.block_size = I2S_BLOCK_SIZE,
		.mem_slab = &i2s_mem_slab,
		.timeout = I2S_WRITE_TIMEOUT,
	};
	void *tx_block;
	int ret;

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		printk("I2S: configure failed (%d)\n", ret);
		return ret;
	}

	ret = k_mem_slab_alloc(&i2s_mem_slab, &tx_block, K_NO_WAIT);
	if (ret < 0) {
		printk("I2S: block alloc failed (%d)\n", ret);
		return ret;
	}
	memset(tx_block, 0, I2S_BLOCK_SIZE);

	ret = i2s_write(i2s_dev, tx_block, I2S_BLOCK_SIZE);
	if (ret < 0) {
		printk("I2S: write failed (%d)\n", ret);
		k_mem_slab_free(&i2s_mem_slab, tx_block);
		return ret;
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		printk("I2S: START trigger failed (%d)\n", ret);
		return ret;
	}

	/* Let the queued block clock out (one block is ~3 ms at 44.1 kHz). */
	k_msleep(10);

	/* DROP returns the stream to READY synchronously and frees queued buffers. */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		printk("I2S: DROP trigger failed (%d)\n", ret);
		return ret;
	}

	printk("I2S transmit: OK (%u bytes)\n", (unsigned int)I2S_BLOCK_SIZE);
	return 0;
}
#endif /* PERIPH_HAS_I2S */

#if PERIPH_HAS_SDHC
/* SD host controller self-test. Configures the block with no card required; run
 * before/after each DeepSleep.
 */
#define SDHC_NODE DT_NODELABEL(sdhc1)
static const struct device *const sdhc_dev = DEVICE_DT_GET(SDHC_NODE);

/* SDHC self-test: read host properties and apply an I/O config (power on, min
 * clock, 1-bit bus, 3.3 V), programming the block registers and clock divider.
 */
static int sdhc_test(void)
{
	struct sdhc_host_props props;
	struct sdhc_io io = {0};
	int present;
	int ret;

	ret = sdhc_get_host_props(sdhc_dev, &props);
	if (ret < 0) {
		printk("SDHC: get_host_props failed (%d)\n", ret);
		return ret;
	}

	io.clock = props.f_min;
	io.bus_mode = SDHC_BUSMODE_PUSHPULL;
	io.power_mode = SDHC_POWER_ON;
	io.bus_width = SDHC_BUS_WIDTH1BIT;
	io.timing = SDHC_TIMING_LEGACY;
	io.signal_voltage = SD_VOL_3_3_V;

	ret = sdhc_set_io(sdhc_dev, &io);
	if (ret < 0) {
		printk("SDHC: set_io failed (%d)\n", ret);
		return ret;
	}

	present = sdhc_card_present(sdhc_dev);
	printk("SDHC host configured: OK (f_min %u Hz, card %s)\n", props.f_min,
	       (present == 1) ? "present" : "absent");
	return 0;
}
#endif /* PERIPH_HAS_SDHC */

#if PERIPH_HAS_UART
/* Second UART, separate from the console and the only peripheral using device
 * runtime PM (zephyr,pm-device-runtime-auto): its interrupt-driven
 * enable/disable paths take and drop a runtime reference. The loopback verifies
 * a TX-to-RX jumper and drives the runtime PM get/put paths. Run before/after
 * DeepSleep.
 */
#define UART_TEST_NODE DT_ALIAS(uart_test)
static const struct device *const uart_test_dev = DEVICE_DT_GET(UART_TEST_NODE);

static const uint8_t uart_test_tx[] = {0xC3, 0x3C, 0x00, 0xFF, 0xA5, 0x5A, 0x0F, 0xF0};
static volatile uint8_t uart_test_rx[sizeof(uart_test_tx)];
static volatile size_t uart_test_tx_idx;
static volatile size_t uart_test_rx_idx;
static K_SEM_DEFINE(uart_test_done, 0, 1);

/* Interrupt-driven loopback ISR: fills TX from the pattern and drains RX. Each
 * direction disables its own interrupt when done, dropping its runtime PM ref.
 */
static void uart_test_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uint8_t c;

			while ((uart_test_rx_idx < sizeof(uart_test_rx)) &&
			       (uart_fifo_read(dev, &c, 1) == 1)) {
				uart_test_rx[uart_test_rx_idx++] = c;
			}

			if (uart_test_rx_idx >= sizeof(uart_test_rx)) {
				uart_irq_rx_disable(dev);
				k_sem_give(&uart_test_done);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			if (uart_test_tx_idx < sizeof(uart_test_tx)) {
				uart_test_tx_idx += uart_fifo_fill(
					dev, (const uint8_t *)&uart_test_tx[uart_test_tx_idx],
					sizeof(uart_test_tx) - uart_test_tx_idx);
			}

			if ((uart_test_tx_idx >= sizeof(uart_test_tx)) &&
			    uart_irq_tx_complete(dev)) {
				uart_irq_tx_disable(dev);
			}
		}

		uart_irq_update(dev);
	}
}

/* UART loopback self-test: send the pattern (interrupt-driven) and check it
 * returns. Requires a TX-to-RX jumper.
 */
static int uart_test(void)
{
	int ret;

	uart_test_tx_idx = 0;
	uart_test_rx_idx = 0;
	memset((void *)uart_test_rx, 0, sizeof(uart_test_rx));
	k_sem_reset(&uart_test_done);

	ret = uart_irq_callback_user_data_set(uart_test_dev, uart_test_isr, NULL);
	if (ret < 0) {
		printk("UART: callback set failed (%d)\n", ret);
		return ret;
	}

	uart_irq_rx_enable(uart_test_dev);
	uart_irq_tx_enable(uart_test_dev);

	if (k_sem_take(&uart_test_done, K_MSEC(100)) != 0) {
		uart_irq_tx_disable(uart_test_dev);
		uart_irq_rx_disable(uart_test_dev);
		printk("UART loopback: TIMEOUT (check the TX-to-RX jumper)\n");
		return -ETIMEDOUT;
	}

	if (memcmp((const void *)uart_test_rx, uart_test_tx, sizeof(uart_test_tx)) != 0) {
		printk("UART loopback: MISMATCH (check the TX-to-RX jumper)\n");
		return -EIO;
	}

	printk("UART loopback: OK (%u bytes matched)\n", (unsigned int)sizeof(uart_test_tx));
	return 0;
}
#endif /* PERIPH_HAS_UART */

#if PERIPH_HAS_DMA
/* DMA memory-to-memory self-test: a software-triggered channel copies a pattern
 * between two RAM buffers, so it needs no external wiring. Run before/after each
 * DeepSleep.
 */
#define DMA_NODE DT_NODELABEL(dma0)
static const struct device *const dma_dev = DEVICE_DT_GET(DMA_NODE);
#define DMA_TEST_CHANNEL 0U
#define DMA_XFER_SIZE    32U

static K_SEM_DEFINE(dma_done, 0, 1);
static volatile int dma_cb_status;

static void dma_test_cb(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	dma_cb_status = status;
	k_sem_give(&dma_done);
}

static int dma_test(void)
{
	/* Source in RAM (.data) so the transfer is pure memory-to-memory. */
	static uint8_t dma_src[DMA_XFER_SIZE] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA,
		0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A,
		0x69, 0x78, 0x87, 0x96, 0xA5, 0xB4, 0xC3, 0xD2, 0xE1, 0xF0,
	};
	static uint8_t dma_dst[DMA_XFER_SIZE];
	struct dma_block_config block = {
		.source_address = (uint32_t)dma_src,
		.dest_address = (uint32_t)dma_dst,
		.block_size = DMA_XFER_SIZE,
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
	};
	struct dma_config cfg = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1,
		.dest_data_size = 1,
		.block_count = 1,
		.head_block = &block,
		.dma_callback = dma_test_cb,
	};
	int ret;

	memset(dma_dst, 0, sizeof(dma_dst));
	k_sem_reset(&dma_done);

	ret = dma_config(dma_dev, DMA_TEST_CHANNEL, &cfg);
	if (ret < 0) {
		printk("DMA: config failed (%d)\n", ret);
		return ret;
	}

	ret = dma_start(dma_dev, DMA_TEST_CHANNEL);
	if (ret < 0) {
		printk("DMA: start failed (%d)\n", ret);
		return ret;
	}

	if (k_sem_take(&dma_done, K_MSEC(100)) != 0) {
		printk("DMA: TIMEOUT waiting for completion\n");
		(void)dma_stop(dma_dev, DMA_TEST_CHANNEL);
		return -ETIMEDOUT;
	}

	if (dma_cb_status < 0) {
		printk("DMA: transfer error (%d)\n", dma_cb_status);
		return dma_cb_status;
	}

	if (memcmp(dma_src, dma_dst, DMA_XFER_SIZE) != 0) {
		printk("DMA mem-to-mem: MISMATCH\n");
		return -EIO;
	}

	printk("DMA mem-to-mem: OK (%u bytes copied)\n", (unsigned int)DMA_XFER_SIZE);
	return 0;
}
#endif /* PERIPH_HAS_DMA */

#if PERIPH_HAS_CAN
/* CAN FD in internal loopback, so no transceiver or bus wiring is needed. Sends
 * one classic frame and receives it back through a msgq filter. Run before/after
 * each DeepSleep; a DeepSleep-RAM warm boot re-powers the Message RAM and
 * re-inits the channel.
 */
#define CAN_NODE DT_CHOSEN(zephyr_canbus)
static const struct device *const can_dev = DEVICE_DT_GET(CAN_NODE);
#define CAN_TEST_ID 0x123U
CAN_MSGQ_DEFINE(can_test_msgq, 2);

static int can_test(void)
{
	static const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67};
	const struct can_filter filter = {
		.id = CAN_TEST_ID,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	struct can_frame tx_frame = {
		.id = CAN_TEST_ID,
		.dlc = can_bytes_to_dlc(sizeof(payload)),
		.flags = 0,
	};
	struct can_frame rx_frame;
	int filter_id;
	int ret;

	memcpy(tx_frame.data, payload, sizeof(payload));

	/* Internal loopback: TX is routed to RX on-chip with a self-generated ACK. */
	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	if (ret < 0) {
		printk("CAN: set_mode(LOOPBACK) failed (%d)\n", ret);
		return ret;
	}

	ret = can_start(can_dev);
	if (ret < 0) {
		printk("CAN: start failed (%d)\n", ret);
		return ret;
	}

	filter_id = can_add_rx_filter_msgq(can_dev, &can_test_msgq, &filter);
	if (filter_id < 0) {
		printk("CAN: add_rx_filter failed (%d)\n", filter_id);
		(void)can_stop(can_dev);
		return filter_id;
	}

	ret = can_send(can_dev, &tx_frame, K_MSEC(100), NULL, NULL);
	if (ret < 0) {
		printk("CAN: send failed (%d)\n", ret);
		can_remove_rx_filter(can_dev, filter_id);
		(void)can_stop(can_dev);
		return ret;
	}

	ret = k_msgq_get(&can_test_msgq, &rx_frame, K_MSEC(100));
	can_remove_rx_filter(can_dev, filter_id);
	(void)can_stop(can_dev);
	if (ret != 0) {
		printk("CAN loopback: TIMEOUT (no frame received)\n");
		return -ETIMEDOUT;
	}

	if ((rx_frame.id != tx_frame.id) || (rx_frame.dlc != tx_frame.dlc) ||
	    (memcmp(rx_frame.data, payload, sizeof(payload)) != 0)) {
		printk("CAN loopback: MISMATCH\n");
		return -EIO;
	}

	printk("CAN loopback: OK (%u bytes matched)\n", (unsigned int)sizeof(payload));
	return 0;
}
#endif /* PERIPH_HAS_CAN */

void peripherals_setup(void)
{
#if PERIPH_HAS_PWM
	/* PWM on blue LED at 2 Hz: it freezes during DeepSleep and resumes on wake
	 * via the PWM pm_action.
	 */
	if (!pwm_is_ready_dt(&pwm_led)) {
		printk("Error: PWM device not ready\n");
	} else {
		int ret = pwm_set_dt(&pwm_led, PWM_MSEC(500), PWM_MSEC(250));

		if (ret < 0) {
			printk("Error %d: failed to set PWM\n", ret);
		} else {
			printk("PWM blue LED started at 2 Hz\n");
		}
	}
#endif

#if PERIPH_HAS_COUNTER
	/* Free-running counter: stops while HF clocks are gated, resumes on wake.
	 * Reading the value across a sleep confirms it resumed.
	 */
	if (!device_is_ready(counter_dev)) {
		printk("Error: counter device not ready\n");
	} else {
		int ret = counter_start(counter_dev);

		if (ret < 0) {
			printk("Error %d: failed to start counter\n", ret);
		} else {
			printk("Free-running counter started\n");
		}
	}
#endif

#if PERIPH_HAS_SPI
	/* SPI loopback baseline. Requires a MOSI-to-MISO jumper. */
	if (!spi_is_ready_dt(&spi_loopback)) {
		printk("Error: SPI loopback device not ready\n");
	} else {
		printk("SPI loopback baseline:\n");
		(void)spi_loopback_test();
	}
#endif

#if PERIPH_HAS_I2C
	/* I2C probe baseline. */
	if (!device_is_ready(i2c_dev)) {
		printk("Error: I2C bus %s not ready\n", i2c_dev->name);
	} else {
		printk("I2C probe baseline:\n");
		(void)i2c_probe_test();
	}
#endif

#if PERIPH_HAS_DMIC
	/* DMIC capture baseline - captures one mono block from the PDM mic. */
	if (!device_is_ready(dmic_dev)) {
		printk("Error: DMIC device %s not ready\n", dmic_dev->name);
	} else {
		printk("DMIC capture baseline:\n");
		(void)dmic_test();
	}
#endif

#if PERIPH_HAS_I2S
	/* I2S transmit baseline - streams one block out of the TDM block. */
	if (!device_is_ready(i2s_dev)) {
		printk("Error: I2S device %s not ready\n", i2s_dev->name);
	} else {
		printk("I2S transmit baseline:\n");
		(void)i2s_test();
	}
#endif

#if PERIPH_HAS_SDHC
	/* SDHC baseline - configures the SD host block (no card required). */
	if (!device_is_ready(sdhc_dev)) {
		printk("Error: SDHC device %s not ready\n", sdhc_dev->name);
	} else {
		printk("SDHC baseline:\n");
		(void)sdhc_test();
	}
#endif

#if PERIPH_HAS_UART
	/* UART loopback baseline. Requires a TX-to-RX jumper. */
	if (!device_is_ready(uart_test_dev)) {
		printk("Error: UART test device %s not ready\n", uart_test_dev->name);
	} else {
		printk("UART loopback baseline:\n");
		(void)uart_test();
	}
#endif

#if PERIPH_HAS_DMA
	/* DMA baseline - a software-triggered memory-to-memory copy. */
	if (!device_is_ready(dma_dev)) {
		printk("Error: DMA device %s not ready\n", dma_dev->name);
	} else {
		printk("DMA mem-to-mem baseline:\n");
		(void)dma_test();
	}
#endif

#if PERIPH_HAS_CAN
	/* CAN loopback baseline (internal loopback, no external wiring). */
	if (!device_is_ready(can_dev)) {
		printk("Error: CAN device %s not ready\n", can_dev->name);
	} else {
		printk("CAN loopback baseline:\n");
		(void)can_test();
	}
#endif
}

uint32_t peripherals_counter_read(void)
{
#if PERIPH_HAS_COUNTER
	uint32_t val = 0;

	(void)counter_get_value(counter_dev, &val);
	return val;
#else
	return 0U;
#endif
}

void peripherals_test_after_wake(const char *phase)
{
#if PERIPH_HAS_PWM
	/* The PWM free-runs and is never read back, so nothing else triggers its
	 * lazy DeepSleep-RAM rebuild. Re-applying the setting runs the PWM
	 * pm_action and restarts the 2 Hz blink.
	 */
	if (pwm_is_ready_dt(&pwm_led)) {
		int ret = pwm_set_dt(&pwm_led, PWM_MSEC(500), PWM_MSEC(250));

		printk("Phase 2: PWM blue LED re-armed after %s (%s)\n", phase,
		       (ret < 0) ? "FAILED" : "blinking");
	}
#endif

#if PERIPH_HAS_COUNTER
	uint32_t cnt_wake = 0;
	uint32_t cnt_after = 0;

	/* Two reads with clocks on: a changing value proves the counter runs again. */
	(void)counter_get_value(counter_dev, &cnt_wake);
	k_busy_wait(1000);
	(void)counter_get_value(counter_dev, &cnt_after);

	printk("Phase 2: counter %u -> %u (%s)\n", cnt_wake, cnt_after,
	       (cnt_after != cnt_wake) ? "running" : "STOPPED");
#endif

#if PERIPH_HAS_SPI
	/* Re-run SPI loopback: a match proves the SPI pm_action restored the block. */
	printk("Phase 2: SPI loopback after %s:\n", phase);
	(void)spi_loopback_test();
#endif

#if PERIPH_HAS_I2C
	/* Re-run the I2C probe: an identical result proves the block resumed. */
	printk("Phase 2: I2C probe after %s:\n", phase);
	(void)i2c_probe_test();
#endif

#if PERIPH_HAS_DMIC
	/* Re-run DMIC capture: the stream coming back proves the audio path resumed. */
	printk("Phase 2: DMIC capture after %s:\n", phase);
	(void)dmic_test();
#endif

#if PERIPH_HAS_I2S
	/* Re-run I2S transmit: a successful stream proves the TDM block resumed. */
	printk("Phase 2: I2S transmit after %s:\n", phase);
	(void)i2s_test();
#endif

#if PERIPH_HAS_SDHC
	/* Re-run the SDHC config: an identical result proves the host block resumed. */
	printk("Phase 2: SDHC after %s:\n", phase);
	(void)sdhc_test();
#endif

#if PERIPH_HAS_UART
	/* Re-run UART loopback: a match proves the block and its runtime PM
	 * get/put paths still balance across the transition.
	 */
	printk("Phase 2: UART loopback after %s:\n", phase);
	(void)uart_test();
#endif

#if PERIPH_HAS_DMA
	/* Re-run the DMA copy: a match proves the DMA pm_action restored the block. */
	printk("Phase 2: DMA mem-to-mem after %s:\n", phase);
	(void)dma_test();
#endif

#if PERIPH_HAS_CAN
	/* Re-run CAN loopback: a matching frame proves the Message RAM was
	 * re-powered and the channel re-initialized.
	 */
	printk("Phase 2: CAN loopback after %s:\n", phase);
	(void)can_test();
#endif
}

#endif /* CONFIG_APP_ROLE_DUT */
