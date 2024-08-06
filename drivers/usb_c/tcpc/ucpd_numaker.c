/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_tcpc

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/drivers/usb_c/usbc_ppc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tcpc_numaker, CONFIG_USBC_LOG_LEVEL);

#include <soc.h>
#include <NuMicro.h>

#include "ucpd_numaker.h"

/* Implementation notes on NuMaker TCPC/PPC/VBUS
 *
 * 1. UTCPD, interfacing to external circuit on VBUS/VCONN voltage measurement,
 *    VBUS/VCONN overcurrent protection, VBUS overvoltage protection, etc.,
 *    can implement all functions defined in TCPC, PPC, and VBUS. For this,
 *    TCPC is implemented in UTCPD majorly; PPC and VBUS rely on TCPC for
 *    their implementation.
 * 2. For VBUS/VCONN voltage measurement, UTCPD is updated periodically
 *    by Timer-trigger EADC. To implement this interconnection, TCPC node_id
 *    will cover UTCPD, EADC, and Timer H/W characteristics of registers,
 *    interrupts, resets, and clocks.
 *    NOTE: EADC and Timer interrupts needn't enable for Timer-triggered EADC.
 *          In BSP sample, they are enabled just for development/debug purpose.
 * 3. About VCONN per PCB
 *    (1) Support only VCONN source, no VCONN sink (like Plug Cable)
 *    (2) Separate pins for VCONN enable on CC1/CC2 (VCNEN1/VCNEN2)
 *    (3) Single pin for VCONN discharge (DISCHG)
 * 4. VBUS discharge precedence
 *    (1) GPIO
 *    (2) UTCPD
 * 5. VCONN discharge precedence
 *    (1) DPM-supplied callback
 *    (2) GPIO
 *    (3) UTCPD
 */

/**
 * @brief Invalid or missing value
 */
#define NUMAKER_INVALID_VALUE UINT32_MAX

/**
 * @brief UTCPD VBUS threshold default in mV
 *
 * These are default values of UTCPD VBUS threshold registers. They need
 * to be reconfigured by taking the following factors into consideration:
 * 1. Analog Vref
 * 2. UTCPD VBVOL.VBSCALE
 */
#define NUMAKER_UTCPD_VBUS_THRESHOLD_OVERVOLTAGE_MV          25000
#define NUMAKER_UTCPD_VBUS_THRESHOLD_VSAFE5V_MV              5000
#define NUMAKER_UTCPD_VBUS_THRESHOLD_VSAFE0V_MV              0
#define NUMAKER_UTCPD_VBUS_THRESHOLD_STOP_FORCE_DISCHARGE_MV 800
#define NUMAKER_UTCPD_VBUS_THRESHOLD_SINK_DISCONNECT_MV      3500

/**
 * @brief SYS register dump
 */
#define NUMAKER_SYS_REG_DUMP(dev, reg_name) LOG_INF("SYS: %8s: 0x%08x", #reg_name, SYS->reg_name);

/**
 * @brief GPIO register dump
 */
#define NUMAKER_GPIO_REG_DUMP(dev, port, reg_name)                                                 \
	LOG_INF("%s: %8s: 0x%08x", #port, #reg_name, port->reg_name);

/**
 * @brief UTCPD register write timeout in microseconds
 */
#define NUMAKER_UTCPD_REG_WRITE_BY_NAME_TIMEOUT_US 20000

/**
 * @brief UTCPD register write by name
 */
#define NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, reg_name, val)                                        \
	({                                                                                         \
		int rc_intern = numaker_utcpd_reg_write_wait_ready(dev);                           \
		if (rc_intern < 0) {                                                               \
			LOG_ERR("UTCPD register (%s) write timeout", #reg_name);                   \
		} else {                                                                           \
			utcpd_base->reg_name = (val);                                              \
		}                                                                                  \
		rc_intern;                                                                         \
	})

/**
 * @brief UTCPD register force write by name
 */
#define NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, reg_name, val)                                  \
	({                                                                                         \
		int rc_intern = numaker_utcpd_reg_write_wait_ready(dev);                           \
		if (rc_intern < 0) {                                                               \
			LOG_ERR("UTCPD register (%s) write timeout, force-write", #reg_name);      \
		}                                                                                  \
		utcpd_base->reg_name = (val);                                                      \
		rc_intern;                                                                         \
	})

/**
 * @brief UTCPD register write by offset
 */
#define NUMAKER_UTCPD_REG_WRITE_BY_OFFSET(dev, reg_offset, val)                                    \
	({                                                                                         \
		int rc_intern = numaker_utcpd_reg_write_wait_ready(dev);                           \
		if (rc_intern < 0) {                                                               \
			LOG_ERR("UTCPD register (0x%04x) write timeout", reg_offset);              \
		} else {                                                                           \
			sys_write32((val), ((uintptr_t)utcpd_base) + reg_offset);                  \
		}                                                                                  \
		rc_intern;                                                                         \
	})

/**
 * @brief UTCPD register force write by offset
 */
#define NUMAKER_UTCPD_REG_FORCE_WRITE_BY_OFFSET(dev, reg_offset, val)                              \
	({                                                                                         \
		int rc_intern = numaker_utcpd_reg_write_wait_ready(dev);                           \
		if (rc_intern < 0) {                                                               \
			LOG_ERR("UTCPD register (0x%04x) write timeout, force-write", reg_offset); \
		}                                                                                  \
		sys_write32((val), ((uintptr_t)utcpd_base) + reg_offset);                          \
		rc_intern;                                                                         \
	})

/**
 * @brief UTCPD register read by name
 */
#define NUMAKER_UTCPD_REG_READ_BY_NAME(dev, reg_name) ({ utcpd_base->reg_name; })

/**
 * @brief UTCPD register read by offset
 */
#define NUMAKER_UTCPD_REG_READ_BY_OFFSET(dev, reg_offset)                                          \
	({ sys_read32(((uintptr_t)utcpd_base) + reg_offset); })

/**
 * @brief UTCPD register dump
 */
#define NUMAKER_UTCPD_REG_DUMP(dev, reg_name)                                                      \
	LOG_INF("UTCPD: %8s: 0x%08x", #reg_name, NUMAKER_UTCPD_REG_READ_BY_NAME(dev, reg_name));

/**
 * @brief Helper to write UTCPD VBUS threshold
 */
#define NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(dev, reg_name, mv_norm)                                 \
	({                                                                                         \
		uint32_t mv_bit;                                                                   \
		mv_bit = numaker_utcpd_vbus_volt_mv2bit(dev, mv_norm);                             \
		mv_bit <<= UTCPD_##reg_name##_##reg_name##_Pos;                                    \
		mv_bit &= UTCPD_##reg_name##_##reg_name##_Msk;                                     \
		NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, reg_name, mv_bit);                            \
	})

/**
 * @brief Helper to read UTCPD VBUS threshold
 */
#define NUMAKER_UTCPD_VBUS_THRESHOLD_READ(dev, reg_name)                                           \
	({                                                                                         \
		uint32_t mv_bit;                                                                   \
		mv_bit = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, reg_name);                            \
		mv_bit &= UTCPD_##reg_name##_##reg_name##_Msk;                                     \
		mv_bit >>= UTCPD_##reg_name##_##reg_name##_Pos;                                    \
		numaker_utcpd_vbus_volt_bit2mv(dev, mv_bit);                                       \
	})

/**
 * @brief Immutable device context
 */
struct numaker_tcpc_config {
	UTCPD_T *utcpd_base;
	EADC_T *eadc_base;
	TIMER_T *timer_base;
	const struct device *clkctrl_dev;
	struct numaker_scc_subsys pcc_utcpd;
	struct numaker_scc_subsys pcc_timer;
	struct reset_dt_spec reset_utcpd;
	struct reset_dt_spec reset_timer;
	void (*irq_config_func_utcpd)(const struct device *dev);
	void (*irq_unconfig_func_utcpd)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	struct {
		struct {
			struct gpio_dt_spec vbus_detect;
			struct gpio_dt_spec vbus_discharge;
			struct gpio_dt_spec vconn_discharge;
		} gpios;

		bool dead_battery;
		struct {
			uint32_t bit;
		} pinpl;
		struct {
			struct {
				uint32_t bit;
				uint32_t value;
			} vbscale;
		} vbvol;
	} utcpd;
	struct {
		const struct adc_dt_spec *spec_vbus;
		const struct adc_dt_spec *spec_vconn;
		/* Rate of timer-triggered voltage measurement (Hz) */
		uint32_t timer_trigger_rate;
		/* Trigger source for measuring VBUS/VCONN voltage */
		uint32_t trgsel_vbus;
		uint32_t trgsel_vconn;
	} eadc;
};

/**
 * @brief Mutable device context
 */
struct numaker_tcpc_data {
	enum tc_rp_value rp;
	bool rx_sop_prime_enabled;

	/* One-slot Rx FIFO */
	bool rx_msg_ready;
	struct pd_msg rx_msg;

	/* The fields below must persist across tcpc_init(). */

	uint32_t vref_mv;

	/* TCPC alert */
	struct {
		tcpc_alert_handler_cb_t handler;
		void *data;
	} tcpc_alert;

	/* PPC event */
	struct {
		usbc_ppc_event_cb_t handler;
		void *data;
	} ppc_event;

	/* DPM supplied */
	struct {
		/* VCONN callback function */
		tcpc_vconn_control_cb_t vconn_cb;
		/* VCONN discharge callback function */
		tcpc_vconn_discharge_cb_t vconn_discharge_cb;
	} dpm;
};

/**
 * @brief Wait ready for next write access to UTCPD register
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_reg_write_wait_ready(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;

	if (!WAIT_FOR((utcpd_base->CLKINFO & UTCPD_CLKINFO_ReadyFlag_Msk),
		      NUMAKER_UTCPD_REG_WRITE_BY_NAME_TIMEOUT_US, NULL)) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief Convert VBUS voltage format from H/W bit to mV
 *
 * The following factors are taken into consideration:
 * 1. Analog Vref
 * 2. UTCPD VBVOL.VBSCALE
 *
 * @note UTCPD VBVOL.VBVOL = MSB 10-bit of EADC DAT.RESULT[11:0],
 *       that is, discarding LSB 2-bit.
 */
static uint32_t numaker_utcpd_vbus_volt_bit2mv(const struct device *dev, uint32_t bit)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;

	__ASSERT_NO_MSG(data->vref_mv);
	return (uint32_t)(((uint64_t)bit) * data->vref_mv * config->utcpd.vbvol.vbscale.value /
			  BIT_MASK(10));
}

/**
 * @brief Convert VBUS voltage format from mV to H/W bit
 *
 * The following factors are taken into consideration:
 * 1. Analog Vref
 * 2. UTCPD VBVOL.VBSCALE
 *
 * @note UTCPD VBVOL.VBVOL = MSB 10-bit of EADC DAT.RESULT[11:0],
 *       that is, discarding LSB 2-bit.
 */
static uint32_t numaker_utcpd_vbus_volt_mv2bit(const struct device *dev, uint32_t mv)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;

	__ASSERT_NO_MSG(data->vref_mv);
	return mv * BIT_MASK(10) / data->vref_mv / config->utcpd.vbvol.vbscale.value;
}

/**
 * @brief UTCPD register dump
 *
 * @retval 0 on success
 */
static int numaker_utcpd_dump_regs(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;

	/* SYS register */
	NUMAKER_SYS_REG_DUMP(dev, VREFCTL);
	NUMAKER_SYS_REG_DUMP(dev, UTCPDCTL);

	/* UTCPD register */
	NUMAKER_UTCPD_REG_DUMP(dev, IS);
	NUMAKER_UTCPD_REG_DUMP(dev, IE);
	NUMAKER_UTCPD_REG_DUMP(dev, PWRSTSIE);
	NUMAKER_UTCPD_REG_DUMP(dev, FUTSTSIE);
	NUMAKER_UTCPD_REG_DUMP(dev, CTL);
	NUMAKER_UTCPD_REG_DUMP(dev, PINPL);
	NUMAKER_UTCPD_REG_DUMP(dev, ROLCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, FUTCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, PWRCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, CCSTS);
	NUMAKER_UTCPD_REG_DUMP(dev, PWRSTS);
	NUMAKER_UTCPD_REG_DUMP(dev, FUTSTS);
	NUMAKER_UTCPD_REG_DUMP(dev, DVCAP1);
	NUMAKER_UTCPD_REG_DUMP(dev, DVCAP2);
	NUMAKER_UTCPD_REG_DUMP(dev, MSHEAD);
	NUMAKER_UTCPD_REG_DUMP(dev, DTRXEVNT);
	NUMAKER_UTCPD_REG_DUMP(dev, VBVOL);
	NUMAKER_UTCPD_REG_DUMP(dev, SKVBDCTH);
	NUMAKER_UTCPD_REG_DUMP(dev, SPDGTH);
	NUMAKER_UTCPD_REG_DUMP(dev, VBAMH);
	NUMAKER_UTCPD_REG_DUMP(dev, VBAML);
	NUMAKER_UTCPD_REG_DUMP(dev, VNDIS);
	NUMAKER_UTCPD_REG_DUMP(dev, VNDIE);
	NUMAKER_UTCPD_REG_DUMP(dev, MUXSEL);
	NUMAKER_UTCPD_REG_DUMP(dev, VCDGCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, ADGTM);
	NUMAKER_UTCPD_REG_DUMP(dev, VSAFE0V);
	NUMAKER_UTCPD_REG_DUMP(dev, VSAFE5V);
	NUMAKER_UTCPD_REG_DUMP(dev, VBOVTH);
	NUMAKER_UTCPD_REG_DUMP(dev, VCPSVOL);
	NUMAKER_UTCPD_REG_DUMP(dev, VCUV);
	NUMAKER_UTCPD_REG_DUMP(dev, PHYCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, FRSRXCTL);
	NUMAKER_UTCPD_REG_DUMP(dev, VCVOL);
	NUMAKER_UTCPD_REG_DUMP(dev, CLKINFO);

	return 0;
}

/**
 * @brief Initializes EADC Vref
 *
 * @retval 0 on success
 */
static int numaker_eadc_vref_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	const struct adc_dt_spec *spec;
	enum adc_reference reference;

	if (data->vref_mv) {
		return 0;
	}

	/* NOTE: Register protection lock will restore automatically. Unlock it again. */
	SYS_UnlockReg();

	/* Analog reference voltage
	 *
	 * NOTE: For Vref being internal, external Vref pin must be floating,
	 *       or it can disturb.
	 */
	spec = config->eadc.spec_vbus ? config->eadc.spec_vbus : config->eadc.spec_vconn;
	if (spec == NULL) {
		return 0;
	}
	/* ADC device ready */
	if (!adc_is_ready_dt(spec)) {
		LOG_ERR("ADC device for VBUS/VCONN not ready");
		return -ENODEV;
	}

	/* ADC channel configuration ready */
	if (!spec->channel_cfg_dt_node_exists) {
		LOG_ERR("ADC channel configuration for VBUS/VCONN not specified");
		return -ENODEV;
	}

	reference = spec->channel_cfg.reference;

	SYS->VREFCTL &= ~SYS_VREFCTL_VREFCTL_Msk;
	if (reference == ADC_REF_EXTERNAL0 || reference == ADC_REF_EXTERNAL1) {
		SYS->VREFCTL |= SYS_VREFCTL_VREF_PIN;
	} else if (reference == ADC_REF_INTERNAL) {
		switch (spec->vref_mv) {
		case 1600:
			SYS->VREFCTL |= SYS_VREFCTL_VREF_1_6V;
			break;
		case 2000:
			SYS->VREFCTL |= SYS_VREFCTL_VREF_2_0V;
			break;
		case 2500:
			SYS->VREFCTL |= SYS_VREFCTL_VREF_2_5V;
			break;
		case 3000:
			SYS->VREFCTL |= SYS_VREFCTL_VREF_3_0V;
			break;
		default:
			LOG_ERR("Invalid Vref voltage");
			return -ENOTSUP;
		}
	} else {
		LOG_ERR("Invalid Vref source");
		return -ENOTSUP;
	}

	data->vref_mv = spec->vref_mv;

	return 0;
}

/**
 * @brief Reads and returns UTCPD VBUS measured in mV
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
int numaker_utcpd_vbus_measure(const struct device *dev, uint32_t *mv)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;

	if (mv == NULL) {
		return -EINVAL;
	}
	*mv = 0;

	if (config->eadc.spec_vbus == NULL) {
		return -ENOTSUP;
	}

	/* Vref */
	rc = numaker_eadc_vref_init(dev);
	if (rc < 0) {
		return rc;
	}

	*mv = NUMAKER_UTCPD_VBUS_THRESHOLD_READ(dev, VBVOL);

	return 0;
}

/**
 * @brief Check if the UTCPD VBUS is present
 *
 * @retval 1 if UTCPD VBUS is present
 * @retval 0 if UTCPD VBUS is not present
 */
int numaker_utcpd_vbus_is_present(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	if (pwrsts & UTCPD_PWRSTS_VBPS_Msk) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Check if the UTCPD VBUS is sourcing
 *
 * @retval 1 if UTCPD VBUS is sourcing
 * @retval 0 if UTCPD VBUS is not sourcing
 */
int numaker_utcpd_vbus_is_source(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	if (pwrsts & (UTCPD_PWRSTS_SRHV_Msk | UTCPD_PWRSTS_SRVB_Msk)) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Check if the UTCPD VBUS is sinking
 *
 * @retval 1 if UTCPD VBUS is sinking
 * @retval 0 if UTCPD VBUS is not sinking
 */
int numaker_utcpd_vbus_is_sink(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	if (pwrsts & UTCPD_PWRSTS_SKVB_Msk) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Enable or disable discharge on UTCPD VBUS
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
int numaker_utcpd_vbus_set_discharge(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	const struct gpio_dt_spec *vbus_discharge_spec = &config->utcpd.gpios.vbus_discharge;
	uint32_t pwrctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRCTL);

	/* Use GPIO VBUS discharge */
	if (vbus_discharge_spec->port != NULL) {
		return gpio_pin_set_dt(vbus_discharge_spec, enable);
	}

	/* Use UTCPD VBUS discharge */
	if (enable) {
		pwrctl |= UTCPD_PWRCTL_FDGEN_Msk;
	} else {
		pwrctl &= ~UTCPD_PWRCTL_FDGEN_Msk;
	}
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PWRCTL, pwrctl);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/**
 * @brief Enable or disable UTCPD BIST test mode
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_bist_test_mode_set_enable(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t ctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CTL);

	/* Enable or not BIST test mode */
	if (enable) {
		ctl |= UTCPD_CTL_BISTEN_Msk;
	} else {
		ctl &= ~UTCPD_CTL_BISTEN_Msk;
	}
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, CTL, ctl);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/**
 * @brief Check if UTCPD BIST test mode is enabled
 *
 * @retval 1 if UTCPD BIST test mode is enabled
 * @retval 0 if UTCPD BIST test mode is not enabled
 */
static int numaker_utcpd_bist_test_mode_is_enabled(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t ctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CTL);

	if (ctl & UTCPD_CTL_BISTEN_Msk) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Clears UTCPD Rx message FIFO
 *
 * @retval 0 on success
 */
static int numaker_utcpd_rx_fifo_clear(const struct device *dev)
{
	struct numaker_tcpc_data *data = dev->data;

	data->rx_msg_ready = false;

	return 0;
}

/**
 * @brief Reads Rx message data from UTCPD
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_rx_read_data(const struct device *dev, uint8_t *rx_data,
				      uint32_t rx_data_size)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t data_rmn = rx_data_size;
	uint8_t *data_pos = rx_data;
	uintptr_t data_reg_offset = offsetof(UTCPD_T, RXDA0);
	uint32_t data_value;

	/* 32-bit aligned */
	while (data_rmn >= 4) {
		data_value = NUMAKER_UTCPD_REG_READ_BY_OFFSET(dev, data_reg_offset);
		sys_put_le32(data_value, data_pos);

		/* Next data */
		data_reg_offset += 4;
		data_pos += 4;
		data_rmn -= 4;
	}

	/* Remaining non-32-bit aligned */
	__ASSERT_NO_MSG(data_rmn < 4);
	if (data_rmn) {
		data_value = NUMAKER_UTCPD_REG_READ_BY_OFFSET(dev, data_reg_offset);
		data_reg_offset += 4;

		switch (data_rmn) {
		case 3:
			sys_put_le24(data_value, data_pos);
			data_pos += 3;
			data_rmn -= 3;
			break;

		case 2:
			sys_put_le16(data_value, data_pos);
			data_pos += 2;
			data_rmn -= 2;
			break;

		case 1:
			*data_pos = data_value;
			data_pos += 1;
			data_rmn -= 1;
			break;
		}
	}

	__ASSERT_NO_MSG(data_rmn == 0);

	return 0;
}

/**
 * @brief Writes Tx message data to UTCPD
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_tx_write_data(const struct device *dev, const uint8_t *tx_data,
				       uint32_t tx_data_size)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t data_rmn = tx_data_size;
	const uint8_t *data_pos = tx_data;
	uint32_t data_reg_offset = offsetof(UTCPD_T, TXDA0);
	uint32_t data_value;

	/* 32-bit aligned */
	while (data_rmn >= 4) {
		data_value = sys_get_le32(data_pos);
		rc = NUMAKER_UTCPD_REG_WRITE_BY_OFFSET(dev, data_reg_offset, data_value);
		if (rc < 0) {
			return rc;
		}

		/* Next data */
		data_pos += 4;
		data_reg_offset += 4;
		data_rmn -= 4;
	}

	/* Remaining non-32-bit aligned */
	__ASSERT_NO_MSG(data_rmn < 4);
	if (data_rmn) {
		switch (data_rmn) {
		case 3:
			data_value = sys_get_le24(data_pos);
			data_pos += 3;
			data_rmn -= 3;
			break;

		case 2:
			data_value = sys_get_le16(data_pos);
			data_pos += 2;
			data_rmn -= 2;
			break;

		case 1:
			data_value = *data_pos;
			data_pos += 1;
			data_rmn -= 1;
			break;
		}
		rc = NUMAKER_UTCPD_REG_WRITE_BY_OFFSET(dev, data_reg_offset, data_value);
		if (rc < 0) {
			return rc;
		}
		data_reg_offset += 4;
	}

	__ASSERT_NO_MSG(data_rmn == 0);

	return 0;
}

/**
 * @brief Enqueues UTCPD Rx message
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_rx_fifo_enqueue(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc = 0;
	uint32_t rxbcnt = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, RXBCNT);
	uint32_t rxftype = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, RXFTYPE);
	uint32_t rxhead = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, RXHEAD);
	uint32_t is = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, IS);
	uint32_t rx_data_size;
	struct pd_msg *msg = &data->rx_msg;

	/* Rx message pending? */
	if (!(is & UTCPD_IS_RXSOPIS_Msk)) {
		goto cleanup;
	}

	/* rxbcnt = 1 (frame type) + 2 (Message Header) + Rx data byte count */
	if (rxbcnt < 3) {
		LOG_ERR("Invalid UTCPD.RXBCNT: %d", rxbcnt);
		rc = -EIO;
		goto cleanup;
	}
	rx_data_size = rxbcnt - 3;

	/* Not support Unchunked Extended Message exceeding PD_CONVERT_PD_HEADER_COUNT_TO_BYTES */
	if (rx_data_size > (PD_MAX_EXTENDED_MSG_LEGACY_LEN + 2)) {
		LOG_ERR("Not support Unchunked Extended Message exceeding "
			"PD_CONVERT_PD_HEADER_COUNT_TO_BYTES: %d",
			rx_data_size);
		rc = -EIO;
		goto cleanup;
	}

	/* Rx FIFO has room? */
	if (data->rx_msg_ready) {
		LOG_WRN("Rx FIFO overflow");
	}

	/* Rx frame type */
	/* NOTE: Needn't extra cast for UTCPD_RXFTYPE.RXFTYPE aligning with pd_packet_type */
	msg->type = (rxftype & UTCPD_RXFTYPE_RXFTYPE_Msk) >> UTCPD_RXFTYPE_RXFTYPE_Pos;

	/* Rx header */
	msg->header.raw_value = (uint16_t)rxhead;

	/* Rx data size */
	msg->len = rx_data_size;

	/* Rx data */
	rc = numaker_utcpd_rx_read_data(dev, msg->data, rx_data_size);
	if (rc < 0) {
		goto cleanup;
	}

	/* Finish enqueue of this Rx message */
	data->rx_msg_ready = true;

cleanup:

	/* This has side effect of clearing UTCPD_RXBCNT and friends. */
	NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, IS, UTCPD_IS_RXSOPIS_Msk);

	return rc;
}

/**
 * @brief Notify TCPC alert
 */
static void numaker_utcpd_notify_tcpc_alert(const struct device *dev, enum tcpc_alert alert)
{
	struct numaker_tcpc_data *data = dev->data;
	tcpc_alert_handler_cb_t alert_handler = data->tcpc_alert.handler;
	void *alert_data = data->tcpc_alert.data;

	if (alert_handler) {
		alert_handler(dev, alert_data, alert);
	}
}

/**
 * @brief Notify PPC event
 */
static void numaker_utcpd_notify_ppc_event(const struct device *dev, enum usbc_ppc_event event)
{
	struct numaker_tcpc_data *data = dev->data;
	usbc_ppc_event_cb_t event_handler = data->ppc_event.handler;
	void *event_data = data->ppc_event.data;

	if (event_handler) {
		event_handler(dev, event_data, event);
	}
}

/**
 * @brief UTCPD ISR
 *
 * @note UTCPD register write cannot be failed, or we may trap in ISR for
 *       interrupt bits not cleared. To avoid that, we use "force-write"
 *       version clear interrupt bits for sure.
 */
static void numaker_utcpd_isr(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t is = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, IS);
	uint32_t futsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, FUTSTS);
	uint32_t vndis = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, VNDIS);
	uint32_t ie = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, IE);
	uint32_t futstsie = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, FUTSTSIE);

	/* CC status changed */
	if (is & UTCPD_IS_CCSCHIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_CC_STATUS);
	}

	/* Power status changed */
	if (is & UTCPD_IS_PWRSCHIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_POWER_STATUS);
	}

	/* Received SOP Message */
	if (is & UTCPD_IS_RXSOPIS_Msk) {
		numaker_utcpd_rx_fifo_enqueue(dev);

		/* Per TCPCI 4.4.5.1 TCPC_CONTROL, BIST Test Mode
		 * Incoming messages enabled by RECEIVE_DETECT result
		 * in GoodCRC response but may not be passed to the TCPM
		 * via Alert. TCPC may temporarily store incoming messages
		 * in the Receive Message Buffer, but this may or may not
		 * result in a Receive SOP* Message Status or a Rx Buffer
		 * Overflow alert.
		 */
		if (numaker_utcpd_bist_test_mode_is_enabled(dev) == 1) {
			numaker_utcpd_rx_fifo_clear(dev);
		} else {
			numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_MSG_STATUS);
		}
	}

	/* Rx buffer overflow */
	if (is & UTCPD_IS_RXOFIS_Msk) {
		LOG_WRN("Rx buffer overflow");
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_RX_BUFFER_OVERFLOW);
	}

	/* Received Hard Reset */
	if (is & UTCPD_IS_RXHRSTIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_HARD_RESET_RECEIVED);
	}

	/* SOP* message transmission not successful, no GoodCRC response received on SOP* message
	 * transmission
	 */
	if (is & UTCPD_IS_TXFALIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_TRANSMIT_MSG_FAILED);
	}

	/* Reset or SOP* message transmission not sent due to incoming receive message */
	if (is & UTCPD_IS_TXDCUDIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_TRANSMIT_MSG_DISCARDED);
	}

	/* Reset or SOP* message transmission successful, GoodCRC response received on SOP* message
	 * transmission
	 */
	if (is & UTCPD_IS_TXOKIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_TRANSMIT_MSG_SUCCESS);
	}

	/* VBUS voltage alarm high */
	if ((is & UTCPD_IS_VBAMHIS_Msk) && (ie & UTCPD_IS_VBAMHIS_Msk)) {
		LOG_WRN("UTCPD VBUS voltage alarm high not addressed, disable the alert");
		ie &= ~UTCPD_IS_VBAMHIS_Msk;
		NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, IE, ie);
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_VBUS_ALARM_HI);
	}

	/* VBUS voltage alarm low */
	if ((is & UTCPD_IS_VBAMLIS_Msk) && (ie & UTCPD_IS_VBAMLIS_Msk)) {
		LOG_WRN("UTCPD VBUS voltage alarm low not addressed, disable the alert");
		ie &= ~UTCPD_IS_VBAMLIS_Msk;
		NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, IE, ie);
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_VBUS_ALARM_LO);
	}

	/* Fault */
	if ((is & UTCPD_IS_FUTIS_Msk) && (futstsie & futsts)) {
		LOG_ERR("UTCPD fault (FUTSTS=0x%08x)", futsts);
		NUMAKER_UTCPD_REG_FORCE_WRITE_BY_OFFSET(dev, offsetof(UTCPD_T, FUTSTS), futsts);
		/* NOTE: FUTSTSIE will restore to default on Hard Reset. We may re-enter
		 *       here and redo mask.
		 */
		LOG_WRN("UTCPD fault (FUTSTS=0x%08x) not addressed, disable fault alert (FUTSTSIE)",
			futsts);
		futstsie &= ~futsts;
		NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, FUTSTSIE, futstsie);
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_FAULT_STATUS);

		/* VBUS overvoltage */
		if (futsts & UTCPD_FUTSTS_VBOVFUT_Msk) {
			if (numaker_utcpd_vbus_is_source(dev)) {
				numaker_utcpd_notify_ppc_event(dev, USBC_PPC_EVENT_SRC_OVERVOLTAGE);
			}

			if (numaker_utcpd_vbus_is_sink(dev)) {
				numaker_utcpd_notify_ppc_event(dev, USBC_PPC_EVENT_SNK_OVERVOLTAGE);
			}
		}

		/* VBUS overcurrent */
		if (futsts & UTCPD_FUTSTS_VBOCFUT_Msk) {
			if (numaker_utcpd_vbus_is_source(dev)) {
				numaker_utcpd_notify_ppc_event(dev, USBC_PPC_EVENT_SRC_OVERCURRENT);
			}
		}
	}

	/* VBUS Sink disconnect threshold crossing has been detected */
	if (is & UTCPD_IS_SKDCDTIS_Msk) {
		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_VBUS_SNK_DISCONNECT);
	}

	/* Vendor defined event detected */
	if (is & UTCPD_IS_VNDIS_Msk) {
		NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, VNDIS, vndis);

		numaker_utcpd_notify_tcpc_alert(dev, TCPC_ALERT_VENDOR_DEFINED);
	}

	NUMAKER_UTCPD_REG_FORCE_WRITE_BY_NAME(dev, IS, is);
}

/**
 * @brief Configures EADC sample module with trigger source, channel, etc.
 */
static int numaker_eadc_smplmod_init(const struct device *dev, const struct adc_dt_spec *spec,
				     uint32_t trgsel)
{
	const struct numaker_tcpc_config *const config = dev->config;
	EADC_T *eadc_base = config->eadc_base;
	uint16_t acquisition_time;
	uint16_t acq_time_unit;
	uint16_t acq_time_value;

	__ASSERT_NO_MSG(spec);

	/* ADC device ready */
	if (!adc_is_ready_dt(spec)) {
		LOG_ERR("ADC device for VBUS/VCONN not ready");
		return -ENODEV;
	}

	/* ADC channel configuration ready */
	if (!spec->channel_cfg_dt_node_exists) {
		LOG_ERR("ADC channel configuration for VBUS/VCONN not specified");
		return -ENODEV;
	}

	acquisition_time = spec->channel_cfg.acquisition_time;
	acq_time_unit = ADC_ACQ_TIME_UNIT(acquisition_time);
	acq_time_value = ADC_ACQ_TIME_VALUE(acquisition_time);
	if (acq_time_unit != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("Invalid acquisition time unit for VBUS/VCONN");
		return -ENOTSUP;
	}

	/* Bind sample module with trigger source and channel */
	EADC_ConfigSampleModule(eadc_base, spec->channel_id, trgsel, spec->channel_id);
	/* Extend sampling time */
	EADC_SetExtendSampleTime(eadc_base, spec->channel_id, acq_time_value);

	return 0;
}

/**
 * @brief Initializes VBUS threshold and monitor
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_vbus_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t vbvol = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, VBVOL);
	uint32_t pwrctl = 0;

	/* UTCPD VBUS scale factor */
	vbvol &= ~UTCPD_VBVOL_VBSCALE_Msk;
	vbvol |= config->utcpd.vbvol.vbscale.bit;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, VBVOL, vbvol);
	if (rc < 0) {
		return rc;
	}

	if (config->eadc.spec_vbus != NULL) {
		/* Vref */
		rc = numaker_eadc_vref_init(dev);
		if (rc < 0) {
			return rc;
		}

		/* UTCPD VBUS overvoltage threshold */
		rc = NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(
			dev, VBOVTH, NUMAKER_UTCPD_VBUS_THRESHOLD_OVERVOLTAGE_MV);
		if (rc < 0) {
			return rc;
		}

		/* UTCPD VBUS vSafe5V threshold */
		rc = NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(dev, VSAFE5V,
							NUMAKER_UTCPD_VBUS_THRESHOLD_VSAFE5V_MV);
		if (rc < 0) {
			return rc;
		}

		/* UTCPD VBUS vSafe0V threshold */
		rc = NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(dev, VSAFE0V,
							NUMAKER_UTCPD_VBUS_THRESHOLD_VSAFE0V_MV);
		if (rc < 0) {
			return rc;
		}

		/* UTCPD VBUS stop force discharge threshold */
		rc = NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(
			dev, SPDGTH, NUMAKER_UTCPD_VBUS_THRESHOLD_STOP_FORCE_DISCHARGE_MV);
		if (rc < 0) {
			return rc;
		}

		/* UTCPD VBUS sink disconnect threshold */
		rc = NUMAKER_UTCPD_VBUS_THRESHOLD_WRITE(
			dev, SKVBDCTH, NUMAKER_UTCPD_VBUS_THRESHOLD_SINK_DISCONNECT_MV);
		if (rc < 0) {
			return rc;
		}
	}

	/* Enable UTCPD VBUS voltage monitor so that UTCPD.VBVOL is available */
	if (config->eadc.spec_vbus != NULL) {
		pwrctl &= ~UTCPD_PWRCTL_VBMONI_DIS;
	} else {
		pwrctl |= UTCPD_PWRCTL_VBMONI_DIS;
	}
	/* Disable UTCPD VBUS voltage alarms */
	pwrctl |= UTCPD_PWRCTL_DSVBAM_DIS;
	/* Disable UTCPD VBUS auto-discharge on disconnect
	 * NOTE: UTCPD may not integrate with discharge, so this feature is
	 *	 disabled and discharge is handled separately.
	 */
	pwrctl &= ~UTCPD_PWRCTL_ADGDC;
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PWRCTL, pwrctl);
}

/**
 * @brief Initializes UTCPD GPIO pins
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_gpios_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	int rc;
	const struct gpio_dt_spec *spec;

	/* Configure VBUS detect pin to INPUT to avoid intervening its power measurement */
	spec = &config->utcpd.gpios.vbus_detect;
	if (spec->port == NULL) {
		LOG_ERR("VBUS detect pin not specified");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(spec)) {
		LOG_ERR("VBUS detect pin port device not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("VBUS detect pin configured to INPUT failed: %d", rc);
		return rc;
	}

	/* Configure VBUS discharge pin to OUTPUT INACTIVE */
	spec = &config->utcpd.gpios.vbus_discharge;
	if (spec->port != NULL) {
		if (!gpio_is_ready_dt(spec)) {
			LOG_ERR("VBUS discharge pin port device not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("VBUS discharge pin configured to OUTPUT INACTIVE failed: %d", rc);
			return rc;
		}
	}

	/* Configure VCONN discharge pin to OUTPUT INACTIVE */
	spec = &config->utcpd.gpios.vconn_discharge;
	if (spec->port != NULL) {
		if (!gpio_is_ready_dt(spec)) {
			LOG_ERR("VCONN discharge pin port device not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("VCONN discharge pin configured to OUTPUT INACTIVE failed: %d", rc);
			return rc;
		}
	}

	return 0;
}

/**
 * @brief Initializes UTCPD PHY
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_phy_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t phyctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PHYCTL);

	/* Enable PHY
	 *
	 * NOTE: Only UTCPD0 is supported.
	 */
	SYS->UTCPDCTL |= SYS_UTCPDCTL_POREN0_Msk;
	phyctl |= UTCPD_PHYCTL_PHYPWR_Msk;
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PHYCTL, phyctl);
}

/**
 * @brief Checks if UTCPD Dead Battery mode is enabled
 *
 * @retval true Dead Battery mode is enabled
 * @retval false Dead Battery mode is not enabled
 */
static bool numaker_utcpd_deadbattery_query_enable(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t phyctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PHYCTL);

	/* 0 = Dead Battery circuit controls internal Rd/Rp.
	 * 1 = Role Control Register controls internal Rd/
	 */
	return !(phyctl & UTCPD_PHYCTL_DBCTL_Msk);
}

/**
 * @brief Enables or disables UTCPD Dead Battery mode
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_deadbattery_set_enable(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t phyctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PHYCTL);

	if (enable) {
		/* Dead Battery circuit controls internal Rd/Rp */
		phyctl &= ~UTCPD_PHYCTL_DBCTL_Msk;
	} else {
		/* UTCPD.ROLCTL controls internal Rd/Rp */
		phyctl |= UTCPD_PHYCTL_DBCTL_Msk;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PHYCTL, phyctl);
}

/**
 * @brief Initializes UTCPD Dead Battery mode
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_deadbattery_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;

	return numaker_utcpd_deadbattery_set_enable(dev, config->utcpd.dead_battery);
}

/**
 * @brief Initializes UTCPD interrupts
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_interrupts_init(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t ie;
	uint32_t pwrstsie;
	uint32_t futstsie;
	uint32_t vndie;

	ie = UTCPD_IE_VNDIE_Msk | UTCPD_IE_SKDCDTIE_Msk | UTCPD_IE_RXOFIE_Msk | UTCPD_IE_FUTIE_Msk |
	     UTCPD_IE_VBAMLIE_Msk | UTCPD_IE_VBAMHIE_Msk | UTCPD_IE_TXOKIE_Msk |
	     UTCPD_IE_TXDCUDIE_Msk | UTCPD_IE_TXFAILIE_Msk | UTCPD_IE_RXHRSTIE_Msk |
	     UTCPD_IE_RXSOPIE_Msk | UTCPD_IE_PWRSCHIE_Msk | UTCPD_IE_CCSCHIE_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, IE, ie);
	if (rc < 0) {
		return rc;
	}

	pwrstsie = UTCPD_PWRSTSIE_DACONIE_Msk | UTCPD_PWRSTSIE_SRHVIE_Msk |
		   UTCPD_PWRSTSIE_SRVBIE_Msk | UTCPD_PWRSTSIE_VBDTDGIE_Msk |
		   UTCPD_PWRSTSIE_VBPSIE_Msk | UTCPD_PWRSTSIE_VCPSIE_Msk |
		   UTCPD_PWRSTSIE_SKVBIE_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PWRSTSIE, pwrstsie);
	if (rc < 0) {
		return rc;
	}

	futstsie = UTCPD_FUTSTSIE_FOFFVBIE_Msk | UTCPD_FUTSTSIE_ADGFALIE_Msk |
		   UTCPD_FUTSTSIE_FDGFALIE_Msk | UTCPD_FUTSTSIE_VBOCIE_Msk |
		   UTCPD_FUTSTSIE_VBOVIE_Msk | UTCPD_FUTSTSIE_VCOCIE_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, FUTSTSIE, futstsie);
	if (rc < 0) {
		return rc;
	}

	vndie = UTCPD_VNDIE_VCDGIE_Msk | UTCPD_VNDIE_CRCERRIE_Msk | UTCPD_VNDIE_TXFRSIE_Msk |
		UTCPD_VNDIE_RXFRSIE_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, VNDIE, vndie);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/**
 * @brief Initializes UTCPD at stack recycle
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_init_recycle(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t value;

	/* Disable BIST, CC1/CC2 for CC/VCOON */
	value = 0;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, CTL, value);
	if (rc < 0) {
		return rc;
	}

	/* Rp default, CC1/CC2 Rd */
	value = UTCPD_ROLECTL_RPVALUE_DEF | UTCPD_ROLECTL_CC1_RD | UTCPD_ROLECTL_CC2_RD;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, ROLCTL, value);
	if (rc < 0) {
		return rc;
	}

	/* Disable VCONN source */
	value = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRCTL);
	value &= ~UTCPD_PWRCTL_VCEN_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PWRCTL, value);
	if (rc < 0) {
		return rc;
	}

	/* Disable detecting Rx events */
	value = 0;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, DTRXEVNT, value);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/**
 * @brief Initializes UTCPD at device startup
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_utcpd_init_startup(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t pinpl;
	uint32_t futctl;
	uint32_t muxsel;

	/* UTCPD GPIO */
	rc = numaker_utcpd_gpios_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* UTCPD PHY */
	rc = numaker_utcpd_phy_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* UTCPD Dead Battery */
	rc = numaker_utcpd_deadbattery_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* UTCPD pin polarity */
	pinpl = config->utcpd.pinpl.bit;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PINPL, pinpl);
	if (rc < 0) {
		return rc;
	}

	/* VBUS voltage and monitor */
	rc = numaker_utcpd_vbus_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* UTCPD fault
	 *
	 * Disable the following fault detects which rely on external circuit:
	 * 1. VBUS force-off
	 * 2. VBUS overcurrent protection
	 * 3. VCONN overcurrent protection
	 */
	futctl = UTCPD_FUTCTL_FOFFVBDS_Msk | UTCPD_FUTCTL_VBOCDTDS_Msk | UTCPD_FUTCTL_VCOCDTDS_Msk;
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, FUTCTL, futctl);
	if (rc < 0) {
		return rc;
	}

	/* UTCPD interconnection select
	 *
	 * NOTE: Just configure CC2FRSS/CC2VCENS/CC1FRSS/CC1VCENS to non-merged
	 *       to follow TCPCI
	 */
	muxsel = UTCPD_MUXSEL_CC2FRSS_Msk | UTCPD_MUXSEL_CC2VCENS_Msk | UTCPD_MUXSEL_CC1FRSS_Msk |
		 UTCPD_MUXSEL_CC1VCENS_Msk;
	/* NOTE: For absence of EADC channel measurement for VCONN, we configure with all-one which
	 *       is supposed to be invalid EADC channel number so that UTCPD won't get updated
	 *       on VCONN by accident.
	 */
	if (config->eadc.spec_vbus != NULL) {
		muxsel |= (config->eadc.spec_vbus->channel_id << UTCPD_MUXSEL_ADCSELVB_Pos);
	} else {
		muxsel |= UTCPD_MUXSEL_ADCSELVB_Msk;
	}
	if (config->eadc.spec_vconn != NULL) {
		muxsel |= (config->eadc.spec_vconn->channel_id << UTCPD_MUXSEL_ADCSELVC_Pos);
	} else {
		muxsel |= UTCPD_MUXSEL_ADCSELVC_Msk;
	}
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, MUXSEL, muxsel);
	if (rc < 0) {
		return rc;
	}

	/* Interrupts */
	rc = numaker_utcpd_interrupts_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* IRQ */
	config->irq_config_func_utcpd(dev);

	return 0;
}

/**
 * @brief Initializes EADC to be timer-triggered for measuring
 *        VBUS/VCONN voltage at device startup
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_eadc_init_startup(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	EADC_T *eadc_base = config->eadc_base;
	int rc;
	const struct adc_dt_spec *spec;

	/* Vref */
	rc = numaker_eadc_vref_init(dev);
	if (rc < 0) {
		return rc;
	}

	/* Set input mode as single-end and enable the A/D converter */
	EADC_Open(eadc_base, EADC_CTL_DIFFEN_SINGLE_END);

	/* Configure sample module for measuring VBUS voltage
	 *
	 * NOTE: Make sample module number the same as channel number for
	 *       easy implementation.
	 * NOTE: EADC measurement channel for VBUS can be absent with PWRSTS.VBPS as fallback
	 */
	spec = config->eadc.spec_vbus;
	if (spec) {
		rc = numaker_eadc_smplmod_init(dev, spec, config->eadc.trgsel_vbus);
		if (rc < 0) {
			return rc;
		}
	}

	/* Configure sample module for measuring VCONN voltage
	 *
	 * NOTE: Make sample module number the same as channel number for
	 *       easy implementation.
	 * NOTE: EADC measurement channel for VCONN can be absent for VCONN unsupported
	 */
	spec = config->eadc.spec_vconn;
	if (spec) {
		rc = numaker_eadc_smplmod_init(dev, spec, config->eadc.trgsel_vconn);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

/**
 * @brief Initializes Timer to trigger EADC for measuring VBUS/VCONN
 *        voltage at device startup
 *
 * @retval 0 on success
 */
static int numaker_timer_init_startup(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	TIMER_T *timer_base = config->timer_base;

	/* Configure Timer to trigger EADC periodically */
	TIMER_Open(timer_base, TIMER_PERIODIC_MODE, config->eadc.timer_trigger_rate);
	TIMER_SetTriggerSource(timer_base, TIMER_TRGSRC_TIMEOUT_EVENT);
	TIMER_SetTriggerTarget(timer_base, TIMER_TRG_TO_EADC);
	TIMER_Start(timer_base);

	return 0;
}

/**
 * @brief Initializes TCPC at stack recycle
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_init_recycle(const struct device *dev)
{
	struct numaker_tcpc_data *data = dev->data;
	int rc;

	/* Initialize UTCPD for attach/detach recycle */
	rc = numaker_utcpd_init_recycle(dev);
	if (rc < 0) {
		return rc;
	}

	/* The fields below must (re-)initialize for tcpc_init(). */
	data->rp = TC_RP_USB;
	data->rx_sop_prime_enabled = false;
	data->rx_msg_ready = false;
	memset(&data->rx_msg, 0x00, sizeof(data->rx_msg));

	return 0;
}

/**
 * @brief Initializes TCPC at device startup
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_init_startup(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	int rc;

	SYS_UnlockReg();

	/* Configure pinmux (NuMaker's SYS MFP) */
	rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	/* Invoke Clock controller to enable module clock */

	/* Equivalent to CLK_EnableModuleClock() */
	rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc_utcpd);
	if (rc < 0) {
		return rc;
	}
	rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc_timer);
	if (rc < 0) {
		return rc;
	}

	/* Equivalent to CLK_SetModuleClock() */
	rc = clock_control_configure(config->clkctrl_dev,
				     (clock_control_subsys_t)&config->pcc_utcpd, NULL);
	if (rc < 0) {
		return rc;
	}
	rc = clock_control_configure(config->clkctrl_dev,
				     (clock_control_subsys_t)&config->pcc_timer, NULL);
	if (rc < 0) {
		return rc;
	}

	/* Invoke Reset controller to reset module to default state */
	/* Equivalent to SYS_ResetModule() */
	rc = reset_line_toggle_dt(&config->reset_utcpd);
	if (rc < 0) {
		return rc;
	}
	rc = reset_line_toggle_dt(&config->reset_timer);
	if (rc < 0) {
		return rc;
	}

	/* Initialize UTCPD */
	rc = numaker_utcpd_init_startup(dev);
	if (rc < 0) {
		return rc;
	}

	if (config->eadc.spec_vbus != NULL || config->eadc.spec_vconn != NULL) {
		/* Initialize EADC */
		rc = numaker_eadc_init_startup(dev);
		if (rc < 0) {
			return rc;
		}

		/* Initialize Timer */
		rc = numaker_timer_init_startup(dev);
		if (rc < 0) {
			return rc;
		}
	}

	return numaker_tcpc_init_recycle(dev);
}

/**
 * @brief Reads the status of the CC lines
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
			       enum tc_cc_voltage_state *cc2)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t rolctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, ROLCTL);
	uint32_t rolctl_cc1 = rolctl & UTCPD_ROLCTL_CC1_Msk;
	uint32_t rolctl_cc2 = rolctl & UTCPD_ROLCTL_CC2_Msk;
	uint32_t ccsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CCSTS);
	uint32_t ccsts_cc1state = ccsts & UTCPD_CCSTS_CC1STATE_Msk;
	uint32_t ccsts_cc2state = ccsts & UTCPD_CCSTS_CC2STATE_Msk;
	uint32_t ccsts_conrlt = ccsts & UTCPD_CCSTS_CONRLT_Msk;

	/* CC1 */
	if (rolctl_cc1 == UTCPD_ROLECTL_CC1_RP || ccsts_conrlt == UTCPD_CONN_RESULT_RP) {
		switch (ccsts_cc1state) {
		case UTCPD_CCSTS_CC1STATE_SRC_RA:
			*cc1 = TC_CC_VOLT_RA;
			break;
		case UTCPD_CCSTS_CC1STATE_SRC_RD:
			*cc1 = TC_CC_VOLT_RD;
			break;
		default:
			*cc1 = TC_CC_VOLT_OPEN;
		}
	} else if (rolctl_cc1 == UTCPD_ROLECTL_CC1_RD || ccsts_conrlt == UTCPD_CONN_RESULT_RD) {
		switch (ccsts_cc1state) {
		case UTCPD_CCSTS_CC1STATE_SNK_DEF:
			*cc1 = TC_CC_VOLT_RP_DEF;
			break;
		case UTCPD_CCSTS_CC1STATE_SNK_1P5A:
			*cc1 = TC_CC_VOLT_RP_1A5;
			break;
		case UTCPD_CCSTS_CC1STATE_SNK_3A:
			*cc1 = TC_CC_VOLT_RP_3A0;
			break;
		default:
			*cc1 = TC_CC_VOLT_OPEN;
		}
	} else {
		*cc1 = TC_CC_VOLT_OPEN;
	}

	/* CC2 */
	if (rolctl_cc2 == UTCPD_ROLECTL_CC2_RP || ccsts_conrlt == UTCPD_CONN_RESULT_RP) {
		switch (ccsts_cc2state) {
		case UTCPD_CCSTS_CC2STATE_SRC_RA:
			*cc2 = TC_CC_VOLT_RA;
			break;
		case UTCPD_CCSTS_CC2STATE_SRC_RD:
			*cc2 = TC_CC_VOLT_RD;
			break;
		default:
			*cc2 = TC_CC_VOLT_OPEN;
		}
	} else if (rolctl_cc2 == UTCPD_ROLECTL_CC2_RD || ccsts_conrlt == UTCPD_CONN_RESULT_RD) {
		switch (ccsts_cc2state) {
		case UTCPD_CCSTS_CC2STATE_SNK_DEF:
			*cc2 = TC_CC_VOLT_RP_DEF;
			break;
		case UTCPD_CCSTS_CC2STATE_SNK_1P5A:
			*cc2 = TC_CC_VOLT_RP_1A5;
			break;
		case UTCPD_CCSTS_CC2STATE_SNK_3A:
			*cc2 = TC_CC_VOLT_RP_3A0;
			break;
		default:
			*cc2 = TC_CC_VOLT_OPEN;
		}
	} else {
		*cc2 = TC_CC_VOLT_OPEN;
	}

	return 0;
}

/**
 * @brief Sets the value of CC pull up resistor used when operating as a Source
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	struct numaker_tcpc_data *data = dev->data;

	data->rp = rp;

	return 0;
}

/**
 * @brief Gets the value of the CC pull up resistor used when operating as a Source
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	struct numaker_tcpc_data *data = dev->data;

	*rp = data->rp;

	return 0;
}

/**
 * @brief Sets the CC pull resistor and sets the role as either Source or Sink
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t rolctl = 0;

	/* Disable Dead Battery mode if it is active, so that
	 * internal Rd/Rp gets controlled by to UTCPD.ROLCTL
	 * from Dead Battery circuit.
	 */
	if (numaker_utcpd_deadbattery_query_enable(dev)) {
		rc = numaker_utcpd_deadbattery_set_enable(dev, false);
		if (rc < 0) {
			return rc;
		}
	}

	/* Rp value: default, 1.5A, or 3.0A */
	switch (data->rp) {
	case TC_RP_USB:
		rolctl |= UTCPD_ROLECTL_RPVALUE_DEF;
		break;

	case TC_RP_1A5:
		rolctl |= UTCPD_ROLECTL_RPVALUE_1P5A;
		break;

	case TC_RP_3A0:
		rolctl |= UTCPD_ROLECTL_RPVALUE_3A;
		break;

	default:
		LOG_ERR("Invalid Rp value: %d", data->rp);
		return -EINVAL;
	}

	/* Pull on both CC1/CC2, determining source/sink role */
	switch (pull) {
	case TC_CC_RA:
		rolctl |= (UTCPD_ROLECTL_CC1_RA | UTCPD_ROLECTL_CC2_RA);
		break;

	case TC_CC_RP:
		rolctl |= (UTCPD_ROLECTL_CC1_RP | UTCPD_ROLECTL_CC2_RP);
		break;

	case TC_CC_RD:
		rolctl |= (UTCPD_ROLECTL_CC1_RD | UTCPD_ROLECTL_CC2_RD);
		break;

	case TC_CC_OPEN:
		rolctl |= (UTCPD_ROLECTL_CC1_OPEN | UTCPD_ROLECTL_CC2_OPEN);
		break;

	default:
		LOG_ERR("Invalid pull: %d", pull);
		return -EINVAL;
	}

	/* Update CC1/CC2 pull values */
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, ROLCTL, rolctl);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/**
 * @brief Sets a callback that can enable or discharge VCONN if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the VCONN control capabilities of the TCPC
 */
static void numaker_tcpc_set_vconn_discharge_cb(const struct device *dev,
						tcpc_vconn_discharge_cb_t cb)
{
	struct numaker_tcpc_data *data = dev->data;

	data->dpm.vconn_discharge_cb = cb;
}

/**
 * @brief Sets a callback that can enable or disable VCONN if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the VCONN control capabilities of the TCPC
 */
static void numaker_tcpc_set_vconn_cb(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb)
{
	struct numaker_tcpc_data *data = dev->data;

	data->dpm.vconn_cb = vconn_cb;
}

/**
 * @brief Discharges VCONN
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_vconn_discharge(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	UTCPD_T *utcpd_base = config->utcpd_base;
	const struct gpio_dt_spec *vconn_discharge_spec = &config->utcpd.gpios.vconn_discharge;
	uint32_t ctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CTL);
	uint32_t vcdgctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, VCDGCTL);
	enum tc_cc_polarity polarity =
		(ctl & UTCPD_CTL_ORIENT_Msk) ? TC_POLARITY_CC2 : TC_POLARITY_CC1;

	/* Use DPM supplied VCONN discharge */
	if (data->dpm.vconn_discharge_cb) {
		return data->dpm.vconn_discharge_cb(dev, polarity, enable);
	}

	/* Use GPIO VCONN discharge */
	if (vconn_discharge_spec->port != NULL) {
		return gpio_pin_set_dt(vconn_discharge_spec, enable);
	}

	/* Use UTCPD VCONN discharge */
	if (enable) {
		vcdgctl |= UTCPD_VCDGCTL_VCDGEN_Msk;
	} else {
		vcdgctl &= ~UTCPD_VCDGCTL_VCDGEN_Msk;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, VCDGCTL, vcdgctl);
}

/**
 * @brief Enables or disables VCONN
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_vconn(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRCTL);
	uint32_t ctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CTL);
	enum tc_cc_polarity polarity =
		(ctl & UTCPD_CTL_ORIENT_Msk) ? TC_POLARITY_CC2 : TC_POLARITY_CC1;

	/* Use DPM supplied VCONN */
	if (data->dpm.vconn_cb) {
		return data->dpm.vconn_cb(dev, polarity, enable);
	}

	/* Use UTCPD VCONN */
	if (enable) {
		pwrctl |= UTCPD_PWRCTL_VCEN_Msk;
	} else {
		pwrctl &= ~UTCPD_PWRCTL_VCEN_Msk;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, PWRCTL, pwrctl);
}

/**
 * @brief Sets the Power and Data Role of the PD message header
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_roles(const struct device *dev, enum tc_power_role power_role,
				  enum tc_data_role data_role)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t mshead = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, MSHEAD);

	/* Power role for auto-reply GoodCRC */
	mshead &= ~UTCPD_MSHEAD_PWRROL_Msk;
	if (power_role == TC_ROLE_SOURCE) {
		mshead |= UTCPD_MHINFO_PROLE_SRC;
	} else {
		mshead |= UTCPD_MHINFO_PROLE_SNK;
	}

	/* Data role for auto-reply GoodCRC */
	mshead &= ~UTCPD_MSHEAD_DAROL_Msk;
	if (data_role == TC_ROLE_DFP) {
		mshead |= UTCPD_MHINFO_DROLE_DFP;
	} else {
		mshead |= UTCPD_MHINFO_DROLE_UFP;
	}

	/* Message Header for auto-reply GoodCRC */
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, MSHEAD, mshead);
}

/**
 * @brief Retrieves the Power Delivery message from the TCPC.
 * If buf is NULL, then only the status is returned, where 0 means there is a message pending and
 * -ENODATA means there is no pending message.
 *
 * @retval Greater or equal to 0 is the number of bytes received if buf parameter is provided
 * @retval 0 if there is a message pending and buf parameter is NULL
 * @retval -EIO on failure
 * @retval -ENODATA if no message is pending
 */
static int numaker_tcpc_get_rx_pending_msg(const struct device *dev, struct pd_msg *msg)
{
	struct numaker_tcpc_data *data = dev->data;

	/* Rx message pending? */
	if (!data->rx_msg_ready) {
		return -ENODATA;
	}

	/* Query status only? */
	if (msg == NULL) {
		return 0;
	}

	/* Dequeue Rx FIFO */
	*msg = data->rx_msg;
	data->rx_msg_ready = false;

	/* Indicate Rx message returned */
	return 1;
}

/**
 * @brief Enables the reception of SOP* message types
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_rx_enable(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	struct numaker_tcpc_data *data = dev->data;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t dtrxevnt = 0;

	/* Enable receive */
	if (enable) {
		/* Enable receive of SOP messages */
		dtrxevnt |= UTCPD_DTRXEVNT_SOPEN_Msk;

		/* Enable receive of SOP'/SOP'' messages */
		if (data->rx_sop_prime_enabled) {
			dtrxevnt |= UTCPD_DTRXEVNT_SOPPEN_Msk | UTCPD_DTRXEVNT_SOPPPEN_Msk;
		}

		/* Enable receive of Hard Reset */
		dtrxevnt |= UTCPD_DTRXEVNT_HRSTEN_Msk;

		/* Don't enable receive of Cable Reset for not being Cable Plug */
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, DTRXEVNT, dtrxevnt);
}

/**
 * @brief Sets the polarity of the CC lines
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_cc_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t ctl = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, CTL);

	/* Update CC polarity */
	switch (polarity) {
	case TC_POLARITY_CC1:
		ctl &= ~UTCPD_CTL_ORIENT_Msk;
		break;

	case TC_POLARITY_CC2:
		ctl |= UTCPD_CTL_ORIENT_Msk;
		break;

	default:
		LOG_ERR("Invalid CC polarity: %d", polarity);
		return -EINVAL;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, CTL, ctl);
}

/**
 * @brief Transmits a Power Delivery message
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_transmit_data(const struct device *dev, struct pd_msg *msg)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	int rc;
	uint32_t txctl;
	uint32_t txctl_retrycnt;
	uint32_t txctl_txstype;

	/* Not support Unchunked Extended Message exceeding PD_CONVERT_PD_HEADER_COUNT_TO_BYTES */
	if (msg->len > (PD_MAX_EXTENDED_MSG_LEGACY_LEN + 2)) {
		LOG_ERR("Not support Unchunked Extended Message exceeding "
			"PD_CONVERT_PD_HEADER_COUNT_TO_BYTES: %d",
			msg->len);
		return -EIO;
	}

	/* txbcnt = 2 (Message Header) + Tx data byte count */
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, TXBCNT, msg->len + 2);
	if (rc < 0) {
		return rc;
	}

	/* Tx header */
	rc = NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, TXHEAD, msg->header.raw_value);
	if (rc < 0) {
		return rc;
	}

	/* Tx data */
	rc = numaker_utcpd_tx_write_data(dev, msg->data, msg->len);
	if (rc < 0) {
		return rc;
	}

	/* Tx control */
	if (msg->type < PD_PACKET_TX_HARD_RESET) {
		/* nRetryCount = 2 for PD REV 3.0 */
		txctl_retrycnt = 2 << UTCPD_TXCTL_RETRYCNT_Pos;
	} else if (msg->type <= PD_PACKET_TX_BIST_MODE_2) {
		/* Per TCPCI spec, no retry for non-SOP* transmission */
		txctl_retrycnt = 0;
	} else {
		LOG_ERR("Invalid PD packet type: %d", msg->type);
		return -EINVAL;
	}
	/* NOTE: Needn't extra cast for UTCPD_TXCTL.TXSTYPE aligning with pd_packet_type */
	txctl_txstype = ((uint32_t)msg->type) << UTCPD_TXCTL_TXSTYPE_Pos;
	txctl = txctl_retrycnt | txctl_txstype;
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, TXCTL, txctl);
}

/**
 * @brief Dump a set of TCPC registers
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_dump_std_reg(const struct device *dev)
{
	return numaker_utcpd_dump_regs(dev);
}

/**
 * @brief Queries the current sinking state of the TCPC
 *
 * @retval true if sinking power
 * @retval false if not sinking power
 */
static int numaker_tcpc_get_snk_ctrl(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	return (pwrsts & UTCPD_PWRSTS_SKVB_Msk) ? true : false;
}

/**
 * @brief Queries the current sourcing state of the TCPC
 *
 * @retval true if sourcing power
 * @retval false if not sourcing power
 */
static int numaker_tcpc_get_src_ctrl(const struct device *dev)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	return (pwrsts & (UTCPD_PWRSTS_SRVB_Msk | UTCPD_PWRSTS_SRHV_Msk)) ? true : false;
}

/**
 * @brief Enables the reception of SOP Prime messages
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_sop_prime_enable(const struct device *dev, bool enable)
{
	struct numaker_tcpc_data *data = dev->data;

	data->rx_sop_prime_enabled = enable;

	return 0;
}

/**
 * @brief Controls the BIST Mode of the TCPC. It disables RX alerts while the
 *	  mode is active.
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int numaker_tcpc_set_bist_test_mode(const struct device *dev, bool enable)
{
	return numaker_utcpd_bist_test_mode_set_enable(dev, enable);
}

/**
 * @brief Sets the alert function that's called when an interrupt is triggered
 *	  due to an alert bit
 *
 * @retval 0 on success
 */
static int numaker_tcpc_set_alert_handler_cb(const struct device *dev,
					     tcpc_alert_handler_cb_t alert_handler,
					     void *alert_data)
{
	struct numaker_tcpc_data *data = dev->data;

	data->tcpc_alert.handler = alert_handler;
	data->tcpc_alert.data = alert_data;

	return 0;
}

/* Functions below with name pattern "*_tcpc_ppc_*" are to invoke by NuMaker PPC driver */

int numaker_tcpc_ppc_is_dead_battery_mode(const struct device *dev)
{
	return numaker_utcpd_deadbattery_query_enable(dev);
}

int numaker_tcpc_ppc_exit_dead_battery_mode(const struct device *dev)
{
	return numaker_utcpd_deadbattery_set_enable(dev, false);
}

int numaker_tcpc_ppc_is_vbus_source(const struct device *dev)
{
	return numaker_utcpd_vbus_is_source(dev);
}

int numaker_tcpc_ppc_is_vbus_sink(const struct device *dev)
{
	return numaker_utcpd_vbus_is_sink(dev);
}

int numaker_tcpc_ppc_set_snk_ctrl(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t cmd;

	if (enable) {
		cmd = UTCPD_CMD_SINK_VBUS;
	} else {
		cmd = UTCPD_CMD_DISABLE_SINK_VBUS;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, CMD, cmd);
}

int numaker_tcpc_ppc_set_src_ctrl(const struct device *dev, bool enable)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t cmd;

	if (enable) {
		/* NOTE: Source VBUS high voltage (UTCPD_CMD_SRC_VBUS_NONDEFAULT) N/A */
		cmd = UTCPD_CMD_SRC_VBUS_DEFAULT;
	} else {
		cmd = UTCPD_CMD_DISABLE_SRC_VBUS;
	}
	return NUMAKER_UTCPD_REG_WRITE_BY_NAME(dev, CMD, cmd);
}

int numaker_tcpc_ppc_set_vbus_discharge(const struct device *dev, bool enable)
{
	return numaker_utcpd_vbus_set_discharge(dev, enable);
}

int numaker_tcpc_ppc_is_vbus_present(const struct device *dev)
{
	return numaker_utcpd_vbus_is_present(dev);
}

int numaker_tcpc_ppc_set_event_handler(const struct device *dev, usbc_ppc_event_cb_t event_handler,
				       void *event_data)
{
	struct numaker_tcpc_data *data = dev->data;

	data->ppc_event.handler = event_handler;
	data->ppc_event.data = event_data;

	return 0;
}

int numaker_tcpc_ppc_dump_regs(const struct device *dev)
{
	return numaker_utcpd_dump_regs(dev);
}

/* End of "*_tcpc_ppc_*" functions */

/* Functions below with name pattern "*_tcpc_vbus_*" are to invoke by NuMaker VBUS driver */

bool numaker_tcpc_vbus_check_level(const struct device *dev, enum tc_vbus_level level)
{
	const struct numaker_tcpc_config *const config = dev->config;
	UTCPD_T *utcpd_base = config->utcpd_base;
	uint32_t mv_norm;
	int rc = numaker_utcpd_vbus_measure(dev, &mv_norm);
	uint32_t pwrsts = NUMAKER_UTCPD_REG_READ_BY_NAME(dev, PWRSTS);

	/* Fall back to PWRSTS.VBPS if VBUS measurement by EADC is not available */
	switch (level) {
	case TC_VBUS_SAFE0V:
		return (rc == 0) ? (mv_norm < PD_V_SAFE_0V_MAX_MV)
				 : !(pwrsts & UTCPD_PWRSTS_VBPS_Msk);
	case TC_VBUS_PRESENT:
		return (rc == 0) ? (mv_norm >= PD_V_SAFE_5V_MIN_MV)
				 : (pwrsts & UTCPD_PWRSTS_VBPS_Msk);
	case TC_VBUS_REMOVED:
		return (rc == 0) ? (mv_norm < TC_V_SINK_DISCONNECT_MAX_MV)
				 : !(pwrsts & UTCPD_PWRSTS_VBPS_Msk);
	}

	return false;
}

int numaker_tcpc_vbus_measure(const struct device *dev, int *vbus_meas)
{
	int rc;
	uint32_t mv;

	if (vbus_meas == NULL) {
		return -EINVAL;
	}
	*vbus_meas = 0;

	rc = numaker_utcpd_vbus_measure(dev, &mv);
	if (rc < 0) {
		return rc;
	}
	*vbus_meas = mv;

	return 0;
}

int numaker_tcpc_vbus_discharge(const struct device *dev, bool enable)
{
	return numaker_utcpd_vbus_set_discharge(dev, enable);
}

int numaker_tcpc_vbus_enable(const struct device *dev, bool enable)
{
	/* VBUS measurement is made automatic through Timer-triggered EADC. */
	return 0;
}

/* End of "*_tcpc_vbus_*" functions */

static const struct tcpc_driver_api numaker_tcpc_driver_api = {
	.init = numaker_tcpc_init_recycle,
	.get_cc = numaker_tcpc_get_cc,
	.select_rp_value = numaker_tcpc_select_rp_value,
	.get_rp_value = numaker_tcpc_get_rp_value,
	.set_cc = numaker_tcpc_set_cc,
	.set_vconn_discharge_cb = numaker_tcpc_set_vconn_discharge_cb,
	.set_vconn_cb = numaker_tcpc_set_vconn_cb,
	.vconn_discharge = numaker_tcpc_vconn_discharge,
	.set_vconn = numaker_tcpc_set_vconn,
	.set_roles = numaker_tcpc_set_roles,
	.get_rx_pending_msg = numaker_tcpc_get_rx_pending_msg,
	.set_rx_enable = numaker_tcpc_set_rx_enable,
	.set_cc_polarity = numaker_tcpc_set_cc_polarity,
	.transmit_data = numaker_tcpc_transmit_data,
	.dump_std_reg = numaker_tcpc_dump_std_reg,
	.get_snk_ctrl = numaker_tcpc_get_snk_ctrl,
	.get_src_ctrl = numaker_tcpc_get_src_ctrl,
	.sop_prime_enable = numaker_tcpc_sop_prime_enable,
	.set_bist_test_mode = numaker_tcpc_set_bist_test_mode,
	.set_alert_handler_cb = numaker_tcpc_set_alert_handler_cb,
};

/* Same as RESET_DT_SPEC_INST_GET_BY_IDX, except by name */
#define NUMAKER_RESET_DT_SPEC_INST_GET_BY_NAME(inst, name)                                         \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_RESET_CTLR_BY_NAME(inst, name)),                      \
		.id = DT_INST_RESET_CELL_BY_NAME(inst, name, id),                                  \
	}

/* Same as GPIO_DT_SPEC_GET_BY_IDX, except by name */
#define NUMAKER_GPIO_DT_SPEC_GET_BY_NAME(node_id, prop, name)                                      \
	{                                                                                          \
		.port = DEVICE_DT_GET(DT_PHANDLE_BY_NAME(node_id, prop, name)),                    \
		.pin = DT_PHA_BY_NAME(node_id, prop, name, pin),                                   \
		.dt_flags = DT_PHA_BY_NAME(node_id, prop, name, flags),                            \
	}

/* Same as GPIO_DT_SPEC_INST_GET_BY_IDX_OR, except by name */
#define NUMAKER_GPIO_DT_SPEC_INST_GET_BY_NAME_OR(inst, prop, name, default_value)                  \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(inst, prop, name),                                       \
		    (NUMAKER_GPIO_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), prop, name)),             \
		    (default_value))

/* Peripheral Clock Control by name */
#define NUMAKER_PCC_INST_GET_BY_NAME(inst, name)                                                   \
	{                                                                                          \
		.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC,                                            \
		.pcc.clk_modidx = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_module_index),     \
		.pcc.clk_src = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_source),              \
		.pcc.clk_div = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_divider),             \
	}

/* UTCPD GPIOs */
#define NUMAKER_UTCPD_GPIOS_INIT(inst)                                                             \
	{                                                                                          \
		.vbus_detect =                                                                     \
			NUMAKER_GPIO_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), gpios, vbus_detect),   \
		.vbus_discharge = NUMAKER_GPIO_DT_SPEC_INST_GET_BY_NAME_OR(inst, gpios,            \
									   vbus_discharge, {0}),   \
		.vconn_discharge = NUMAKER_GPIO_DT_SPEC_INST_GET_BY_NAME_OR(inst, gpios,           \
									    vconn_discharge, {0}), \
	}

/* UTCPD.PINPL.<PIN> cast */
#define NUMAKER_UTCPD_PINPOL_CAST(inst, pin_dt, pin_utcpd)                                         \
	(DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), pin_dt, high_active) ? UTCPD_PINPL_##pin_utcpd##_Msk \
								   : 0)

/* UTCPD.VBVOL.VBSCALE cast */
#define NUMAKER_UTCPD_VBUS_DIVIDE_CAST(inst) NUMAKER_UTCPD_VBUS_DIVIDE_CAST_NO_DIVIDE(inst)
/* no_divide */
#define NUMAKER_UTCPD_VBUS_DIVIDE_CAST_NO_DIVIDE(inst)                                             \
	COND_CODE_1(DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), vbus_divide, no_divice),                  \
		    ({.bit = (0 << UTCPD_VBVOL_VBSCALE_Pos), .value = 1}),                         \
		    (NUMAKER_UTCPD_VBUS_DIVIDE_CAST_DIVIDE_10(inst)))
/* divide_10 */
#define NUMAKER_UTCPD_VBUS_DIVIDE_CAST_DIVIDE_10(inst)                                             \
	COND_CODE_1(DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), vbus_divide, divide_10),                  \
		    ({.bit = (1 << UTCPD_VBVOL_VBSCALE_Pos), .value = 10}),                        \
		    (NUMAKER_UTCPD_VBUS_DIVIDE_CAST_DIVIDE_20(inst)))
/* divide_20 */
#define NUMAKER_UTCPD_VBUS_DIVIDE_CAST_DIVIDE_20(inst)                                             \
	COND_CODE_1(DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), vbus_divide, divide_20),                  \
		    ({.bit = (2 << UTCPD_VBVOL_VBSCALE_Pos), .value = 20}), (no_divide error))

/* UTCPD.PINPL */
#define NUMAKER_UTCPD_PINPL_INIT(inst)                                                             \
	{                                                                                          \
		.bit = NUMAKER_UTCPD_PINPOL_CAST(inst, vconn_overcurrent_event_polarity, VCOCPL) | \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vconn_discharge_polarity, VCDGENPL) |       \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vconn_enable_polarity, VCENPL) |            \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vbus_overcurrent_event_polarity, VBOCPL) |  \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vbus_forceoff_event_polarity, FOFFVBPL) |   \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, frs_tx_polarity, TXFRSPL) |                 \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vbus_discharge_enable_polarity, VBDGENPL) | \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vbus_sink_enable_polarity, VBSKENPL) |      \
		       NUMAKER_UTCPD_PINPOL_CAST(inst, vbus_source_enable_polarity, VBSRENPL)      \
	}

/* UTCPD.VBVOL */
#define NUMAKER_UTCPD_VBVOL_INIT(inst)                                                             \
	{                                                                                          \
		.vbscale = NUMAKER_UTCPD_VBUS_DIVIDE_CAST(inst),                                   \
	}

#define NUMAKER_UTCPD_INIT(inst)                                                                   \
	{                                                                                          \
		.gpios = NUMAKER_UTCPD_GPIOS_INIT(inst),                                           \
		.dead_battery = DT_INST_PROP(inst, dead_battery),                                  \
		.pinpl = NUMAKER_UTCPD_PINPL_INIT(inst), .vbvol = NUMAKER_UTCPD_VBVOL_INIT(inst),  \
	}

/* EADC register address is duplicated for easy implementation.
 * They must be the same.
 */
#define BUILD_ASSERT_NUMAKER_EADC_REG(inst)                                                        \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(inst), io_channels),                               \
		   (BUILD_ASSERT(DT_INST_REG_ADDR_BY_NAME(inst, eadc) ==                           \
				 DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(inst)));))

#define NUMAKER_EADC_TRGSRC_CAST(inst)                                                             \
	((DT_INST_REG_ADDR_BY_NAME(inst, timer) == TIMER0_BASE)   ? EADC_TIMER0_TRIGGER            \
	 : (DT_INST_REG_ADDR_BY_NAME(inst, timer) == TIMER1_BASE) ? EADC_TIMER1_TRIGGER            \
	 : (DT_INST_REG_ADDR_BY_NAME(inst, timer) == TIMER2_BASE) ? EADC_TIMER2_TRIGGER            \
	 : (DT_INST_REG_ADDR_BY_NAME(inst, timer) == TIMER3_BASE) ? EADC_TIMER3_TRIGGER            \
								  : NUMAKER_INVALID_VALUE)

#define BUILD_ASSERT_NUMAKER_EADC_TRGSRC_CAST(inst)                                                \
	BUILD_ASSERT(NUMAKER_EADC_TRGSRC_CAST(inst) != NUMAKER_INVALID_VALUE,                      \
		     "NUMAKER_EADC_TRGSRC_CAST error");

/* Notes on specifying EADC channels
 *
 * 1. Must be in order of chn_vbus, chn_vconn, etc.
 * 2. The front channel can be absent, e.g. only chn_vconn.
 * 3. Build assert will check the above rules.
 */
#define NUMAKER_EADC_SPEC_GET_BY_IDX_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define NUMAKER_EADC_SPEC_DEFINE(inst)                                                             \
	IF_ENABLED(                                                                                \
		DT_NODE_HAS_PROP(DT_DRV_INST(inst), io_channels),                                  \
		(static const struct adc_dt_spec eadc_specs##inst[] = {DT_FOREACH_PROP_ELEM(       \
			 DT_DRV_INST(inst), io_channels, NUMAKER_EADC_SPEC_GET_BY_IDX_COMMA)};))

/* Note on EADC spec index
 *
 * These indexes must be integer literal, or meet macro expansion error.
 * However, macro expansion just does text replacement, no evaluation.
 * To overcome this, UTIL_INC() and friends are invoked to do evaluation
 * at preprocess time.
 */
#define NUMAKER_EADC_SPEC_IDX_VBUS(inst) 0
#define NUMAKER_EADC_SPEC_IDX_VCONN(inst)                                                          \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(inst, io_channels, chn_vbus),                            \
		    (UTIL_INC(NUMAKER_EADC_SPEC_IDX_VBUS(inst))),                                  \
		    (NUMAKER_EADC_SPEC_IDX_VBUS(inst)))

#define NUMAKER_EADC_SPEC_PTR_VBUS(inst)                                                           \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(inst, io_channels, chn_vbus),                            \
		    (&eadc_specs##inst[NUMAKER_EADC_SPEC_IDX_VBUS(inst)]), (NULL))
#define NUMAKER_EADC_SPEC_PTR_VCONN(inst)                                                          \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(inst, io_channels, chn_vconn),                           \
		    (&eadc_specs##inst[NUMAKER_EADC_SPEC_IDX_VCONN(inst)]), (NULL))

#define NUMAKER_EADC_DEVICE_BY_NAME(inst, name)                                                    \
	DEVICE_DT_GET(DT_IO_CHANNELS_CTLR_BY_NAME(DT_DRV_INST(inst), name))
#define NUMAKER_EADC_DEVICE_BY_IDX(inst, idx)                                                      \
	DEVICE_DT_GET(DT_IO_CHANNELS_CTLR_BY_IDX(DT_DRV_INST(inst), idx))

#define NUMAKER_EADC_INPUT_BY_NAME(inst, name) DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), name)
#define NUMAKER_EADC_INPUT_BY_IDX(inst, idx)   DT_IO_CHANNELS_INPUT_BY_IDX(DT_DRV_INST(inst), idx)

#define BUILD_ASSERT_NUMAKER_EADC_SPEC_VBUS(inst)                                                  \
	IF_ENABLED(DT_INST_PROP_HAS_NAME(inst, io_channels, chn_vbus),                             \
		   (BUILD_ASSERT(NUMAKER_EADC_DEVICE_BY_NAME(inst, chn_vbus) ==                    \
					 NUMAKER_EADC_DEVICE_BY_IDX(                               \
						 inst, NUMAKER_EADC_SPEC_IDX_VBUS(inst)),          \
				 "EADC device for VBUS error");                                    \
		    BUILD_ASSERT(NUMAKER_EADC_INPUT_BY_NAME(inst, chn_vbus) ==                     \
					 NUMAKER_EADC_INPUT_BY_IDX(                                \
						 inst, NUMAKER_EADC_SPEC_IDX_VBUS(inst)),          \
				 "EADC channel for VBUS error");))

#define BUILD_ASSERT_NUMAKER_EADC_SPEC_VCONN(inst)                                                 \
	IF_ENABLED(DT_INST_PROP_HAS_NAME(inst, io_channels, chn_vconn),                            \
		   (BUILD_ASSERT(NUMAKER_EADC_DEVICE_BY_NAME(inst, chn_vconn) ==                   \
					 NUMAKER_EADC_DEVICE_BY_IDX(                               \
						 inst, NUMAKER_EADC_SPEC_IDX_VCONN(inst)),         \
				 "EADC device for VCONN error");                                   \
		    BUILD_ASSERT(NUMAKER_EADC_INPUT_BY_NAME(inst, chn_vconn) ==                    \
					 NUMAKER_EADC_INPUT_BY_IDX(                                \
						 inst, NUMAKER_EADC_SPEC_IDX_VCONN(inst)),         \
				 "EADC channel for VCONN error");))

#define NUMAKER_EADC_INIT(inst)                                                                    \
	{                                                                                          \
		.spec_vbus = NUMAKER_EADC_SPEC_PTR_VBUS(inst),                                     \
		.spec_vconn = NUMAKER_EADC_SPEC_PTR_VCONN(inst),                                   \
		.timer_trigger_rate = DT_INST_PROP(inst, adc_measure_timer_trigger_rate),          \
		.trgsel_vbus = NUMAKER_EADC_TRGSRC_CAST(inst),                                     \
		.trgsel_vconn = NUMAKER_EADC_TRGSRC_CAST(inst),                                    \
	}

#define NUMAKER_TCPC_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	NUMAKER_EADC_SPEC_DEFINE(inst);                                                            \
                                                                                                   \
	static void numaker_utcpd_irq_config_func_##inst(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, utcpd, irq),                                 \
			    DT_INST_IRQ_BY_NAME(inst, utcpd, priority), numaker_utcpd_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, utcpd, irq));                                 \
	}                                                                                          \
                                                                                                   \
	static void numaker_utcpd_irq_unconfig_func_##inst(const struct device *dev)               \
	{                                                                                          \
		irq_disable(DT_INST_IRQ_BY_NAME(inst, utcpd, irq));                                \
	}                                                                                          \
                                                                                                   \
	static const struct numaker_tcpc_config numaker_tcpc_config_##inst = {                     \
		.utcpd_base = (UTCPD_T *)DT_INST_REG_ADDR_BY_NAME(inst, utcpd),                    \
		.eadc_base = (EADC_T *)DT_INST_REG_ADDR_BY_NAME(inst, eadc),                       \
		.timer_base = (TIMER_T *)DT_INST_REG_ADDR_BY_NAME(inst, timer),                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                \
		.pcc_utcpd = NUMAKER_PCC_INST_GET_BY_NAME(inst, utcpd),                            \
		.pcc_timer = NUMAKER_PCC_INST_GET_BY_NAME(inst, timer),                            \
		.reset_utcpd = NUMAKER_RESET_DT_SPEC_INST_GET_BY_NAME(inst, utcpd),                \
		.reset_timer = NUMAKER_RESET_DT_SPEC_INST_GET_BY_NAME(inst, timer),                \
		.irq_config_func_utcpd = numaker_utcpd_irq_config_func_##inst,                     \
		.irq_unconfig_func_utcpd = numaker_utcpd_irq_unconfig_func_##inst,                 \
		.utcpd = NUMAKER_UTCPD_INIT(inst),                                                 \
		.eadc = NUMAKER_EADC_INIT(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT_NUMAKER_EADC_REG(inst);                                                       \
	BUILD_ASSERT_NUMAKER_EADC_TRGSRC_CAST(inst);                                               \
	BUILD_ASSERT_NUMAKER_EADC_SPEC_VBUS(inst);                                                 \
	BUILD_ASSERT_NUMAKER_EADC_SPEC_VCONN(inst);                                                \
                                                                                                   \
	static struct numaker_tcpc_data numaker_tcpc_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, numaker_tcpc_init_startup, NULL, &numaker_tcpc_data_##inst,    \
			      &numaker_tcpc_config_##inst, POST_KERNEL,                            \
			      CONFIG_USBC_TCPC_INIT_PRIORITY, &numaker_tcpc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_TCPC_INIT);
