/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private definitions of the ADE9000 driver. Independent of
 *        DT_DRV_COMPAT, so struct ade9000_config has the same layout in every
 *        source file.
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ade9000.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

/**
 * @brief         Index of a custom channel in the channel table.
 * @param _chan   Custom channel, see enum sensor_channel_ade9000
 */
#define ADE9000_CHAN_IDX(_chan) ((int)(_chan) - (int)SENSOR_CHAN_PRIV_START)

/**
 * @brief  Number of entries in the channel table.
 */
#define ADE9000_CHAN_COUNT ADE9000_CHAN_IDX(SENSOR_CHAN_ADE9000_MAX)

/**
 * @brief  Flag in ade9000_data.irq_pending: IRQ0 waits to be serviced.
 */
#define ADE9000_PENDING_IRQ0 BIT(0)

/**
 * @brief  Flag in ade9000_data.irq_pending: IRQ1 waits to be serviced.
 */
#define ADE9000_PENDING_IRQ1 BIT(1)

/**
 * @brief         Three consecutive slots for phase A, B and C, e.g.
 *                ADE9000_SLOT_ABC(IRMS) gives ADE9000_SLOT_AIRMS,
 *                ADE9000_SLOT_BIRMS and ADE9000_SLOT_CIRMS.
 * @param _name   Register name without the phase letter
 */
#define ADE9000_SLOT_ABC(_name) ADE9000_SLOT_A##_name, ADE9000_SLOT_B##_name, ADE9000_SLOT_C##_name

/**
 * @brief  Number of slots that are read with one burst, AV_PCF_1 to
 *         NIRMS1012_1. The slots behind them are read one by one.
 */
#define ADE9000_SLOT_BURST_COUNT ADE9000_SLOT_ISUMRMS

/**
 * @brief  Current gains per phase: xIGAIN plus the multipoint gains xIGAIN0 to
 *         xIGAIN4.
 */
#define ADE9000_IGAIN_COUNT (1 + ADE9000_MT_REGION_COUNT)

/**
 * @brief  1 if any instance has reset-gpios in the devicetree, which adds the
 *         reset line to struct ade9000_config.
 */
#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(adi_ade9000, reset_gpios)
#define ADE9000_RESET_GPIO 1
#else
#define ADE9000_RESET_GPIO 0
#endif

/**
 * @brief  Offset registers, in the order of the offsets in the config. The
 *         entries before ADE9000_OS_NIRMS exist once per phase.
 */
enum ade9000_os {
	/** xIRMSOS, total current rms */
	ADE9000_OS_IRMS,
	/** xVRMSOS, total voltage rms */
	ADE9000_OS_VRMS,
	/** xIFRMSOS, fundamental current rms */
	ADE9000_OS_IFRMS,
	/** xVFRMSOS, fundamental voltage rms */
	ADE9000_OS_VFRMS,
	/** xIRMSONEOS, one cycle current rms */
	ADE9000_OS_IRMSONE,
	/** xVRMSONEOS, one cycle voltage rms */
	ADE9000_OS_VRMSONE,
	/** xIRMS1012OS, 10/12 cycle current rms */
	ADE9000_OS_IRMS1012,
	/** xVRMS1012OS, 10/12 cycle voltage rms */
	ADE9000_OS_VRMS1012,
	/** xWATTOS, total active power */
	ADE9000_OS_WATT,
	/** xVAROS, total reactive power */
	ADE9000_OS_VAR,
	/** xFWATTOS, fundamental active power */
	ADE9000_OS_FWATT,
	/** xFVAROS, fundamental reactive power */
	ADE9000_OS_FVAR,
	/** NIRMSOS, neutral current rms */
	ADE9000_OS_NIRMS,
	/** NIRMSONEOS, one cycle neutral current rms */
	ADE9000_OS_NIRMSONE,
	/** NIRMS1012OS, 10/12 cycle neutral current rms */
	ADE9000_OS_NIRMS1012,
	/** ISUMRMSOS, rms of the current sum */
	ADE9000_OS_ISUMRMS,
	/** Number of offset registers, not an offset */
	ADE9000_OS_COUNT,
	/** No offset register */
	ADE9000_OS_NONE = 0xFF,
};

/**
 * @brief  Registers in the sample buffer. The slots before ISUMRMS follow the
 *         burst block AV_PCF_1 to NIRMS1012_1 and must not be reordered. The
 *         slots from ANGL_VA_VB on are 16 bit registers.
 */
enum ade9000_slot {
	/** AV_PCF to CV_PCF, instantaneous voltages */
	ADE9000_SLOT_ABC(V_PCF),
	/** NI_PCF, instantaneous neutral current */
	ADE9000_SLOT_NI_PCF,
	/** AI_PCF to CI_PCF, instantaneous currents */
	ADE9000_SLOT_ABC(I_PCF),
	/** AIRMS to CIRMS, total current rms */
	ADE9000_SLOT_ABC(IRMS),
	/** AVRMS to CVRMS, total voltage rms */
	ADE9000_SLOT_ABC(VRMS),
	/** NIRMS, neutral current rms */
	ADE9000_SLOT_NIRMS,
	/** AWATT to CWATT, total active power */
	ADE9000_SLOT_ABC(WATT),
	/** AVA to CVA, total apparent power */
	ADE9000_SLOT_ABC(VA),
	/** AVAR to CVAR, total reactive power */
	ADE9000_SLOT_ABC(VAR),
	/** AFVAR to CFVAR, fundamental reactive power */
	ADE9000_SLOT_ABC(FVAR),
	/** APF to CPF, power factor */
	ADE9000_SLOT_ABC(PF),
	/** AVTHD to CVTHD, voltage THD */
	ADE9000_SLOT_ABC(VTHD),
	/** AITHD to CITHD, current THD */
	ADE9000_SLOT_ABC(ITHD),
	/** AFWATT to CFWATT, fundamental active power */
	ADE9000_SLOT_ABC(FWATT),
	/** AFVA to CFVA, fundamental apparent power */
	ADE9000_SLOT_ABC(FVA),
	/** AIFRMS to CIFRMS, fundamental current rms */
	ADE9000_SLOT_ABC(IFRMS),
	/** AVFRMS to CVFRMS, fundamental voltage rms */
	ADE9000_SLOT_ABC(VFRMS),
	/** AIRMSONE to CIRMSONE, one cycle current rms */
	ADE9000_SLOT_ABC(IRMSONE),
	/** AVRMSONE to CVRMSONE, one cycle voltage rms */
	ADE9000_SLOT_ABC(VRMSONE),
	/** NIRMSONE, one cycle neutral current rms */
	ADE9000_SLOT_NIRMSONE,
	/** AIRMS1012 to CIRMS1012, 10/12 cycle current rms */
	ADE9000_SLOT_ABC(IRMS1012),
	/** AVRMS1012 to CVRMS1012, 10/12 cycle voltage rms */
	ADE9000_SLOT_ABC(VRMS1012),
	/** NIRMS1012, 10/12 cycle neutral current rms. Last slot of the burst */
	ADE9000_SLOT_NIRMS1012,
	/** ISUMRMS, rms of the current sum. First slot read one by one */
	ADE9000_SLOT_ISUMRMS,
	/** APERIOD to CPERIOD, line period */
	ADE9000_SLOT_ABC(PERIOD),
	/** AMTREGION to CMTREGION, active multipoint region */
	ADE9000_SLOT_ABC(MTREGION),
	/** ANGL_VA_VB, angle between VA and VB. 16 bit */
	ADE9000_SLOT_ANGL_VA_VB,
	/** ANGL_VB_VC, angle between VB and VC. 16 bit */
	ADE9000_SLOT_ANGL_VB_VC,
	/** ANGL_VA_VC, angle between VA and VC. 16 bit */
	ADE9000_SLOT_ANGL_VA_VC,
	/** ANGL_VA_IA, angle between VA and IA. 16 bit */
	ADE9000_SLOT_ANGL_VA_IA,
	/** ANGL_VB_IB, angle between VB and IB. 16 bit */
	ADE9000_SLOT_ANGL_VB_IB,
	/** ANGL_VC_IC, angle between VC and IC. 16 bit */
	ADE9000_SLOT_ANGL_VC_IC,
	/** ANGL_IA_IB, angle between IA and IB. 16 bit */
	ADE9000_SLOT_ANGL_IA_IB,
	/** ANGL_IB_IC, angle between IB and IC. 16 bit */
	ADE9000_SLOT_ANGL_IB_IC,
	/** ANGL_IA_IC, angle between IA and IC. 16 bit */
	ADE9000_SLOT_ANGL_IA_IC,
	/** Number of slots, not a slot */
	ADE9000_SLOT_COUNT,
};

/**
 * @brief  Energy registers in the order of their addresses, xWATTHR_HI to
 *         xFVAHR_HI. The index is also the order of the energy channels.
 */
enum ade9000_energy {
	/** xWATTHR_HI, total active energy */
	ADE9000_ENERGY_WATTHR,
	/** xVARHR_HI, total reactive energy */
	ADE9000_ENERGY_VARHR,
	/** xVAHR_HI, total apparent energy */
	ADE9000_ENERGY_VAHR,
	/** xFWATTHR_HI, fundamental active energy */
	ADE9000_ENERGY_FWATTHR,
	/** xFVARHR_HI, fundamental reactive energy */
	ADE9000_ENERGY_FVARHR,
	/** xFVAHR_HI, fundamental apparent energy */
	ADE9000_ENERGY_FVAHR,
	/** Number of energy registers per phase, not an energy */
	ADE9000_ENERGY_COUNT,
};

/**
 * @brief  Constant configuration of an instance, from the devicetree.
 *         Calibration values are raw register codes.
 */
struct ade9000_config {
	/** SPI bus and chip select */
	struct spi_dt_spec spi;
#if ADE9000_RESET_GPIO
	/** Reset line, port is NULL without reset-gpios */
	struct gpio_dt_spec reset;
#endif
#ifdef CONFIG_ADE9000_TRIGGER
	/** IRQ0 line, port is NULL without irq0-gpios */
	struct gpio_dt_spec irq0;
	/** IRQ1 line, port is NULL without irq1-gpios */
	struct gpio_dt_spec irq1;
#endif
	/** xIGAIN and xIGAIN0 to xIGAIN4 per phase */
	int32_t igain[ADE9000_PHASE_COUNT][ADE9000_IGAIN_COUNT];
	/** xPHCAL0 to xPHCAL4 per phase */
	int32_t phcal[ADE9000_PHASE_COUNT][ADE9000_MT_REGION_COUNT];
	/** xVGAIN per phase */
	int32_t vgain[ADE9000_PHASE_COUNT];
	/** xPGAIN per phase */
	int32_t pgain[ADE9000_PHASE_COUNT];
	/** NIGAIN */
	int32_t nigain;
	/** NPHCAL */
	int32_t nphcal;
	/** Offset registers in the order of enum ade9000_os, per phase */
	int32_t os[ADE9000_OS_COUNT][ADE9000_PHASE_COUNT];
	/** Voltage at the full-scale input in uV, scales the voltage channels */
	uint32_t voltage_fs_uv;
	/** Current at the full-scale input in uA, scales the current channels */
	uint32_t current_fs_ua;
	/** Nominal line frequency in Hz, 50 or 60 */
	uint8_t line_frequency;
	/** Multipoint compensation on, writes the MTTHR thresholds at init */
	bool multi_point_compensation;
	/** CONFIG0 register value */
	uint32_t config0;
	/** CONFIG1 register value */
	uint16_t config1;
	/** CONFIG2 register value */
	uint16_t config2;
	/** ACCMODE register value */
	uint16_t accmode;
	/** EP_CFG register value */
	uint16_t ep_cfg;
	/** PGA_GAIN register value */
	uint16_t pga_gain;
	/** CHNL_DIS register value, the disabled ADCs */
	uint16_t chnl_dis;
	/** EGY_TIME register value, energy update interval */
	uint16_t egy_time;
	/** VNOM register value, nominal voltage for the apparent power */
	uint32_t vnom;
	/** Total reactive power calculation off (VAR_DIS) */
	bool var_dis;
	/** STATUS0 events also on IRQ1 (CONFIG1.IRQ0_ON_IRQ1) */
	bool irq0_on_irq1;
	/** MTTHR_L0 to MTTHR_L4 as xIRMS codes */
	uint32_t mtthr_low[ADE9000_MT_REGION_COUNT];
	/** MTTHR_H0 to MTTHR_H4 as xIRMS codes */
	uint32_t mtthr_high[ADE9000_MT_REGION_COUNT];
};

/**
 * @brief  Run time data of an instance.
 */
struct ade9000_data {
	/** Protects the samples, the energy counters and the status */
	struct k_mutex lock;
	/** Raw register values of the last fetch, see enum ade9000_slot */
	uint32_t samples[ADE9000_SLOT_COUNT];
	/** TEMP_RSLT of the last fetch */
	uint16_t temp_rslt;
	/** TEMP_GAIN from TEMP_TRIM, read at init */
	uint16_t temp_gain;
	/** TEMP_OFFSET from TEMP_TRIM, read at init */
	uint16_t temp_offset;
	/** Line frequency in Hz, follows ACCMODE.SELFREQ. The angle conversion
	 *  uses it for a phase without zero crossings
	 */
	uint8_t line_frequency;
	/** Energy since init in xWATTHR_HI codes, per type and phase */
	int64_t energy[ADE9000_ENERGY_COUNT][ADE9000_PHASE_COUNT];
	/** Energy register values of the last fetch, per type and phase */
	uint32_t energy_last[ADE9000_ENERGY_COUNT][ADE9000_PHASE_COUNT];
	/** Start values set with SENSOR_ATTR_OFFSET in uWh, uvarh or uVAh */
	int64_t energy_base_u[ADE9000_ENERGY_COUNT][ADE9000_PHASE_COUNT];
	/** STATUS0 as read and cleared last */
	uint32_t status0;
	/** STATUS1 as read and cleared last */
	uint32_t status1;
#ifdef CONFIG_ADE9000_TRIGGER
	/** Back reference for the GPIO callbacks and the work item */
	const struct device *dev;
	/** GPIO callback of IRQ0 */
	struct gpio_callback irq0_cb;
	/** GPIO callback of IRQ1 */
	struct gpio_callback irq1_cb;
	/** ADE9000_PENDING_IRQ0 and ADE9000_PENDING_IRQ1 of lines to service */
	atomic_t irq_pending;
	/** Application handler of SENSOR_TRIG_ADE9000_IRQ0, may be NULL */
	sensor_trigger_handler_t irq0_handler;
	/** Trigger the IRQ0 handler was installed for */
	const struct sensor_trigger *irq0_trig;
	/** Application handler of SENSOR_TRIG_ADE9000_IRQ1, may be NULL */
	sensor_trigger_handler_t irq1_handler;
	/** Trigger the IRQ1 handler was installed for */
	const struct sensor_trigger *irq1_trig;
#if defined(CONFIG_ADE9000_TRIGGER_OWN_THREAD)
	/** Stack of the interrupt thread */
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADE9000_THREAD_STACK_SIZE);
	/** Interrupt thread */
	struct k_thread thread;
	/** Wakes the interrupt thread */
	struct k_sem irq_sem;
#elif defined(CONFIG_ADE9000_TRIGGER_GLOBAL_THREAD)
	/** Work item on the system work queue */
	struct k_work irq_work;
#endif
#endif
};

/**
 * @brief        Read consecutive registers or waveform buffer locations in one transfer.
 * @param dev    ADE9000 device
 * @param addr   Address of the first register
 * @param buf    Takes the raw big endian bytes
 * @param len    Number of bytes to read
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_read(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len);

/**
 * @brief        Read a 16 bit register.
 * @param dev    ADE9000 device
 * @param addr   Register address
 * @param value  Takes the register value
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_reg_read16(const struct device *dev, uint16_t addr, uint16_t *value);

/**
 * @brief        Read a 32 bit register.
 * @param dev    ADE9000 device
 * @param addr   Register address
 * @param value  Takes the register value
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_reg_read32(const struct device *dev, uint16_t addr, uint32_t *value);

/**
 * @brief        Write a 16 bit register.
 * @param dev    ADE9000 device
 * @param addr   Register address
 * @param value  Register value
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_reg_write16(const struct device *dev, uint16_t addr, uint16_t value);

/**
 * @brief        Write a 32 bit register.
 * @param dev    ADE9000 device
 * @param addr   Register address
 * @param value  Register value
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_reg_write32(const struct device *dev, uint16_t addr, uint32_t value);

/**
 * @brief         Read and clear a status register and keep its value.
 * @param dev     ADE9000 device
 * @param reg     ADE9000_REG_STATUS0 or ADE9000_REG_STATUS1
 * @param status  Takes the value, data->status0 or data->status1
 * @return        0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_status_update(const struct device *dev, uint16_t reg, uint32_t *status);

/**
 * @brief        Set or read the start value of an energy channel.
 * @param dev    ADE9000 device
 * @param chan   Energy channel
 * @param val    ADE9000_PHASE_COUNT values in Wh, varh or VAh
 * @param set    true to set the start value and clear the count, false to read it
 * @return       0 on success, -ENOTSUP for other channels
 */
int ade9000_energy_base(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val, bool set);

/**
 * @brief      Write the calibration and the operating mode from the
 *             devicetree. Expects a freshly reset IC with the DSP stopped.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_configure(const struct device *dev);

/**
 * @brief        Set an attribute. SENSOR_CHAN_ALL takes the operating mode,
 *               mask and register attributes, the calibrated channels take the
 *               gains, offsets and the phase compensation xPHCAL0/NPHCAL, which
 *               overwrite the devicetree values until the next reset. The
 *               multipoint registers come from the devicetree or
 *               ade9000_multipoint_set().
 * @param dev    ADE9000 device
 * @param chan   SENSOR_CHAN_ALL or a calibrated channel
 * @param attr   Attribute
 * @param val    New value, or ADE9000_PHASE_COUNT values for a three phase
 *               channel
 * @return       0 on success, -ENOTSUP for an unsupported combination,
 *               negative errno of the SPI transfer otherwise
 */
int ade9000_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val);

/**
 * @brief        Read back an attribute. Like ade9000_attr_set, plus the
 *               status of the last interrupt.
 * @param dev    ADE9000 device
 * @param chan   SENSOR_CHAN_ALL or a calibrated channel
 * @param attr   Attribute
 * @param val    Takes the value, or ADE9000_PHASE_COUNT values for a three
 *               phase channel
 * @return       0 on success, -ENOTSUP for an unsupported combination,
 *               negative errno of the SPI transfer otherwise
 */
int ade9000_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val);

#ifdef CONFIG_ADE9000_TRIGGER
/**
 * @brief          Install the handler for SENSOR_TRIG_ADE9000_IRQ0 or
 *                 SENSOR_TRIG_ADE9000_IRQ1. The events that fire it are
 *                 selected with SENSOR_ATTR_ADE9000_MASK0/1.
 * @param dev      ADE9000 device
 * @param trig     Trigger, must stay valid while installed
 * @param handler  Handler, NULL removes it
 * @return         0 on success, -ENOTSUP for other triggers or a line that is
 *                 not in the devicetree
 */
int ade9000_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

/**
 * @brief      Set up the interrupt thread or work item and both IRQ lines.
 * @param dev  ADE9000 device
 * @return     0 on success, negative errno otherwise
 */
int ade9000_init_interrupt(const struct device *dev);
#endif

/**
 * @brief        Convert a gain factor into the code of a gain register:
 *               code = (gain - 1) * 2^27.
 * @param val    Gain as a factor, -15.0 up to below 17.0
 * @param code   Takes the register code
 * @return       0 on success, -EINVAL for a factor out of range
 */
int ade9000_gain_to_code(const struct sensor_value *val, int32_t *code);

/**
 * @brief        Convert the code of a gain register into a gain factor.
 * @param code   Register code
 * @param val    Takes the gain as a factor
 * @return       0 on success, -ERANGE if the value does not fit
 */
int ade9000_gain_from_code(int32_t code, struct sensor_value *val);

/**
 * @brief        Convert an angle in degrees into the code of a phase
 *               compensation register, with data sheet equation 4:
 *               xPHCAL = (sin(phi - omega) + sin(omega)) / sin(2 * omega - phi) * 2^27
 * @param dev    ADE9000 device
 * @param val    Angle in degrees, -15 up to 2.25 at 50 Hz or 2.7 at 60 Hz
 * @param code   Takes the register code
 * @return       0 on success, -EINVAL for an angle out of range
 */
int ade9000_phcal_to_code(const struct device *dev, const struct sensor_value *val, int32_t *code);

/**
 * @brief        Convert the code of a phase compensation register into an
 *               angle in degrees. Solves data sheet equation 4 for phi: with
 *               F = xPHCAL * 2^-27, A = F * sin(2 * omega) + sin(omega) and
 *               B = F * cos(2 * omega) + cos(omega) it is
 *               A * cos(phi) - B * sin(phi) = sin(omega), so
 *               phi = acos(sin(omega) / sqrt(A^2 + B^2)) - atan2(B, A).
 * @param dev    ADE9000 device
 * @param code   Register code
 * @param val    Takes the angle in degrees
 * @return       0 on success, -ERANGE if the value does not fit
 */
int ade9000_phcal_from_code(const struct device *dev, int32_t code, struct sensor_value *val);

/**
 * @brief        Convert a current threshold into an xIRMS code:
 *               code = I / I_fs * 52702092.
 * @param dev    ADE9000 device
 * @param val    Current in A
 * @param code   Takes the register code
 * @return       0 on success, -EINVAL for a negative current or one beyond the
 *               register range
 */
int ade9000_mtthr_to_code(const struct device *dev, const struct sensor_value *val, uint32_t *code);

/**
 * @brief        Convert an xIRMS code into a current.
 * @param dev    ADE9000 device
 * @param code   Register code
 * @param val    Takes the current in A
 * @return       0 on success, -ERANGE if the value does not fit
 */
int ade9000_mtthr_from_code(const struct device *dev, uint32_t code, struct sensor_value *val);
