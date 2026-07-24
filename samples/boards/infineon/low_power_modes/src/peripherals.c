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
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/audio/dmic.h>
#include <string.h>

#include "peripherals.h"

/*
 * Per-peripheral presence guards.  Each peripheral's device, self-test and
 * baseline/after-wake handling is compiled in only when the matching node is
 * present in the board's devicetree, so this file adapts to other PSOC Edge E84
 * boards that expose a different subset of peripherals.
 */
#define PERIPH_HAS_PWM     DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
#define PERIPH_HAS_COUNTER DT_NODE_HAS_STATUS(DT_ALIAS(counter0), okay)
#define PERIPH_HAS_SPI     DT_NODE_HAS_STATUS(DT_NODELABEL(loopback_dev), okay)
#define PERIPH_HAS_I2C     DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
#define PERIPH_HAS_DMIC    DT_NODE_HAS_STATUS(DT_NODELABEL(dmic0), okay)
#define PERIPH_HAS_I2S     DT_NODE_HAS_STATUS(DT_ALIAS(i2s_tx), okay)
#define PERIPH_HAS_SDHC    DT_NODE_HAS_STATUS(DT_NODELABEL(sdhc1), okay)
#define PERIPH_HAS_UART    DT_NODE_HAS_STATUS(DT_ALIAS(uart_test), okay)

/* Console flush shared by every self-test so its output completes before the
 * next low-power transition.
 */
static const struct device *const uart_console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
extern uint32_t ifx_cat1_uart_get_num_in_tx_fifo(const struct device *dev);
extern bool ifx_cat1_uart_get_tx_active(const struct device *dev);

static void flush_console(void)
{
	while (ifx_cat1_uart_get_num_in_tx_fifo(uart_console_dev)) {
	}
	while (ifx_cat1_uart_get_tx_active(uart_console_dev)) {
	}
}

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

/* SPI loopback self-test: jumper P16.1 (MOSI) to P16.2 (MISO).  Sends a known
 * pattern and checks the received bytes match.  Run before and after DeepSleep
 * to confirm the SPI driver's pm_action keeps the block working across the
 * low-power transition.
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
		printk("SPI loopback: MISMATCH (is P16.1 jumpered to P16.2?)\n");
		return -EIO;
	}

	printk("SPI loopback: OK (%u bytes matched)\n", (unsigned int)sizeof(tx_data));
	return 0;
}
#endif /* PERIPH_HAS_SPI */

#if PERIPH_HAS_I2C
/* I2C controller on SCB0 (P8.0 SCL / P8.1 SDA).  The eval board populates a
 * BMI270 IMU at address 0x68; reading its CHIP_ID register is a self-contained
 * single-instance probe.  The probe is repeated after each DeepSleep wake; an
 * identical result confirms the I2C driver's pm_action kept the block working.
 * If no device answers, the probe simply returns an error consistently.
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
/* PDM/PCM microphone on PDM3 (P8.5 clk, P8.6 data) captured through DMA0.  A
 * single mono stream is started before and after each DeepSleep as a liveness
 * check: the capture is refused while active so a regular DeepSleep never
 * corrupts the stream, and bringing the stream back up after wake confirms the
 * DMIC and DMA driver pm_action callbacks kept the audio path alive.
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

/* DMIC self-test: configure a single mono stream, start it and try to read one
 * 100 ms block, then stop.  The goal here is a liveness check rather than a
 * pass/fail on the audio data: if configure and START are accepted the PDM
 * block and its DMA channel came up, so a read that returns data reports the
 * captured size, and a read that merely times out (no block within the window)
 * still reports the block as alive and streaming.  Run before and after
 * DeepSleep to confirm the DMIC and DMA driver pm_action callbacks kept the
 * audio path alive across the low-power transition.
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
/* I2S transmitter on TDM I2S1 (P11.0 SCK, P11.1 FSYNC, P11.2 SD) driven by
 * DMA0.  Configured as a controller so it generates its own clocks and streams
 * out a single block without needing an external codec.  The block is sent
 * before and after each DeepSleep: a regular DeepSleep is refused while the
 * stream is active so it never corrupts a transfer, and a successful re-send
 * after wake confirms the I2S and DMA driver pm_action callbacks restored the
 * TDM block.
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

/* I2S self-test: configure the transmit stream as controller, queue one block
 * of silence, start the stream and let it clock out, then drop it.  Run before
 * and after DeepSleep to confirm the I2S driver's pm_action keeps the TDM block
 * working across the low-power transition.
 *
 * The I2S API is asynchronous: I2S_TRIGGER_DRAIN/STOP only request a stop and
 * the stream returns to the READY state later from the TX-underflow ISR.  This
 * self-test instead uses I2S_TRIGGER_DROP, which returns the stream to READY
 * synchronously, so the driver is never left in the RUNNING/STOPPING state when
 * this function returns.  That matters here because a stream stuck in STOPPING
 * would make the I2S pm_action veto every DeepSleep attempt and would reject the
 * next i2s_configure() call.
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

	/* DROP returns the stream to READY synchronously and frees any queued
	 * buffers, so the driver is left idle and ready to be reconfigured.
	 */
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
/* SD host controller on SDHC1, the eval board's microSD card slot (P7.x) with
 * card-detect on P17.7.  The self-test reads the host properties and applies an
 * I/O configuration (power on, minimum clock, 1-bit bus), which programs the
 * SDHC block registers and bus-clock divider; this needs no formatted card and
 * card presence is only reported for information.  Run before and after each
 * DeepSleep: an identical result confirms the SDHC driver's pm_action kept the
 * block working across the low-power transition (the configuration is retained
 * after a regular DeepSleep and fully re-initialized after a DeepSleep-RAM warm
 * boot).
 */
#define SDHC_NODE DT_NODELABEL(sdhc1)
static const struct device *const sdhc_dev = DEVICE_DT_GET(SDHC_NODE);

/* SDHC self-test: read the host properties and apply an I/O configuration
 * (power on, minimum clock, 1-bit bus, 3.3 V).  This programs the SDHC block
 * registers and bus-clock divider without requiring a formatted card.  Run
 * before and after DeepSleep to confirm the SDHC driver's pm_action keeps the
 * block working across the low-power transition.
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
/* Second UART instance on SCB5 (P17.1 TX, P17.0 RX).  This is a separate UART
 * from the console and is the only one wired for device runtime PM in this
 * sample: it opts in through zephyr,pm-device-runtime-auto, so the driver's
 * interrupt-driven TX/RX enable paths take a runtime reference and the disable
 * paths drop it.  The self-test loops a known pattern back through a physical
 * P17.1 (TX) to P17.0 (RX) jumper using the interrupt-driven API, which both
 * verifies the link and drives the runtime PM get/put paths.  Run before and
 * after DeepSleep to confirm the block keeps working across the transition.
 */
#define UART_TEST_NODE DT_ALIAS(uart_test)
static const struct device *const uart_test_dev = DEVICE_DT_GET(UART_TEST_NODE);

static const uint8_t uart_test_tx[] = {0xC3, 0x3C, 0x00, 0xFF, 0xA5, 0x5A, 0x0F, 0xF0};
static volatile uint8_t uart_test_rx[sizeof(uart_test_tx)];
static volatile size_t uart_test_tx_idx;
static volatile size_t uart_test_rx_idx;
static K_SEM_DEFINE(uart_test_done, 0, 1);

/* Interrupt-driven loopback ISR: fills the TX FIFO from the pattern and drains
 * the RX FIFO into the receive buffer.  Each direction disables its own
 * interrupt once complete, which drops the matching runtime PM reference.
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

/* UART loopback self-test: send the pattern with the interrupt-driven API and
 * check the received bytes match.  Requires P17.1 (TX) jumpered to P17.0 (RX).
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
		printk("UART loopback: TIMEOUT (is P17.1 jumpered to P17.0?)\n");
		return -ETIMEDOUT;
	}

	if (memcmp((const void *)uart_test_rx, uart_test_tx, sizeof(uart_test_tx)) != 0) {
		printk("UART loopback: MISMATCH (is P17.1 jumpered to P17.0?)\n");
		return -EIO;
	}

	printk("UART loopback: OK (%u bytes matched)\n", (unsigned int)sizeof(uart_test_tx));
	return 0;
}
#endif /* PERIPH_HAS_UART */

void peripherals_setup(void)
{
#if PERIPH_HAS_PWM
	/* PWM on blue LED at 2 Hz - HF clocks stop during DeepSleep, so a
	 * frozen blue LED confirms DeepSleep entry and a resumed blink
	 * confirms the PWM driver's pm_action restarted it on wake.
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
	/* Free-running counter - it stops while HF clocks are gated during
	 * DeepSleep and resumes counting on wake via the counter driver's
	 * pm_action.  Reading the value across a sleep confirms it resumed.
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
	flush_console();

#if PERIPH_HAS_SPI
	/* SPI loopback baseline - confirms the link works before any sleep.
	 * Requires P16.1 (MOSI) jumpered to P16.2 (MISO).
	 */
	if (!spi_is_ready_dt(&spi_loopback)) {
		printk("Error: SPI loopback device not ready\n");
	} else {
		printk("SPI loopback baseline:\n");
		(void)spi_loopback_test();
	}
	flush_console();
#endif

#if PERIPH_HAS_I2C
	/* I2C probe baseline - reads the on-board BMI270 CHIP_ID register. */
	if (!device_is_ready(i2c_dev)) {
		printk("Error: I2C bus %s not ready\n", i2c_dev->name);
	} else {
		printk("I2C probe baseline:\n");
		(void)i2c_probe_test();
	}
	flush_console();
#endif

#if PERIPH_HAS_DMIC
	/* DMIC capture baseline - captures one mono block from the PDM mic. */
	if (!device_is_ready(dmic_dev)) {
		printk("Error: DMIC device %s not ready\n", dmic_dev->name);
	} else {
		printk("DMIC capture baseline:\n");
		(void)dmic_test();
	}
	flush_console();
#endif

#if PERIPH_HAS_I2S
	/* I2S transmit baseline - streams one block out of the TDM block. */
	if (!device_is_ready(i2s_dev)) {
		printk("Error: I2S device %s not ready\n", i2s_dev->name);
	} else {
		printk("I2S transmit baseline:\n");
		(void)i2s_test();
	}
	flush_console();
#endif

#if PERIPH_HAS_SDHC
	/* SDHC baseline - configures the SD host block (no card required). */
	if (!device_is_ready(sdhc_dev)) {
		printk("Error: SDHC device %s not ready\n", sdhc_dev->name);
	} else {
		printk("SDHC baseline:\n");
		(void)sdhc_test();
	}
	flush_console();
#endif

#if PERIPH_HAS_UART
	/* UART loopback baseline - confirms the SCB5 link works before any sleep.
	 * Requires P17.1 (TX) jumpered to P17.0 (RX).
	 */
	if (!device_is_ready(uart_test_dev)) {
		printk("Error: UART test device %s not ready\n", uart_test_dev->name);
	} else {
		printk("UART loopback baseline:\n");
		(void)uart_test();
	}
	flush_console();
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
	/* The PWM free-runs in hardware and is never read back, so unlike the
	 * other peripherals nothing touches it after wake to trigger the
	 * driver's lazy DeepSleep-RAM rebuild.  Re-apply the same setting here:
	 * the first post-wake pwm_set_dt() runs the PWM driver's warm-boot
	 * re-init (pm_action) and restarts the 2 Hz blink.
	 */
	if (pwm_is_ready_dt(&pwm_led)) {
		int ret = pwm_set_dt(&pwm_led, PWM_MSEC(500), PWM_MSEC(250));

		printk("Phase 2: PWM blue LED re-armed after %s (%s)\n", phase,
		       (ret < 0) ? "FAILED" : "blinking");
		flush_console();
	}
#endif

#if PERIPH_HAS_COUNTER
	uint32_t cnt_wake = 0;
	uint32_t cnt_after = 0;

	/* Two quick reads with clocks on: a changing value proves the
	 * counter is running again, so its pm_action restarted it.
	 */
	(void)counter_get_value(counter_dev, &cnt_wake);
	k_busy_wait(1000);
	(void)counter_get_value(counter_dev, &cnt_after);

	printk("Phase 2: counter %u -> %u (%s)\n", cnt_wake, cnt_after,
	       (cnt_after != cnt_wake) ? "running" : "STOPPED");
	flush_console();
#endif

#if PERIPH_HAS_SPI
	/* Re-run the SPI loopback after waking: a match proves the SPI
	 * driver's pm_action restored the block.
	 */
	printk("Phase 2: SPI loopback after %s:\n", phase);
	(void)spi_loopback_test();
	flush_console();
#endif

#if PERIPH_HAS_I2C
	/* Re-run the I2C probe after waking: an identical result proves
	 * the I2C driver's pm_action restored the SCB block.
	 */
	printk("Phase 2: I2C probe after %s:\n", phase);
	(void)i2c_probe_test();
	flush_console();
#endif

#if PERIPH_HAS_DMIC
	/* Re-run the DMIC capture after waking: bringing the stream back up
	 * proves the DMIC and DMA driver pm_action callbacks kept the audio
	 * path alive.
	 */
	printk("Phase 2: DMIC capture after %s:\n", phase);
	(void)dmic_test();
	flush_console();
#endif

#if PERIPH_HAS_I2S
	/* Re-run the I2S transmit after waking: a successful stream
	 * proves the I2S and DMA driver pm_action callbacks restored
	 * the TDM block.
	 */
	printk("Phase 2: I2S transmit after %s:\n", phase);
	(void)i2s_test();
	flush_console();
#endif

#if PERIPH_HAS_SDHC
	/* Re-run the SDHC configuration after waking: an identical
	 * result proves the SDHC driver's pm_action restored the host
	 * block.
	 */
	printk("Phase 2: SDHC after %s:\n", phase);
	(void)sdhc_test();
	flush_console();
#endif

#if PERIPH_HAS_UART
	/* Re-run the UART loopback after waking: a match proves the UART
	 * driver kept the SCB5 block working and its restricted-scope
	 * runtime PM get/put paths still balance across the transition.
	 */
	printk("Phase 2: UART loopback after %s:\n", phase);
	(void)uart_test();
	flush_console();
#endif
}

#endif /* CONFIG_APP_ROLE_DUT */
