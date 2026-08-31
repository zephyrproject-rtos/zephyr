/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

struct dac12_gen_event_regs {
	volatile uint32_t IIDX;  /* !< (@ 0x00001050) Interrupt index */
	uint32_t RESERVED0;      /* !< Reserved*/
	volatile uint32_t IMASK; /* !< (@ 0x00001058) Interrupt mask */
	uint32_t RESERVED1;      /* !< Reserved*/
	volatile uint32_t RIS;   /* !< (@ 0x00001060) Raw interrupt status */
	uint32_t RESERVED2;      /* !< Reserved*/
	volatile uint32_t MIS;   /* !< (@ 0x00001068) Masked interrupt status */
	uint32_t RESERVED3;      /* !< Reserved*/
	volatile uint32_t ISET;  /* !< (@ 0x00001070) Interrupt set */
	uint32_t RESERVED4;      /* !< Reserved*/
	volatile uint32_t ICLR;  /* !< (@ 0x00001078) Interrupt clear */
};

struct dac12_cpu_init_regs {
	volatile uint32_t IIDX;  /* !< (@ 0x00001020) Interrupt index */
	uint32_t RESERVED0;      /* !< Reserved*/
	volatile uint32_t IMASK; /* !< (@ 0x00001028) Interrupt mask */
	uint32_t RESERVED1;      /* !< Reserved*/
	volatile uint32_t RIS;   /* !< (@ 0x00001030) Raw interrupt status */
	uint32_t RESERVED2;      /* !< Reserved*/
	volatile uint32_t MIS;   /* !< (@ 0x00001038) Masked interrupt status */
	uint32_t RESERVED3;      /* !< Reserved*/
	volatile uint32_t ISET;  /* !< (@ 0x00001040) Interrupt set */
	uint32_t RESERVED4;      /* !< Reserved*/
	volatile uint32_t ICLR;  /* !< (@ 0x00001048) Interrupt clear */
};

struct dac12_gprcm_regs {
	volatile uint32_t PWREN;  /* !< (@ 0x00000800) Power enable */
	volatile uint32_t RSTCTL; /* !< (@ 0x00000804) Reset Control */
	uint32_t RESERVED0[3];    /* !< Reserved*/
	volatile uint32_t STAT;   /* !< (@ 0x00000814) Status Register */
};

struct dac12_regs {
	uint32_t RESERVED0[256];
	volatile uint32_t FSUB_0;              /* !< (@ 0x00000400) Subscriber Port 0 */
	uint32_t RESERVED1[16];                /* !< Reserved*/
	volatile uint32_t FPUB_1;              /* !< (@ 0x00000444) Publisher port 1 */
	uint32_t RESERVED2[238];               /* !< Reserved*/
	struct dac12_gprcm_regs GPRCM;         /* !< (@ 0x00000800) */
	uint32_t RESERVED3[514];               /* !< Reserved*/
	struct dac12_cpu_init_regs CPU_INT;    /* !< (@ 0x00001020) */
	uint32_t RESERVED4;                    /* !< Reserved*/
	struct dac12_gen_event_regs GEN_EVENT; /* !< (@ 0x00001050) */
	uint32_t RESERVED5[25];                /* !< Reserved*/
	volatile uint32_t EVT_MODE;            /* !< (@ 0x000010E0) Event Mode */
	uint32_t RESERVED6[6];                 /* !< Reserved*/
	volatile uint32_t DESC;                /* !< (@ 0x000010FC) Module Description */
	volatile uint32_t CTL0;                /* !< (@ 0x00001100) Control 0 */
	uint32_t RESERVED7[3];                 /* !< Reserved*/
	volatile uint32_t CTL1;                /* !< (@ 0x00001110) Control 1 */
	uint32_t RESERVED8[3];                 /* !< Reserved*/
	volatile uint32_t CTL2;                /* !< (@ 0x00001120) Control 2 */
	uint32_t RESERVED9[3];                 /* !< Reserved*/
	volatile uint32_t CTL3;                /* !< (@ 0x00001130) Control 3 */
	uint32_t RESERVED10[3];                /* !< Reserved*/
	volatile uint32_t CALCTL;              /* !< (@ 0x00001140) Calibration control */
	uint32_t RESERVED11[7];                /* !< Reserved*/
	volatile uint32_t CALDATA;             /* !< (@ 0x00001160) Calibration data */
	uint32_t RESERVED12[39];               /* !< Reserved*/
	volatile uint32_t DATA0;               /* !< (@ 0x00001200) Data 0 */
};

/*
 * Compile-time checks
 */
BUILD_ASSERT(offsetof(struct dac12_regs, GPRCM) == 0x0800U);
BUILD_ASSERT(offsetof(struct dac12_regs, CPU_INT) == 0x1020U);
BUILD_ASSERT(offsetof(struct dac12_regs, GEN_EVENT) == 0x1050U);
BUILD_ASSERT(offsetof(struct dac12_regs, CTL0) == 0x1100U);
BUILD_ASSERT(offsetof(struct dac12_regs, CTL1) == 0x1110U);
BUILD_ASSERT(offsetof(struct dac12_regs, CALCTL) == 0x1140U);
BUILD_ASSERT(offsetof(struct dac12_regs, DATA0) == 0x1200U);

/*
 * Bit-field constants
 */

#ifndef CONFIG_HAS_MSPM0_SDK
/* DAC12_RSTCTL Bits */
#define DAC12_RSTCTL_RESETSTKYCLR_CLR   BIT(1)      /* !< Clear reset sticky bit */
#define DAC12_RSTCTL_RESETASSERT_ASSERT BIT(0)      /* !< Assert reset */
#define DAC12_RSTCTL_KEY_UNLOCK_W       0xB1000000U /* !< KEY to allow write access */

/* GPRCM.PWREN — writing the unlock key + enable bit powers the peripheral on */
#define DAC12_PWREN_KEY_UNLOCK_W  0x26000000U
#define DAC12_PWREN_ENABLE_ENABLE BIT(0)

/* CTL0 — main DAC enable/disable, resolution, and data format */
#define DAC12_CTL0_ENABLE_SET  BIT(0) /* bit 0: DAC on */
#define DAC12_CTL0_ENABLE_MASK BIT(0)
#define DAC12_CTL0_RES_MASK    BIT(8) /* bit 8: resolution select */
#define DAC12_CTL0_RES__8BITS  0x0
#define DAC12_CTL0_RES__12BITS BIT(8)
#define DAC12_CTL0_DFM_MASK    BIT(16) /* bit 16: data format (binary vs 2s-comp) */
#define DAC12_CTL0_DFM_BINARY  0x0

/* CTL1 — output amplifier, voltage reference, and output pin routing */
#define DAC12_CTL1_AMPEN_MASK      BIT(0)
#define DAC12_CTL1_AMPEN_ENABLE    BIT(0)  /* bit 0: amplifier enable */
#define DAC12_CTL1_AMPHIZ_MASK     BIT(1)  /* bit 1: amp-off output state */
#define DAC12_CTL1_AMPHIZ_PULLDOWN BIT(1)  /* pull DAC_OUT to 0 V when amp is off */
#define DAC12_CTL1_REFSP_MASK      BIT(8)  /* bit 8: positive reference select */
#define DAC12_CTL1_REFSP_VDDA      0x0     /* use VDDA as VR+ */
#define DAC12_CTL1_REFSP_VEREFP    BIT(8)  /* use VEREFP pin as VR+ */
#define DAC12_CTL1_REFSN_MASK      BIT(9)  /* bit 9: negative reference select */
#define DAC12_CTL1_REFSN_VEREFN    0x0     /* use VEREFN pin as VR- */
#define DAC12_CTL1_REFSN_VSSA      BIT(9)  /* use VSSA as VR- */
#define DAC12_CTL1_OPS_MASK        BIT(24) /* bit 24: output pin select */
#define DAC12_CTL1_OPS_OUT0        BIT(24) /* route output to DAC_OUT pin */

/* CALCTL — self-calibration trigger and trim source select */
#define DAC12_CALCTL_CALON_ACTIVE               BIT(0) /* bit 0: calibration running */
#define DAC12_CALCTL_CALSEL_SELFCALIBRATIONTRIM BIT(1) /* bit 1: use self-cal trim */

/* GEN_EVENT.RIS — raw interrupt status flags */
#define DAC12_GEN_EVENT_RIS_MODRDYIFG_SET BIT(1) /* bit 1: DAC core is ready */

/* DATA0 — the value written here appears on the DAC output */
#define DAC12_DATA0_DATA_VALUE_MASK GENMASK(11, 0) /* bits [11:0]: 12-bit data field */
#endif

#define DAC_RESOLUTION_8BIT	8
#define DAC_RESOLUTION_12BIT	12

#define DAC8_MAX_VALUE		255
#define DAC12_MAX_VALUE		4095

#define DAC_PRIMARY_CHANNEL_ID	0
#define DAC_READY_TIMEOUT_US	1000

#define DAC12_VREF_SOURCE_VEREFP_VEREFN (DAC12_CTL1_REFSP_VEREFP | DAC12_CTL1_REFSN_VEREFN)
#define DAC12_VREF_SOURCE_VDDA_VSSA     (DAC12_CTL1_REFSP_VDDA | DAC12_CTL1_REFSN_VSSA)

struct dac_mspm0_config {
	struct dac12_regs *base;
	uint32_t vref_ctl1_bits;
};

struct dac_mspm0_data {
	struct k_mutex lock;
	uint8_t resolution;
};

static int dac_mspm0_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;
	struct dac12_regs *regs = config->base;

	if (channel_cfg->channel_id != DAC_PRIMARY_CHANNEL_ID) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != DAC_RESOLUTION_8BIT &&
			channel_cfg->resolution != DAC_RESOLUTION_12BIT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* disable DAC before reconfiguring */
	regs->CTL0 &= ~DAC12_CTL0_ENABLE_MASK;

	/* set data format (binary) and resolution in CTL0 */
	uint32_t res_bits = (channel_cfg->resolution == DAC_RESOLUTION_12BIT)
				    ? DAC12_CTL0_RES__12BITS
				    : DAC12_CTL0_RES__8BITS;

	regs->CTL0 =
		(regs->CTL0 & ~(DAC12_CTL0_DFM_MASK | DAC12_CTL0_RES_MASK)) |
		((DAC12_CTL0_DFM_BINARY | res_bits) & (DAC12_CTL0_DFM_MASK | DAC12_CTL0_RES_MASK));

	/* configure amplifier, voltage reference, and output routing in CTL1 */
	uint32_t amp_bits =
		channel_cfg->buffered ? DAC12_CTL1_AMPEN_ENABLE : DAC12_CTL1_AMPHIZ_PULLDOWN;
	uint32_t ops_bits = channel_cfg->internal ? 0U : DAC12_CTL1_OPS_OUT0;
	uint32_t ctl1_mask = DAC12_CTL1_AMPEN_MASK | DAC12_CTL1_AMPHIZ_MASK |
			     DAC12_CTL1_REFSP_MASK | DAC12_CTL1_REFSN_MASK | DAC12_CTL1_OPS_MASK;

	regs->CTL1 = (regs->CTL1 & ~ctl1_mask) |
		     ((amp_bits | config->vref_ctl1_bits | ops_bits) & ctl1_mask);

	/* re-enable the DAC */
	regs->CTL0 |= DAC12_CTL0_ENABLE_SET;

	/* Wait for the DAC core and amplifier to settle */
	if (!WAIT_FOR(regs->GEN_EVENT.RIS & DAC12_GEN_EVENT_RIS_MODRDYIFG_SET, DAC_READY_TIMEOUT_US,
		      k_busy_wait(1))) {
		k_mutex_unlock(&data->lock);
		return -ETIMEDOUT;
	}

	data->resolution = channel_cfg->resolution;

	/* self-calibrate offset error if amplifier is active */
	if (channel_cfg->buffered) {
		regs->CALCTL = DAC12_CALCTL_CALON_ACTIVE | DAC12_CALCTL_CALSEL_SELFCALIBRATIONTRIM;
		if (!WAIT_FOR(regs->CALCTL & DAC12_CALCTL_CALON_ACTIVE, DAC_READY_TIMEOUT_US,
			      k_busy_wait(1))) {
			k_mutex_unlock(&data->lock);
			return -ETIMEDOUT;
		}
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int dac_mspm0_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;
	struct dac12_regs *regs = config->base;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Validate channel and resolution */
	if (channel != DAC_PRIMARY_CHANNEL_ID || data->resolution == 0) {
		ret = -EINVAL;
		goto unlock;
	}

	if (data->resolution == DAC_RESOLUTION_12BIT) {
		if (value > DAC12_MAX_VALUE) {
			ret = -EINVAL;
			goto unlock;
		}
		regs->DATA0 = value & DAC12_DATA0_DATA_VALUE_MASK;
	} else {
		if (value > DAC8_MAX_VALUE) {
			ret = -EINVAL;
			goto unlock;
		}
		regs->DATA0 = (uint8_t)value;
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int dac_mspm0_init(const struct device *dev)
{
	const struct dac_mspm0_config *config = dev->config;

	config->base->GPRCM.RSTCTL = DAC12_RSTCTL_KEY_UNLOCK_W | DAC12_RSTCTL_RESETSTKYCLR_CLR |
				     DAC12_RSTCTL_RESETASSERT_ASSERT;

	config->base->GPRCM.PWREN = DAC12_PWREN_KEY_UNLOCK_W | DAC12_PWREN_ENABLE_ENABLE;

	return 0;
}

static DEVICE_API(dac, dac_mspm0_driver_api) = {
	.channel_setup = dac_mspm0_channel_setup,
	.write_value   = dac_mspm0_write_value
};

#define DAC_MSPM0_DEFINE(id)									\
												\
	static const struct dac_mspm0_config dac_mspm0_config_##id = {				\
		.base = (struct dac12_regs *)DT_INST_REG_ADDR(id),				\
		.vref_ctl1_bits = COND_CODE_1(DT_INST_NODE_HAS_PROP(id, vref),			\
			DAC12_VREF_SOURCE_VEREFP_VEREFN,					\
			DAC12_VREF_SOURCE_VDDA_VSSA),						\
	};											\
												\
	static struct dac_mspm0_data dac_mspm0_data_##id = {					\
		.lock = Z_MUTEX_INITIALIZER(dac_mspm0_data_##id.lock),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id, &dac_mspm0_init, NULL, &dac_mspm0_data_##id,			\
			      &dac_mspm0_config_##id, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,	\
			      &dac_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MSPM0_DEFINE)
