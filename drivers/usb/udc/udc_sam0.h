/*
 * Copyright Google LLC.
 * Copyright Nordic Semiconductor ASA
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_UDC_SAM0_H_
#define ZEPHYR_DRIVERS_USB_UDC_UDC_SAM0_H_

/*
 * Hardware access layer for the SAM0-compatible USB device controller.
 *
 * The same USB IP is described by two different vendor HALs:
 *   - Atmel ASF (CONFIG_ASF)
 *   - Microchip HAL (device family packs)
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_ASF)

typedef UsbDevice sam0_usb_reg_t;
typedef UsbDeviceEndpoint sam0_ep_reg_t;

#define SAM0_REG(base, name)     ((base)->name.reg)
#define SAM0_EP_REG(ep, name)    ((ep)->name.reg)
#define SAM0_EP_ARRAY(base, idx) ((base)->DeviceEndpoint[idx])

#else /* Microchip HAL */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/sys/sys_io.h>

typedef usb_device_registers_t sam0_usb_reg_t;
typedef usb_device_endpoint_registers_t sam0_ep_reg_t;

#define SAM0_REG(base, name)     ((base)->USB_##name)
#define SAM0_EP_REG(ep, name)    ((ep)->USB_##name)
#define SAM0_EP_ARRAY(base, idx) ((base)->DEVICE_ENDPOINT[idx])

#endif /* CONFIG_ASF */

/* HAL-neutral interrupt flag masks */
#define SAM0_USB_INT_SUSPEND BIT(USB_DEVICE_INTFLAG_SUSPEND_Pos)
#define SAM0_USB_INT_SOF     BIT(USB_DEVICE_INTFLAG_SOF_Pos)
#define SAM0_USB_INT_EORST   BIT(USB_DEVICE_INTFLAG_EORST_Pos)
#define SAM0_USB_INT_EORSM   BIT(USB_DEVICE_INTFLAG_EORSM_Pos)
#define SAM0_USB_INT_RAMACER BIT(USB_DEVICE_INTFLAG_RAMACER_Pos)

#define SAM0_EP_INT_TRCPT0 BIT(USB_DEVICE_EPINTFLAG_TRCPT0_Pos)
#define SAM0_EP_INT_TRCPT1 BIT(USB_DEVICE_EPINTFLAG_TRCPT1_Pos)
#define SAM0_EP_INT_RXSTP  BIT(USB_DEVICE_EPINTFLAG_RXSTP_Pos)

/* Controller registers */

static inline bool sam0_usb_syncbusy(const sam0_usb_reg_t *base)
{
	return SAM0_REG(base, SYNCBUSY) != 0U;
}

static inline void sam0_usb_reset(sam0_usb_reg_t *base)
{
	SAM0_REG(base, CTRLA) |= BIT(USB_CTRLA_SWRST_Pos);
}

static inline void sam0_usb_set_qos(sam0_usb_reg_t *base, const uint8_t cqos,
				    const uint8_t dqos)
{
	SAM0_REG(base, QOSCTRL) = (SAM0_REG(base, QOSCTRL) &
				   ~(USB_QOSCTRL_CQOS_Msk | USB_QOSCTRL_DQOS_Msk)) |
				  USB_QOSCTRL_CQOS(cqos) | USB_QOSCTRL_DQOS(dqos);
}

static inline void sam0_usb_set_padcal(sam0_usb_reg_t *base, const uint32_t padcal)
{
	SAM0_REG(base, PADCAL) = padcal;
}

static inline void sam0_usb_configure_device(sam0_usb_reg_t *base)
{
	SAM0_REG(base, CTRLA) = USB_CTRLA_MODE_DEVICE | BIT(USB_CTRLA_RUNSTDBY_Pos);
	SAM0_REG(base, CTRLB) = USB_DEVICE_CTRLB_SPDCONF_FS;
}

static inline void sam0_usb_set_desc_addr(sam0_usb_reg_t *base, const uintptr_t addr)
{
	SAM0_REG(base, DESCADD) = addr;
}

static inline void sam0_usb_set_address(sam0_usb_reg_t *base, const uint8_t addr)
{
	SAM0_REG(base, DADD) = addr ? (addr | BIT(USB_DEVICE_DADD_ADDEN_Pos)) : 0U;
}

static inline void sam0_usb_set_enable(sam0_usb_reg_t *base, const bool enabled)
{
	if (enabled) {
		SAM0_REG(base, CTRLA) |= BIT(USB_CTRLA_ENABLE_Pos);
	} else {
		SAM0_REG(base, CTRLA) &= ~BIT(USB_CTRLA_ENABLE_Pos);
	}
}

static inline void sam0_usb_set_detach(sam0_usb_reg_t *base, const bool detached)
{
	if (detached) {
		SAM0_REG(base, CTRLB) |= BIT(USB_DEVICE_CTRLB_DETACH_Pos);
	} else {
		SAM0_REG(base, CTRLB) &= ~BIT(USB_DEVICE_CTRLB_DETACH_Pos);
	}
}

static inline void sam0_usb_request_resume(sam0_usb_reg_t *base)
{
	SAM0_REG(base, CTRLB) |= BIT(USB_DEVICE_CTRLB_UPRSM_Pos);
}

static inline uint32_t sam0_usb_get_epint_summary(const sam0_usb_reg_t *base)
{
	return SAM0_REG(base, EPINTSMRY);
}

static inline uint32_t sam0_usb_get_int_flags(const sam0_usb_reg_t *base)
{
	return SAM0_REG(base, INTFLAG);
}

static inline void sam0_usb_clear_int_flags(sam0_usb_reg_t *base, const uint32_t flags)
{
	SAM0_REG(base, INTFLAG) = flags;
}

static inline void sam0_usb_enable_int(sam0_usb_reg_t *base, const uint32_t flags)
{
	SAM0_REG(base, INTENSET) = flags;
}

/* Endpoint registers */

static inline sam0_ep_reg_t *sam0_ep_reg(sam0_usb_reg_t *base, const uint8_t ep_idx)
{
	return &SAM0_EP_ARRAY(base, ep_idx);
}

static inline bool sam0_ep_bank_ready(const sam0_ep_reg_t *endpoint, const bool in)
{
	return (SAM0_EP_REG(endpoint, EPSTATUS) &
		(in ? BIT(USB_DEVICE_EPSTATUS_BK1RDY_Pos) :
		      BIT(USB_DEVICE_EPSTATUS_BK0RDY_Pos))) != 0U;
}

/* EPSTATUSSET/EPSTATUSCLR are write-only, write-one-to-act registers */
static inline void sam0_ep_set_bank_ready(sam0_ep_reg_t *endpoint, const bool in)
{
	SAM0_EP_REG(endpoint, EPSTATUSSET) = in ? BIT(USB_DEVICE_EPSTATUSSET_BK1RDY_Pos) :
						  BIT(USB_DEVICE_EPSTATUSSET_BK0RDY_Pos);
}

static inline void sam0_ep_clear_bank_ready(sam0_ep_reg_t *endpoint, const bool in)
{
	SAM0_EP_REG(endpoint, EPSTATUSCLR) = in ? BIT(USB_DEVICE_EPSTATUSCLR_BK1RDY_Pos) :
						  BIT(USB_DEVICE_EPSTATUSCLR_BK0RDY_Pos);
}

static inline void sam0_ep_set_stall(sam0_ep_reg_t *endpoint, const bool in)
{
	SAM0_EP_REG(endpoint, EPSTATUSSET) = in ? BIT(USB_DEVICE_EPSTATUSSET_STALLRQ1_Pos) :
						  BIT(USB_DEVICE_EPSTATUSSET_STALLRQ0_Pos);
}

static inline void sam0_ep_clear_stall(sam0_ep_reg_t *endpoint, const bool in)
{
	SAM0_EP_REG(endpoint, EPSTATUSCLR) = in ? BIT(USB_DEVICE_EPSTATUSCLR_STALLRQ1_Pos) :
						  BIT(USB_DEVICE_EPSTATUSCLR_STALLRQ0_Pos);
}

static inline void sam0_ep_clear_data_toggle(sam0_ep_reg_t *endpoint, const bool in)
{
	SAM0_EP_REG(endpoint, EPSTATUSCLR) = in ? BIT(USB_DEVICE_EPSTATUSCLR_DTGLIN_Pos) :
						  BIT(USB_DEVICE_EPSTATUSCLR_DTGLOUT_Pos);
}

static inline uint32_t sam0_ep_get_int_flags(const sam0_ep_reg_t *endpoint)
{
	return SAM0_EP_REG(endpoint, EPINTFLAG);
}

static inline void sam0_ep_clear_int_flags(sam0_ep_reg_t *endpoint, const uint32_t flags)
{
	SAM0_EP_REG(endpoint, EPINTFLAG) = flags;
}

static inline void sam0_ep_enable_int(sam0_ep_reg_t *endpoint, const uint32_t flags)
{
	SAM0_EP_REG(endpoint, EPINTENSET) = flags;
}

static inline void sam0_ep_disable_int(sam0_ep_reg_t *endpoint, const uint32_t flags)
{
	SAM0_EP_REG(endpoint, EPINTENCLR) = flags;
}

static inline void sam0_ep_set_type(sam0_ep_reg_t *endpoint, const bool in, const uint8_t type)
{
	if (in) {
		SAM0_EP_REG(endpoint, EPCFG) =
			(SAM0_EP_REG(endpoint, EPCFG) & ~USB_DEVICE_EPCFG_EPTYPE1_Msk) |
			USB_DEVICE_EPCFG_EPTYPE1(type);
	} else {
		SAM0_EP_REG(endpoint, EPCFG) =
			(SAM0_EP_REG(endpoint, EPCFG) & ~USB_DEVICE_EPCFG_EPTYPE0_Msk) |
			USB_DEVICE_EPCFG_EPTYPE0(type);
	}
}

/* Platform specific clock and pad calibration handling */

#if defined(CONFIG_ASF)

struct sam0_platform_config {
	uint8_t unused;
};

/* Nothing to take from devicetree, the ASF path uses fixed SoC resources */
#define UDC_SAM0_PLATFORM_DATA_DEFINE(n)
#define UDC_SAM0_PLATFORM_CONFIG(n)

static inline int sam0_platform_clock_enable(const struct sam0_platform_config *platform)
{
	ARG_UNUSED(platform);
#ifdef MCLK
	/* Enable the clock in MCLK */
	MCLK->APBBMASK.bit.USB_ = 1;

	/* Enable the GCLK - use 48 MHz source */
	GCLK->PCHCTRL[USB_GCLK_ID].reg = GCLK_PCHCTRL_GEN(2) | GCLK_PCHCTRL_CHEN;

	while (GCLK->SYNCBUSY.reg) {
	}
#else
	/* Enable the clock in PM */
	PM->APBBMASK.bit.USB_ = 1;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_USB | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
#endif

	return 0;
}

static inline int sam0_platform_clock_disable(const struct sam0_platform_config *platform)
{
	ARG_UNUSED(platform);
#ifdef MCLK
	/* Disable 48 MHz clock source in GCLK */
	GCLK->PCHCTRL[USB_GCLK_ID].reg = 0;

	/* Disable the clock in MCLK */
	MCLK->APBBMASK.bit.USB_ = 0;

	while (GCLK->SYNCBUSY.reg) {
	}
#else
	/* Disable clock source in GCLK */
	GCLK->CLKCTRL.reg = 0;

	/* Disable the clock in PM */
	PM->APBBMASK.bit.USB_ = 0;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
#endif

	return 0;
}

static inline int sam0_platform_load_padcal(sam0_usb_reg_t *base,
					    const struct sam0_platform_config *platform)
{
	ARG_UNUSED(platform);
	uint32_t pad_transn;
	uint32_t pad_transp;
	uint32_t pad_trim;

#ifdef USB_FUSES_TRANSN_ADDR
	pad_transn = *(uint32_t *)USB_FUSES_TRANSN_ADDR;
#else
#define NVM_USB_PAD_TRANSN_POS 45
#define NVM_USB_PAD_TRANSN_SIZE 5
	pad_transn = (*((uint32_t *)(NVMCTRL_OTP4) +
			(NVM_USB_PAD_TRANSN_POS / 32)) >>
		      (NVM_USB_PAD_TRANSN_POS % 32)) &
		     ((1 << NVM_USB_PAD_TRANSN_SIZE) - 1);

	if (pad_transn == 0x1F) {
		pad_transn = 5U;
	}
#endif

#ifdef USB_FUSES_TRANSP_ADDR
	pad_transp = *(uint32_t *)USB_FUSES_TRANSP_ADDR;
#else
#define NVM_USB_PAD_TRANSP_POS 50
#define NVM_USB_PAD_TRANSP_SIZE 5
	pad_transp = (*((uint32_t *)(NVMCTRL_OTP4) +
			(NVM_USB_PAD_TRANSP_POS / 32)) >>
		      (NVM_USB_PAD_TRANSP_POS % 32)) &
		     ((1 << NVM_USB_PAD_TRANSP_SIZE) - 1);

	if (pad_transp == 0x1F) {
		pad_transp = 29U;
	}
#endif

#ifdef USB_FUSES_TRIM_ADDR
	pad_trim = *(uint32_t *)USB_FUSES_TRIM_ADDR;
#else
#define NVM_USB_PAD_TRIM_POS 55
#define NVM_USB_PAD_TRIM_SIZE 3
	pad_trim = (*((uint32_t *)(NVMCTRL_OTP4) +
		      (NVM_USB_PAD_TRIM_POS / 32)) >>
		    (NVM_USB_PAD_TRIM_POS % 32)) &
		   ((1 << NVM_USB_PAD_TRIM_SIZE) - 1);

	if (pad_trim == 0x7) {
		pad_trim = 3U;
	}
#endif

	sam0_usb_set_padcal(base, USB_PADCAL_TRANSN(pad_transn) |
				  USB_PADCAL_TRANSP(pad_transp) |
				  USB_PADCAL_TRIM(pad_trim));

	return 0;
}

#else /* Microchip HAL */

struct sam0_platform_config {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_subsys;
	clock_control_subsys_t gclk_subsys;
	enum clock_mchp_gclkgen gclk_src;
	const uint8_t *calib;
	size_t calib_len;
	uintptr_t nvm_reg;
};

/* Clocks and pad calibration mapping are taken from devicetree */
#define UDC_SAM0_GCLKPERIPH_NODE(n) DT_CHILD(DT_NODELABEL(gclkperiph), usb##n)

#define UDC_SAM0_PLATFORM_DATA_DEFINE(n)					\
	static const uint8_t sam0_calib_##n[] = DT_INST_PROP(n, calib_mapping)

#define UDC_SAM0_PLATFORM_CONFIG(n)						\
	.platform = {								\
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),		\
		.mclk_subsys = (clock_control_subsys_t)				\
			DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),	\
		.gclk_subsys = (clock_control_subsys_t)				\
			DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),	\
		.gclk_src = (enum clock_mchp_gclkgen)DT_ENUM_IDX(		\
			UDC_SAM0_GCLKPERIPH_NODE(n), gclkperiph_src),		\
		.calib = sam0_calib_##n,					\
		.calib_len = DT_INST_PROP_LEN(n, calib_mapping),		\
		.nvm_reg = DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_calib)),		\
	},

static inline int sam0_platform_clock_enable(const struct sam0_platform_config *platform)
{
	struct clock_mchp_subsys_gclkperiph_config gclk_cfg = {
		.src = platform->gclk_src,
	};
	int ret;

	if (!device_is_ready(platform->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_configure(platform->clock_dev, platform->gclk_subsys, &gclk_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_on(platform->clock_dev, platform->gclk_subsys);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}

	ret = clock_control_on(platform->clock_dev, platform->mclk_subsys);
	if (ret != 0 && ret != -EALREADY) {
		(void)clock_control_off(platform->clock_dev, platform->gclk_subsys);
		return ret;
	}

	return 0;
}

static inline int sam0_platform_clock_disable(const struct sam0_platform_config *platform)
{
	int first_error = 0;
	int ret;

	ret = clock_control_off(platform->clock_dev, platform->mclk_subsys);
	if (ret != 0 && ret != -EALREADY) {
		first_error = ret;
	}

	ret = clock_control_off(platform->clock_dev, platform->gclk_subsys);
	if (ret != 0 && ret != -EALREADY && first_error == 0) {
		first_error = ret;
	}

	return first_error;
}

static inline int sam0_platform_load_padcal(sam0_usb_reg_t *base,
					    const struct sam0_platform_config *platform)
{
	uint32_t padcal = 0U;
	uint32_t otp_word = 0U;
	uintptr_t previous_addr = UINTPTR_MAX;

	if (platform->calib == NULL || platform->calib_len == 0U ||
	    (platform->calib_len % 4U) != 0U || platform->nvm_reg == 0U) {
		return -EINVAL;
	}

	/* calib-mapping is a list of <dst_bit nvm_bit mask default> tuples */
	for (size_t i = 0U; i < platform->calib_len; i += 4U) {
		const uint32_t dst = platform->calib[i];
		const uint32_t bit = platform->calib[i + 1U];
		const uint32_t mask = platform->calib[i + 2U];
		const uint32_t def = platform->calib[i + 3U];
		const uintptr_t addr = platform->nvm_reg + ((bit >> 5U) << 2U);
		uint32_t value;

		if (addr != previous_addr) {
			otp_word = sys_read32(addr);
			previous_addr = addr;
		}

		value = (otp_word >> (bit & 31U)) & mask;
		if (value == mask) {
			value = def;
		}

		padcal |= value << dst;
	}

	sam0_usb_set_padcal(base, padcal);

	return 0;
}

#endif /* CONFIG_ASF */

#endif /* ZEPHYR_DRIVERS_USB_UDC_UDC_SAM0_H_ */
