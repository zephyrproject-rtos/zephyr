/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am13_cmpss_dac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <soc_sysctl.h>

/*
 * CMPSS-Lite register map (offsets from the TRM).
 *
 * All registers are 16 bits wide, packed at 2-byte offsets. Use 16-bit
 * accesses only; padding is declared as uint16_t so the layout stays packed.
 */
struct cmpss_lite_regs {
	volatile uint16_t compctl;          /* 0x00: Comparator control */
	volatile uint16_t comphysctl;       /* 0x02: Comparator hysteresis control */
	volatile uint16_t compsts;          /* 0x04: Comparator status */
	volatile uint16_t compstsclr;       /* 0x06: Comparator status clear */
	volatile uint16_t compdachctl;      /* 0x08: High DAC control */
	uint16_t reserved0;                 /* 0x0A: Reserved */
	volatile uint16_t dachvals;         /* 0x0C: High DAC value, shadow */
	volatile uint16_t dachvala;         /* 0x0E: High DAC value, active */
	uint16_t reserved1[10];             /* 0x10-0x23: Reserved */
	volatile uint16_t daclvals;         /* 0x24: Low DAC value, shadow */
	volatile uint16_t daclvala;         /* 0x26: Low DAC value, active */
	uint16_t reserved2[2];              /* 0x28-0x2B: Reserved */
	volatile uint16_t ctriplfilctl;     /* 0x2C: CTRIPL filter control */
	volatile uint16_t ctriplfilclkctl;  /* 0x2E: CTRIPL filter clock control */
	volatile uint16_t ctriphfilctl;     /* 0x30: CTRIPH filter control */
	volatile uint16_t ctriphfilclkctl;  /* 0x32: CTRIPH filter clock control */
	volatile uint16_t complock;         /* 0x34: Register lock */
	uint16_t reserved3[9];              /* 0x36-0x47: Reserved */
	volatile uint16_t compdaclctl;      /* 0x48: Low DAC control */
	uint16_t reserved4[18];             /* 0x4A-0x6D: Reserved */
	volatile uint16_t ctriplfilclkctl2; /* 0x6E: CTRIPL filter clock control 2 */
	uint16_t reserved5;                 /* 0x70: Reserved */
	volatile uint16_t ctriphfilclkctl2; /* 0x72: CTRIPH filter clock control 2 */
};

/*
 * Compile-time checks
 */
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, compctl) == 0x00U);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, compdachctl) == 0x08U);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, dachvals) == 0x0CU);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, daclvals) == 0x24U);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, ctriplfilctl) == 0x2CU);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, complock) == 0x34U);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, compdaclctl) == 0x48U);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, ctriplfilclkctl2) == 0x6EU);
BUILD_ASSERT(offsetof(struct cmpss_lite_regs, ctriphfilclkctl2) == 0x72U);

/* COMPCTL */
#define CMPSS_COMPCTL_COMPDACE    BIT(15) /* Comparator/DAC enable */
#define CMPSS_COMPCTL_COMPLSOURCE BIT(8)  /* 1: COMPL inverting input from pin, not DACL */

/*
 * Bits written to COMPCTL when the instance is used as a general purpose DAC.
 * COMPLSOURCE=1 detaches COMPL from DACL.
 */
#define CMPSS_COMPCTL_DAC_BITS (CMPSS_COMPCTL_COMPDACE | CMPSS_COMPCTL_COMPLSOURCE)

/* COMPDACHCTL, shared by DACH and DACL */
#define CMPSS_COMPDACHCTL_SWLOADSEL BIT(7) /* 0: DACxVALA loads from DACxVALS on SYSCLK */

/* DACLVALS / DACLVALA */
#define CMPSS_DACVAL_MASK GENMASK(11, 0)

#define CMPSS_DAC_CHANNEL_ID 0
#define CMPSS_DAC_RESOLUTION 12
#define CMPSS_DAC_MAX_VALUE  BIT_MASK(CMPSS_DAC_RESOLUTION)

struct dac_am13_config {
	DEVICE_MMIO_ROM;
	const struct device *sysctl;
	uint8_t ldacouten_bit;
	uint8_t pwren_bit;
	uint8_t reset_bit;
};

struct dac_am13_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
	bool configured;
};

static inline struct cmpss_lite_regs *dac_am13_regs(const struct device *dev)
{
	return (struct cmpss_lite_regs *)DEVICE_MMIO_GET(dev);
}

static int dac_am13_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_am13_config *config = dev->config;
	struct dac_am13_data *data = dev->data;
	struct cmpss_lite_regs *regs = dac_am13_regs(dev);
	int ret = 0;

	if (channel_cfg->channel_id != CMPSS_DAC_CHANNEL_ID) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != CMPSS_DAC_RESOLUTION) {
		return -ENOTSUP;
	}

	/* The internal path would feed COMPL, which is unusable in this mode */
	if (channel_cfg->internal) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	regs->daclvals = 0;

	/* Load DACLVALA from DACLVALS on SYSCLK, without any MCPWMSYNCPER */
	regs->compdachctl &= ~CMPSS_COMPDACHCTL_SWLOADSEL;

	regs->compctl |= CMPSS_COMPCTL_DAC_BITS;

	/* Route DACL to its pin last, once the DAC is configured */
	ret = syscon_update_bits(config->sysctl, SYSCTL_CMPSSCTL_OFFSET,
				 SYSCTL_CMPSSCTL_CMPSSCTLEN | BIT(config->ldacouten_bit),
				 SYSCTL_CMPSSCTL_CMPSSCTLEN | BIT(config->ldacouten_bit));
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->configured = true;

	k_mutex_unlock(&data->lock);

	return ret;
}

static int dac_am13_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac_am13_data *data = dev->data;
	struct cmpss_lite_regs *regs = dac_am13_regs(dev);

	if (channel != CMPSS_DAC_CHANNEL_ID || !data->configured) {
		return -EINVAL;
	}

	if (value > CMPSS_DAC_MAX_VALUE) {
		return -EINVAL;
	}

	regs->daclvals = value & CMPSS_DACVAL_MASK;

	return 0;
}

static int dac_am13_init(const struct device *dev)
{
	const struct dac_am13_config *config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->sysctl)) {
		return -ENODEV;
	}

	syscon_update_bits(config->sysctl, SYSCTL_RSTCTL_ASSERT_MCPERIPH_OFFSET,
			   SYSCTL_RSTCTL_MCPERIPH_KEY_UNLOCK | BIT(config->reset_bit),
			   SYSCTL_RSTCTL_MCPERIPH_KEY_UNLOCK | BIT(config->reset_bit));

	syscon_update_bits(config->sysctl, SYSCTL_RSTCTL_CLEAR_MCPERIPH_OFFSET,
			   SYSCTL_RSTCTL_MCPERIPH_KEY_UNLOCK | BIT(config->reset_bit),
			   SYSCTL_RSTCTL_MCPERIPH_KEY_UNLOCK | BIT(config->reset_bit));

	syscon_update_bits(config->sysctl, SYSCTL_PWREN_MCPERIPH_OFFSET,
			   SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | BIT(config->pwren_bit),
			   SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | BIT(config->pwren_bit));
	return 0;
}

static DEVICE_API(dac, dac_am13_driver_api) = {
	.channel_setup = dac_am13_channel_setup,
	.write_value = dac_am13_write_value,
};

#define DAC_AM13_DEFINE(id)                                                                        \
                                                                                                   \
	static const struct dac_am13_config dac_am13_config_##id = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(id)),                                             \
		.sysctl = DEVICE_DT_GET(DT_INST_PHANDLE(id, ti_sysctl)),                           \
		.ldacouten_bit = DT_INST_PROP(id, ti_ldacouten_bit),                               \
		.pwren_bit = DT_INST_PROP(id, ti_pwren_bit),                                       \
		.reset_bit = DT_INST_PROP(id, ti_reset_bit),                                       \
	};                                                                                         \
                                                                                                   \
	static struct dac_am13_data dac_am13_data_##id = {                                         \
		.lock = Z_MUTEX_INITIALIZER(dac_am13_data_##id.lock),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &dac_am13_init, NULL, &dac_am13_data_##id,                       \
			      &dac_am13_config_##id, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,        \
			      &dac_am13_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_AM13_DEFINE)
