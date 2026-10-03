/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the AD5940/AD5941 sensor driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_AD5940_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_AD5940_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AD5940/AD5941 measurement modes.
 */
enum ad5940_mode {
	/** Electrochemical Impedance Spectroscopy — complex Z via DFT engine */
	AD5940_MODE_EIS = 0,
	/** Amperometry — DC current measurement at fixed bias */
	AD5940_MODE_AMPEROMETRY = 1,
	/** Cyclic voltammetry — triangular sweep with current measurement */
	AD5940_MODE_CYCLIC_VOLTAMMETRY = 2,
	/** Raw ADC streaming */
	AD5940_MODE_ADC = 3,
};

/**
 * @brief AD5940 sequencer slot IDs (maps to hardware slots A–D).
 *
 * The AD5940 has four independent sequencer slots (SEQ0–SEQ3). Each slot
 * holds a start address and instruction count in SRAM. The wakeup timer
 * rotates through the active slots in the order defined by SEQORDER.
 */
enum ad5940_seq_id {
	/** Sequencer slot 0 */
	AD5940_SEQ_0 = 0,
	/** Sequencer slot 1 */
	AD5940_SEQ_1 = 1,
	/** Sequencer slot 2 */
	AD5940_SEQ_2 = 2,
	/** Sequencer slot 3 */
	AD5940_SEQ_3 = 3,
};

/**
 * @brief Configuration for one AD5940 sequencer slot.
 *
 * The driver splits each sequence into an init phase and a measurement phase.
 * The application provides the compiled SRAM opcodes; the driver manages SRAM
 * placement, SEQxINFO programming, and wakeup/sleep timing.
 */
struct ad5940_seq_cfg {
	/**
	 * Init sequence opcodes (written to SRAM once at load time).
	 * Run once when the slot is first activated. Typically configures
	 * switch matrix, ADC mux, and AFE peripherals for this measurement type.
	 * Set to NULL and init_len to 0 if no init phase is needed.
	 */
	const uint32_t *init_opcodes;
	/** Number of opcodes in init_opcodes (0 if no init phase). */
	uint16_t        init_len;

	/**
	 * Measurement sequence opcodes (written to SRAM at load time).
	 * Run on every wakeup tick. Must end with SEQTRGSLP to re-hibernate.
	 */
	const uint32_t *meas_opcodes;
	/** Number of opcodes in meas_opcodes. Must be > 0. */
	uint16_t        meas_len;

	/** Wakeup timer count (LFOSC ticks the AFE stays awake per tick). */
	uint32_t        wakeup_ticks;
	/** Sleep timer count (LFOSC ticks the AFE hibernates between ticks). */
	uint32_t        sleep_ticks;

	/**
	 *FIFO data source for this sequence.
	 * Use the AD5940_FIFOSRC_* constants from the private header or
	 * define them using the public aliases below.
	 *   1 = ADC raw, 2 = DFT, 3 = SINC2, 4 = Variance, 5 = Mean
	 */
	uint8_t         fifo_src;
};

/**
 * @brief FIFO source selectors (public aliases for ad5940_seq_cfg.fifo_src).
 */
#define AD5940_PUBLIC_FIFOSRC_ADC      1u /**< Raw ADC output */
#define AD5940_PUBLIC_FIFOSRC_DFT      2u /**< DFT real+imag pair */
#define AD5940_PUBLIC_FIFOSRC_SINC2    3u /**< SINC2+Notch output */
#define AD5940_PUBLIC_FIFOSRC_VARIANCE 4u /**< Statistical variance */
#define AD5940_PUBLIC_FIFOSRC_MEAN     5u /**< Statistical mean */

/**
 * @brief AFE analog/conversion states for ad5940_seq_builder_add_afe_state().
 *
 * These mirror the three AFECON configurations a measurement sequence steps
 * through: power up the excitation/sense analog path, start conversion
 * (ADC + DFT engine), then power the analog path back down before the next
 * wakeup tick.
 */
enum ad5940_afe_state {
	/** Excitation buffer, TIA, and DAC reference on; no conversion yet. */
	AD5940_AFE_STATE_ANALOG = 0,
	/** Analog path on, plus ADC conversion and DFT engine enabled. */
	AD5940_AFE_STATE_CONV = 1,
	/** Analog path powered down (idle between captures). */
	AD5940_AFE_STATE_OFF = 2,
};

/**
 * @brief Incremental builder for AD5940 sequencer opcodes.
 *
 * Wraps a caller-owned uint32_t buffer and appends one pre-encoded opcode
 * per ad5940_seq_builder_add_*() call, without exposing register addresses
 * or bitfield layouts to the caller. Once built, pass the buffer and
 * ad5940_seq_builder_len() to ad5940_seq_cfg.meas_opcodes/meas_len (or
 * init_opcodes/init_len) and load it with ad5940_seq_load().
 *
 * Not safe for concurrent use on the same instance; build one sequence at a
 * time, typically from application init code before any device opens a
 * stream.
 */
struct ad5940_seq_builder {
	/** @cond INTERNAL_HIDDEN */
	uint32_t *buf;
	uint16_t  cap;
	uint16_t  len;
	int       err;
	/** @endcond */
};

/**
 * @brief Initialize a sequence builder over a caller-provided opcode buffer.
 *
 * @param b   Builder instance to initialize.
 * @param buf Backing storage for encoded opcodes. Must outlive the builder
 *            and remain valid until ad5940_seq_load() returns (the opcodes
 *            are copied into AD5940 SRAM by that call).
 * @param cap Capacity of buf, in 32-bit words.
 */
void ad5940_seq_builder_init(struct ad5940_seq_builder *b, uint32_t *buf, uint16_t cap);

/**
 * @brief Number of opcodes appended so far.
 *
 * @param b Builder instance.
 * @return Opcode count, suitable for ad5940_seq_cfg.meas_len/init_len.
 */
uint16_t ad5940_seq_builder_len(const struct ad5940_seq_builder *b);

/**
 * @brief Check whether any add_*() call has failed.
 *
 * Errors are sticky: once set, further add_*() calls are no-ops. Check this
 * once after building the full sequence rather than after every call.
 *
 * @param b Builder instance.
 * @retval 0       No error.
 * @retval -ENOSPC An add_*() call would have exceeded the buffer capacity.
 */
int ad5940_seq_builder_error(const struct ad5940_seq_builder *b);

/**
 * @brief Append a sequencer wait opcode.
 *
 * @param b            Builder instance.
 * @param sysclk_cycles Number of SYSCLK cycles to wait (0 to 0x3FFFFFFF).
 */
void ad5940_seq_builder_add_wait(struct ad5940_seq_builder *b, uint32_t sysclk_cycles);

/**
 * @brief Append an opcode that transitions the AFE analog/conversion state.
 *
 * @param b     Builder instance.
 * @param state Target state (AD5940_AFE_STATE_ANALOG/_CONV/_OFF).
 */
void ad5940_seq_builder_add_afe_state(struct ad5940_seq_builder *b,
				      enum ad5940_afe_state state);

/**
 * @brief Append an opcode that selects the ADC input mux pair.
 *
 * @param b     Builder instance.
 * @param mux_p Positive input selector (AD5940_DT_ADC_MUXP_* from
 *              <zephyr/dt-bindings/sensor/ad5940.h>).
 * @param mux_n Negative input selector (AD5940_DT_ADC_MUXN_* from
 *              <zephyr/dt-bindings/sensor/ad5940.h>).
 */
void ad5940_seq_builder_add_adc_mux(struct ad5940_seq_builder *b,
				    uint8_t mux_p, uint8_t mux_n);

/**
 * @brief Append an opcode that selects the ADC input mux pair and PGA gain.
 *
 * ADCCON's mux and PGA gain fields share one register, and the sequencer can
 * only write a register in full — ad5940_seq_builder_add_adc_mux() always
 * writes PGA gain as 0 (1x), silently overwriting any other gain previously
 * set on the same register. Use this instead whenever the sequence needs a
 * non-default PGA gain.
 *
 * @param b        Builder instance.
 * @param mux_p    Positive input selector (AD5940_DT_ADC_MUXP_* from
 *                 <zephyr/dt-bindings/sensor/ad5940.h>).
 * @param mux_n    Negative input selector (AD5940_DT_ADC_MUXN_* from
 *                 <zephyr/dt-bindings/sensor/ad5940.h>).
 * @param pga_gain ADC PGA gain index: 0=1x, 1=1.5x, 2=2x, 3=4x, 4=9x (same
 *                 encoding as SENSOR_ATTR_AD5940_ADC_PGA_GAIN).
 */
void ad5940_seq_builder_add_adc_mux_gain(struct ad5940_seq_builder *b,
					 uint8_t mux_p, uint8_t mux_n,
					 uint8_t pga_gain);

/**
 * @brief Append an opcode that configures the ADC filter chain.
 *
 * Covers the ADCFILTERCON fields not reachable through SENSOR_ATTR_AD5940_*
 * attributes: the 50/60 Hz notch filter bypass and the raw ADC output rate.
 * SINC2/SINC3 OSR are included so one opcode fully replaces a hand-encoded
 * ADCFILTERCON write; set them to the values already configured via
 * SENSOR_ATTR_AD5940_ADC_SINC2_OSR/_SINC3_OSR to leave those unaffected.
 *
 * @param b          Builder instance.
 * @param sinc3_osr  SINC3 oversampling selector: 0=OSR5, 1=OSR4, 2=OSR2
 *                   (same encoding as SENSOR_ATTR_AD5940_ADC_SINC3_OSR).
 * @param sinc2_osr  SINC2 oversampling selector, 0 to 11 (same encoding as
 *                   SENSOR_ATTR_AD5940_ADC_SINC2_OSR).
 * @param lpf_bypass true to bypass the 50 Hz/60 Hz notch filter (ADCDAT is
 *                   fed by the SINC3/raw path instead of SINC2DAT).
 * @param sample_rate Raw ADC output rate: 0=1.6 MHz, 1=800 kHz. 800 kHz is
 *                   required when the DFT input data rate is 800 kHz.
 */
void ad5940_seq_builder_add_adc_filter(struct ad5940_seq_builder *b,
				       uint8_t sinc3_osr, uint8_t sinc2_osr,
				       bool lpf_bypass, uint8_t sample_rate);

/**
 * @brief Append opcodes that program the 4-wire switch matrix.
 *
 * Also re-asserts SWCON's source-select bit, since SWCON resets to its
 * power-on mux mode across hibernate and would otherwise ignore these
 * switch settings on the next wakeup tick.
 *
 * @param b   Builder instance.
 * @param dsw D-node switches (bitmask of AD5940_DT_SWD_* from
 *            <zephyr/dt-bindings/sensor/ad5940.h>).
 * @param psw P-node switches (bitmask of AD5940_DT_SWP_*).
 * @param nsw N-node switches (bitmask of AD5940_DT_SWN_*).
 * @param tsw T-node (HSTIA input) switches (bitmask of AD5940_DT_SWT_*).
 */
void ad5940_seq_builder_add_switch_matrix(struct ad5940_seq_builder *b,
					  uint32_t dsw, uint32_t psw,
					  uint32_t nsw, uint32_t tsw);

/**
 * @brief Append an opcode that sets the data FIFO watermark.
 *
 * Unlike hand-encoding a register write, this correctly places the
 * watermark in DATAFIFOTHRES's 11-bit field (1 to 2047 words) regardless of
 * its value — there is no 8-bit encoding limit to work around.
 *
 * @param b     Builder instance.
 * @param words Watermark in 32-bit FIFO words (1 to 2047).
 */
void ad5940_seq_builder_add_fifo_watermark(struct ad5940_seq_builder *b, uint16_t words);

/**
 * @brief Append opcodes that re-arm hibernate at the end of a measurement.
 *
 * Locks the sequencer sleep-control key and sets SEQTRGSLP so the AFE
 * hibernates until the next wakeup-timer tick. Leaves SEQCON untouched —
 * the sequencer itself stays enabled. Typically the last call before
 * finishing a measurement sequence loaded via ad5940_seq_load().
 *
 * @param b Builder instance.
 */
void ad5940_seq_builder_add_sleep(struct ad5940_seq_builder *b);

/**
 * @brief Append an opcode that disables the sequencer (SEQCON = 0).
 *
 * @param b Builder instance.
 */
void ad5940_seq_builder_add_stop(struct ad5940_seq_builder *b);

/**
 * @brief AD5940 analog GPIO pin numbers.
 *
 * The AD5940 has 8 general-purpose analog GPIO pins (GP0–GP7). Their
 * function is selected per-pin via GP0CON (2 bits each). Pin 0 is
 * special: it also carries the INT0 interrupt output.
 */
#define AD5940_GPIO_PIN0  0u /**< Analog GPIO pin 0 (also carries INT0) */
#define AD5940_GPIO_PIN1  1u /**< Analog GPIO pin 1 */
#define AD5940_GPIO_PIN2  2u /**< Analog GPIO pin 2 */
#define AD5940_GPIO_PIN3  3u /**< Analog GPIO pin 3 */
#define AD5940_GPIO_PIN4  4u /**< Analog GPIO pin 4 */
#define AD5940_GPIO_PIN5  5u /**< Analog GPIO pin 5 */
#define AD5940_GPIO_PIN6  6u /**< Analog GPIO pin 6 */
#define AD5940_GPIO_PIN7  7u /**< Analog GPIO pin 7 */

/**
 * @brief AD5940 analog GPIO pin function selectors.
 *
 * Used to build the func_cfg argument to ad5940_gpio_cfg(). Each pin
 * occupies bits [2n+1:2n] in GP0CON. Pin 0 functions are shown as the
 * canonical example; all pins support INT0/TRIG/SYNC/GPIO.
 */
#define AD5940_GPIO_FUNC_INT0  0u /**< Interrupt 0 output */
#define AD5940_GPIO_FUNC_TRIG  1u /**< Sequence trigger signal input */
#define AD5940_GPIO_FUNC_SYNC  2u /**< External device sync output */
#define AD5940_GPIO_FUNC_GPIO  3u /**< General-purpose input/output */

/**
 * @brief Configure AD5940 analog GPIO pins.
 *
 * Sets the function, drive direction, and optional output value for any
 * combination of the 8 AGPIO pins. Call this after driver init and before
 * triggering measurements that rely on external mux control or GPIO-triggered
 * sequences.
 *
 * @param dev        AD5940 device pointer.
 * @param pin        Pin number (AD5940_GPIO_PIN0 … AD5940_GPIO_PIN7).
 * @param func       Pin function (AD5940_GPIO_FUNC_*).
 * @param output_en  true to enable the output driver (required for INT0 and
 *                   GPIO-output mux control).
 * @param input_en   true to enable the input path (required for TRIG function).
 * @param pull_en    true to enable the internal pull resistor on this pin.
 * @param value      Output level when func = AD5940_GPIO_FUNC_GPIO and
 *                   output_en = true.
 *
 * @retval 0          Success.
 * @retval -EIO       SPI communication error.
 */
int ad5940_gpio_cfg(const struct device *dev, uint8_t pin, uint8_t func,
		    bool output_en, bool input_en, bool pull_en, bool value);

/**
 * @brief Set the output level of an AD5940 GPIO pin.
 *
 * Only meaningful when the pin is configured as AD5940_GPIO_FUNC_GPIO with
 * output enabled. Typically used to drive an external analog mux to route
 * signals to the AD5940 inputs depending on the active measurement sequence.
 *
 * @param dev    AD5940 device pointer.
 * @param pin    Pin number (AD5940_GPIO_PIN0 … AD5940_GPIO_PIN7).
 * @param value  Output level: true = high, false = low.
 *
 * @retval 0     Success.
 * @retval -EIO  SPI communication error.
 */
int ad5940_gpio_set(const struct device *dev, uint8_t pin, bool value);

/**
 * @brief Load a sequence into the AD5940 SRAM and register it in a slot.
 *
 * Writes the init and measurement opcodes into contiguous SRAM, programs
 * SEQxINFO with the start address and instruction count, and stores the
 * wakeup/sleep timing. Does NOT start the wakeup timer or modify SEQORDER —
 * call ad5940_seq_start() when ready to begin measurements.
 *
 * The driver places sequences in SRAM in slot order (slot 0 at the base,
 * each subsequent slot after the previous). Total SRAM consumed by all
 * loaded sequences must not exceed the device's sequencer SRAM capacity
 * (6 KB = 1536 32-bit words on AD5940).
 *
 * @param dev  AD5940 device pointer.
 * @param id   Sequencer slot (AD5940_SEQ_0 … AD5940_SEQ_3).
 * @param cfg  Sequence configuration: opcodes, lengths, timing, FIFO source.
 *
 * @retval 0          Success.
 * @retval -EINVAL    cfg is NULL, meas_len is 0, or opcodes exceed SRAM.
 * @retval -EIO       SPI communication error.
 */
int ad5940_seq_load(const struct device *dev, enum ad5940_seq_id id,
		    const struct ad5940_seq_cfg *cfg);

/**
 * @brief Start periodic measurements using a previously loaded sequence slot.
 *
 * Enables the wakeup timer to trigger the specified sequence slot, sets the
 * FIFO source from the slot's cfg, and arms the sequencer. The slot must have
 * been loaded with ad5940_seq_load() first.
 *
 * @param dev  AD5940 device pointer.
 * @param id   Sequencer slot to activate (AD5940_SEQ_0 … AD5940_SEQ_3).
 *
 * @retval 0       Success.
 * @retval -ENOENT Slot has not been loaded (no valid SRAM content).
 * @retval -EIO    SPI communication error.
 */
int ad5940_seq_start(const struct device *dev, enum ad5940_seq_id id);

/**
 * @brief Stop the active sequence and disable the wakeup timer.
 *
 * Halts the wakeup timer and waits for any in-progress sequencer execution
 * to complete before returning. Safe to call from any context.
 *
 * @param dev  AD5940 device pointer.
 *
 * @retval 0    Always succeeds (errors writing TMRCON are logged but ignored).
 */
int ad5940_seq_stop(const struct device *dev);

/**
 * @brief Trigger a single immediate sequence execution via TRIGSEQ register.
 *
 * Useful for one-shot measurements or GPIO-triggered workflows where the MCU
 * drives timing rather than the AD5940 wakeup timer. Wakes the AFE and
 * enables the sequencer (SEQCON.SEQEN) before triggering, so no separate
 * wake or enable call is needed first — TRIGSEQ alone only queues the
 * trigger and has no effect while the sequencer is disabled.
 *
 * Before writing TRIGSEQ this applies the FIFO data source configured for the
 * slot (ad5940_seq_cfg.fifo_src supplied at ad5940_seq_load()), so alternating
 * MCU-triggered slots with different sources (e.g. an ADC/SINC2 slot and an
 * impedance/DFT slot) each read from the correct FIFO source without a separate
 * ad5940_seq_start(). Use ad5940_fifo_source_set() to override ad hoc.
 *
 * @param dev  AD5940 device pointer.
 * @param id   Sequencer slot to trigger (AD5940_SEQ_0 … AD5940_SEQ_3).
 *
 * @retval 0          Success.
 * @retval -ENOENT    Slot has not been loaded (no valid SRAM content).
 * @retval -ETIMEDOUT AFE did not respond to the wakeup attempt.
 * @retval -EIO       SPI communication error.
 */
int ad5940_seq_trigger(const struct device *dev, enum ad5940_seq_id id);

/**
 * @brief Trigger the initialization sequence for a user mode.
 *
 * The driver supports up to four independent measurement "modes" (id 0..3),
 * each carrying its own initialization block and measurement block, both loaded
 * into SRAM by ad5940_seq_load(). Call this to run a mode's init sequence once
 * (typically to configure the switch matrix / ADC mux / AFE peripherals for that
 * mode) before running its measurement sequence with ad5940_measure_sequence().
 *
 * If the mode was loaded with no init block (init_len = 0), this is a no-op and
 * returns 0.
 *
 * Internally the driver keeps all modes resident in SRAM and re-points a single
 * hardware sequencer slot (SEQ1) at the requested mode's init block just before
 * issuing TRIGSEQ, so the caller need not manage hardware slots directly. Wakes
 * the AFE before triggering, so no separate wake call is needed first.
 *
 * @param dev  AD5940 device pointer.
 * @param id   User mode to initialize (AD5940_SEQ_0 … AD5940_SEQ_3).
 *
 * @retval 0          Success (including the no-init-block no-op case).
 * @retval -ENOENT    Mode has not been loaded.
 * @retval -ETIMEDOUT AFE did not respond to the wakeup attempt.
 * @retval -EIO       SPI communication error.
 */
int ad5940_init_sequence(const struct device *dev, enum ad5940_seq_id id);

/**
 * @brief Trigger the measurement sequence for a user mode.
 *
 * Runs the measurement block of the given mode (see ad5940_init_sequence() for
 * the per-mode model). Before triggering, this selects the FIFO data source
 * configured for the mode at load time (ad5940_seq_cfg.fifo_src), so alternating
 * modes with different sources (e.g. an ADC/SINC2 mode and an impedance/DFT
 * mode) each read from the correct source.
 *
 * Typical use pairs it with the init call: ad5940_init_sequence(dev, id) once,
 * then ad5940_measure_sequence(dev, id) per measurement.
 *
 * Internally the driver re-points hardware sequencer slot SEQ0 at the requested
 * mode's measurement block just before issuing TRIGSEQ. Wakes the AFE before
 * triggering, so no separate wake call is needed first.
 *
 * @param dev  AD5940 device pointer.
 * @param id   User mode to measure (AD5940_SEQ_0 … AD5940_SEQ_3).
 *
 * @retval 0          Success.
 * @retval -ENOENT    Mode has not been loaded.
 * @retval -ETIMEDOUT AFE did not respond to the wakeup attempt.
 * @retval -EIO       SPI communication error.
 */
int ad5940_measure_sequence(const struct device *dev, enum ad5940_seq_id id);

/**
 * @brief Set the data FIFO source explicitly.
 *
 * Runtime override of the FIFO data source, independent of the per-slot source
 * applied by ad5940_seq_start()/ad5940_seq_trigger(). Use for ad-hoc
 * measurements or to force a source before a manual acquisition.
 *
 * @param dev  AD5940 device pointer.
 * @param src  One of AD5940_PUBLIC_FIFOSRC_ADC / _DFT / _SINC2 / _VARIANCE /
 *             _MEAN (1..5). 0 (disabled) and out-of-range values are rejected.
 *
 * @retval 0        Success.
 * @retval -EINVAL  src is 0 or greater than AD5940_PUBLIC_FIFOSRC_MEAN.
 * @retval -EIO     SPI communication error.
 */
int ad5940_fifo_source_set(const struct device *dev, uint8_t src);

/**
 * @brief Perform a hardware reset via the RESET GPIO pin.
 *
 * Asserts the reset pin active-low for at least 200 µs, then deasserts and
 * waits 1 ms for the AFE to return to its power-on state. After this call the
 * AFE is in hardware-reset state; the caller must re-initialise the driver
 * (or call the internal sys_init_sequence equivalent) before using it again.
 *
 * If no reset-gpios DT property is present, returns -ENOTSUP immediately.
 *
 * @param dev  AD5940 device pointer.
 *
 * @retval 0         Success.
 * @retval -ENOTSUP  No reset GPIO configured in DT.
 * @retval -EIO      GPIO driver error.
 */
int ad5940_hw_reset(const struct device *dev);

/**
 * @brief Custom sensor channels for AD5940/AD5941.
 *
 * All channels start at SENSOR_CHAN_PRIV_START to avoid collision
 * with standard Zephyr sensor channels.
 */
enum sensor_channel_ad5940 {
	/** EIS impedance real part (raw 18-bit DFT count) */
	SENSOR_CHAN_AD5940_IMPEDANCE_REAL = SENSOR_CHAN_PRIV_START,
	/** EIS impedance imaginary part (raw 18-bit DFT count) */
	SENSOR_CHAN_AD5940_IMPEDANCE_IMAG,
	/** EIS impedance magnitude |Z| in Ohms */
	SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE,
	/** EIS impedance phase angle in radians */
	SENSOR_CHAN_AD5940_IMPEDANCE_PHASE,
	/** Amperometry current in nA */
	SENSOR_CHAN_AD5940_CURRENT,
	/** CV potential in mV */
	SENSOR_CHAN_AD5940_POTENTIAL,
	/** Raw ADC 16-bit unsigned output */
	SENSOR_CHAN_AD5940_ADC_RAW,
	/** Raw DFT word-pair (val1=real, val2=imag; calibration/debug) */
	SENSOR_CHAN_AD5940_DFT,
	/** SINC2+Notch filter output (16-bit, amperometry/ADC modes) */
	SENSOR_CHAN_AD5940_SINC2,
	/** Statistical variance output (when stat block enabled) */
	SENSOR_CHAN_AD5940_VARIANCE,
	/** Statistical mean output (when stat block enabled) */
	SENSOR_CHAN_AD5940_MEAN,
};

/**
 * @brief Custom sensor attributes for AD5940/AD5941.
 *
 * All attributes start at SENSOR_ATTR_PRIV_START to avoid collision
 * with standard Zephyr sensor attributes.
 */
enum sensor_attribute_ad5940 {
	/** Set measurement mode (val1 = enum ad5940_mode) */
	SENSOR_ATTR_AD5940_MODE = SENSOR_ATTR_PRIV_START,

	/* ---- EIS / DFT configuration ---- */
	/** Current EIS excitation frequency in Hz (val1 = int Hz, val2 = frac µHz) */
	SENSOR_ATTR_AD5940_EIS_FREQ_HZ,
	/** EIS sweep start frequency in Hz */
	SENSOR_ATTR_AD5940_EIS_FREQ_START_HZ,
	/** EIS sweep stop frequency in Hz */
	SENSOR_ATTR_AD5940_EIS_FREQ_STOP_HZ,
	/** Number of sweep frequency points (2 to AD5940_MAX_SWEEP_POINTS) */
	SENSOR_ATTR_AD5940_EIS_FREQ_POINTS,
	/** Frequency spacing: 1 = logarithmic, 0 = linear */
	SENSOR_ATTR_AD5940_EIS_FREQ_LOG_SCALE,
	/** Excitation amplitude in mVpp (1 to 800) */
	SENSOR_ATTR_AD5940_EIS_AMPLITUDE_MVPP,
	/** DFT settling cycles (1 to 255) */
	SENSOR_ATTR_AD5940_EIS_SETTLING_CYCLES,
	/**
	 * DFT point count selector (no-OS DftNum).
	 * val1 encodes the register field: 0=4, 1=8, 2=16, ... 11=8192, 12=16384.
	 * Use 11 (8192 pts) for EIS below 1 kHz; reduces spectral leakage.
	 */
	SENSOR_ATTR_AD5940_DFT_NUM,
	/**
	 * DFT input source (no-OS DftSrc).
	 * 0=SINC2+Notch, 1=SINC3, 2=ADC raw, 3=SINC3 average.
	 */
	SENSOR_ATTR_AD5940_DFT_SRC,
	/** Hanning window enable (val1: 0=off, 1=on). Reduces spectral leakage. */
	SENSOR_ATTR_AD5940_HANNING_WIN,

	/* ---- HSTIA configuration ---- */
	/** HSTIA internal RTIA selector (0=200Ω, 1=1kΩ, 2=5kΩ, 3=10kΩ, ... 7=160kΩ, 8=open) */
	SENSOR_ATTR_AD5940_RTIA_SEL,
	/**
	 * HSTIA CTIA bypass capacitor selector (no-OS CtiaSel, 0–31).
	 * Each step adds ~2 pF in parallel with RTIA. Use 0 for fastest settling.
	 */
	SENSOR_ATTR_AD5940_CTIA_SEL,

	/* ---- DAC excitation chain ---- */
	/**
	 * Excitation buffer gain (no-OS ExcitBufGain).
	 * 0=2× (default, full output swing), 1=0.25× (attenuated, small signals).
	 */
	SENSOR_ATTR_AD5940_EXCIT_BUF_GAIN,
	/**
	 * HSDAC output gain (no-OS HsDacGain).
	 * 0=1× (default), 1=0.2× (attenuated).
	 */
	SENSOR_ATTR_AD5940_HSDAC_GAIN,
	/**
	 * HSDAC update rate divider (no-OS HsDacUpdateRate, 7–255).
	 * DAC update rate = SysClk / (HsDacUpdateRate + 1).
	 * Lower = faster DAC updates (better high-freq fidelity); default 7.
	 */
	SENSOR_ATTR_AD5940_HSDAC_UPDATE_RATE,

	/* ---- ADC / filter configuration ---- */
	/** SINC2 oversampling ratio selector (0=22, 1=44, 2=89, 3=178, ...) */
	SENSOR_ATTR_AD5940_ADC_SINC2_OSR,
	/**
	 * SINC3 oversampling ratio selector (no-OS ADCSinc3Osr).
	 * 0=OSR5, 1=OSR4, 2=OSR2. Use 2 (OSR2) for EIS; gives 400 kHz output.
	 */
	SENSOR_ATTR_AD5940_ADC_SINC3_OSR,
	/** ADC PGA gain index (0=1x, 1=1.5x, 2=2x, 3=4x, 4=9x) */
	SENSOR_ATTR_AD5940_ADC_PGA_GAIN,
	/** Output data rate in Hz */
	SENSOR_ATTR_AD5940_ODR_HZ,
	/**
	 * Statistics engine (STATSCON) enable + sample count.
	 * val1 = STATSSAMPLE code (0=off, 1=4, 2=8, 3=16, 4=32, 5=64, 6=128
	 * samples per mean/variance block). Non-zero enables the engine and
	 * populates the MEAN / VARIANCE channels.
	 */
	SENSOR_ATTR_AD5940_STATS_ENABLE,

	/* ---- FIFO configuration ---- */
	/**
	 * Data FIFO watermark in 32-bit words (DATAFIFOTHRES.HIGHTHRES, 1 to
	 * 2047). Applied on the next ad5940_seq_start()/seq_trigger()/
	 * measure_sequence() or ad5940_fifo_source_set() call. Non-EIS stream
	 * reads wait for this many words per buffer (see
	 * ad5940_submit_stream()); EIS mode always uses a fixed 4-word frame
	 * regardless of this setting.
	 */
	SENSOR_ATTR_AD5940_FIFO_WATERMARK,
	/**
	 * Data FIFO SRAM size selector (val1: 1=2kB, 2=4kB, 3=6kB). Command
	 * (sequencer) SRAM is the complementary share of the fixed 6 kB total;
	 * selecting 6 kB here leaves no SRAM for user-loaded sequences.
	 */
	SENSOR_ATTR_AD5940_FIFO_SIZE,
	/**
	 * Data FIFO mode selector (val1: 2=FIFO mode, 3=stream/overwrite mode).
	 */
	SENSOR_ATTR_AD5940_FIFO_MODE,

	/* ---- Power mode ---- */
	/**
	 * AFE power mode (no-OS PwrMod).
	 * 0=low-power (≤80 kHz, 16 MHz HFOSC), 1=high-power (>80 kHz, 32 MHz).
	 */
	SENSOR_ATTR_AD5940_POWER_MODE,

	/* ---- Amperometry configuration ---- */
	/** DC bias potential in mV (200 to 2200) */
	SENSOR_ATTR_AD5940_BIAS_MV,
	/** Low-power amplifier RTIA selector */
	SENSOR_ATTR_AD5940_LPAMP_RTIA,

	/* ---- Cyclic voltammetry configuration ---- */
	/** First vertex potential in mV (200 to 2200) */
	SENSOR_ATTR_AD5940_CV_VERTEX1_MV,
	/** Second vertex potential in mV (200 to 2200, must differ from VERTEX1) */
	SENSOR_ATTR_AD5940_CV_VERTEX2_MV,
	/** Initial potential in mV (200 to 2200) */
	SENSOR_ATTR_AD5940_CV_INIT_MV,
	/** CV scan rate in mV/s (val1 = integer, val2 = fractional micro-mV/s) */
	SENSOR_ATTR_AD5940_CV_SCAN_RATE_MV_PER_S,
	/** Number of complete CV cycles to run (1 to 255) */
	SENSOR_ATTR_AD5940_CV_CYCLES,

	/** Software reset (write any value to trigger SW reset + sys init) */
	SENSOR_ATTR_AD5940_SW_RESET,
	/** Trigger RTIA recalibration immediately (write any value) */
	SENSOR_ATTR_AD5940_RECAL,

	/* ---- Read-only device information ---- */
	/** Chip ID (read-only, returns 0x5500–0x5502 for AD5940) */
	SENSOR_ATTR_AD5940_CHIP_ID,
	/** ADI company ID (read-only, returns 0x4144) */
	SENSOR_ATTR_AD5940_ADI_ID,
	/** RTIA calibration magnitude in Ohms (read-only, after calibration) */
	SENSOR_ATTR_AD5940_RTIA_CAL_MAGNITUDE,
	/** RTIA calibration phase in radians (read-only, after calibration) */
	SENSOR_ATTR_AD5940_RTIA_CAL_PHASE,
	/**
	 * FIFO word count (read-only). Returns the number of 32-bit words
	 * currently in the AD5940 data FIFO (live FIFOCNTSTA reading) in val1.
	 */
	SENSOR_ATTR_AD5940_FIFO_COUNT,
	/**
	 * Excitation frequency (Hz) of the most recently emitted stream buffer
	 * (read-only), returned via sensor_value_from_double() in val1/val2.
	 *
	 * Use this to label a streamed EIS reading with the frequency that
	 * produced it. Unlike SENSOR_ATTR_AD5940_EIS_FREQ_HZ — which returns the
	 * NEXT sweep point, because the stream callback advances the sweep at its
	 * end — this reflects the point that was just measured. 0 before the
	 * first stream buffer is emitted.
	 */
	SENSOR_ATTR_AD5940_EIS_LAST_FREQ_HZ,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_AD5940_H_ */
