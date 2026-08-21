/*
 * Copyright (c) 2026 Texas Instruments
 * copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ti_mspm0_adc12

#include <errno.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_mspm0);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mspm0_clock.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_MSPM0_CHANNEL_MAX            (32)
#define ADC_MSPM0_NUM_SAMPLE_TIMERS      2
#define ADC_MSPM0_MAX_OVERSAMPLING       7
#define ADC_MSPM0_RESOLUTION_14          14
#define ADC_MSPM0_RESOLUTION_12          12
#define ADC_MSPM0_RESOLUTION_10          10
#define ADC_MSPM0_RESOLUTION_8           8
#define ADC_MSPM0_VREF_MULTIPLIER        1000
#define ADC_MSPM0_SCOMP0                 0
#define ADC_MSPM0_SCOMP1                 1
#define ADC_MSPM0_WAKEUP_TIME_NS         5000
#define ADC_MSPM0_NS_PER_SEC             1000000000
#define INT_VREF                         BIT(0)
#define EXT_VREF                         BIT(1)

/*
 * CPU_INT, GEN_EVENT and DMA_TRIG share this interrupt-controller register
 * layout; offsets below are relative to each block's own base address.
 */
struct adc_mspm0_int_regs {
	volatile const uint32_t iidx; /**< IIDX Interrupt index, offset: +0x00 */
	uint32_t reserved0;           /**< Reserved, offset: +0x04 - +0x08 */
	volatile uint32_t imask;      /**< IMASK Interrupt mask, offset: +0x08 */
	uint32_t reserved1;           /**< Reserved, offset: +0x0C - +0x10 */
	volatile const uint32_t ris;  /**< RIS Raw interrupt status, offset: +0x10 */
	uint32_t reserved2;           /**< Reserved, offset: +0x14 - +0x18 */
	volatile const uint32_t mis;  /**< MIS Masked interrupt status, offset: +0x18 */
	uint32_t reserved3;           /**< Reserved, offset: +0x1C - +0x20 */
	volatile uint32_t iset;       /**< ISET Interrupt set, offset: +0x20 */
	uint32_t reserved4;           /**< Reserved, offset: +0x24 - +0x28 */
	volatile uint32_t iclr;       /**< ICLR Interrupt clear, offset: +0x28 */
};

struct adc_mspm0_gprcm {
	volatile uint32_t pwren;      /**< PWREN Power enable, offset: 0x800 */
	volatile uint32_t rstctl;     /**< RSTCTL Reset Control, offset: 0x804 */
	volatile uint32_t clkcfg;     /**< CLKCFG ADC clock configuration Register, offset: 0x808 */
	uint32_t reserved[2];         /**< Reserved, offset: 0x80C - 0x814 */
	volatile const uint32_t stat; /**< STAT Status Register, offset: 0x814 */
};

/* ADC12 register map */
struct adc_mspm0_regs {
	uint32_t reserved0[256];  /**< Reserved, offset: 0x000 - 0x400 */
	volatile uint32_t fsub_0; /**< FSUB_0 Subscriber Configuration Register, offset: 0x400 */
	uint32_t reserved1[16];   /**< Reserved, offset: 0x404 - 0x444 */
	volatile uint32_t fpub_1; /**< FPUB_1 Publisher Configuration Register, offset: 0x444 */
	uint32_t reserved2[238];  /**< Reserved, offset: 0x448 - 0x800 */
	struct adc_mspm0_gprcm gprcm;        /**< Power/reset/clock control block, offset: 0x800 */
	uint32_t reserved3[514];             /**< Reserved, offset: 0x818 - 0x1020 */
	struct adc_mspm0_int_regs cpu_int;   /**< CPU interrupt block, offset: 0x1020 */
	uint32_t reserved4;                  /**< Reserved, offset: 0x104C - 0x1050 */
	struct adc_mspm0_int_regs gen_event; /**< General event interrupt block, offset: 0x1050 */
	uint32_t reserved5;                  /**< Reserved, offset: 0x107C - 0x1080 */
	struct adc_mspm0_int_regs dma_trig;  /**< DMA trigger interrupt block, offset: 0x1080 */
	uint32_t reserved6[13];              /**< Reserved, offset: 0x10AC - 0x10E0 */
	volatile const uint32_t evt_mode;    /**< EVT_MODE Event Mode, offset: 0x10E0 */
	uint32_t reserved7[6];               /**< Reserved, offset: 0x10E4 - 0x10FC */
	volatile const uint32_t desc;        /**< DESC Module Description, offset: 0x10FC */
	volatile uint32_t ctl0;              /**< CTL0 Control Register 0, offset: 0x1100 */
	volatile uint32_t ctl1;              /**< CTL1 Control Register 1, offset: 0x1104 */
	volatile uint32_t ctl2;              /**< CTL2 Control Register 2, offset: 0x1108 */
	uint32_t reserved8;                  /**< Reserved, offset: 0x110C - 0x1110 */
	volatile uint32_t clkfreq; /**< CLKFREQ Sample Clock Frequency Range, offset: 0x1110 */
	volatile uint32_t scomp0;  /**< SCOMP0 Sample Time Compare 0 Register, offset: 0x1114 */
	volatile uint32_t scomp1;  /**< SCOMP1 Sample Time Compare 1 Register, offset: 0x1118 */
	uint32_t reserved9[11];    /**< Reserved, offset: 0x111C - 0x1148 */
	volatile uint32_t wclow;   /**< WCLOW Window Comparator Low Threshold, offset: 0x1148 */
	uint32_t reserved10;       /**< Reserved, offset: 0x114C - 0x1150 */
	volatile uint32_t wchigh;  /**< WCHIGH Window Comparator High Threshold, offset: 0x1150 */
	uint32_t reserved11[3];    /**< Reserved, offset: 0x1154 - 0x1160 */
	volatile const uint32_t fifodata;   /**< FIFODATA FIFO Data Register, offset: 0x1160 */
	uint32_t reserved12[7];             /**< Reserved, offset: 0x1164 - 0x1180 */
	volatile uint32_t memctl[24];       /**< MEMCTL_y Memory Control, offset: 0x1180+0x04y */
	uint32_t reserved13[40];            /**< Reserved, offset: 0x11E0 - 0x1280 */
	volatile const uint32_t memres[24]; /**< MEMRES_y Memory Result, offset: 0x1280+0x04y */
	uint32_t reserved14[24];            /**< Reserved, offset: 0x12E0 - 0x1340 */
	volatile const uint32_t status;     /**< STATUS Status Register, offset: 0x1340 */
};

/* pwren bits */
#define ADC12_PWREN_ENABLE         BIT(0)
#define ADC12_PWREN_KEY            GENMASK(31, 24)
#define ADC12_PWREN_KEY_VAL_UNLOCK 0x26U

/* clkcfg bits */
#define ADC12_CLKCFG_SAMPCLK        GENMASK(1, 0)
#define ADC12_CLKCFG_KEY            GENMASK(31, 24)
#define ADC12_CLKCFG_KEY_VAL_UNLOCK 0xA9U

/* CLKCFG.SAMPCLK source select values (0 = ULPCLK, 1 = SYSOSC, 2 = HFCLK) */
#define ADC12_CLKCFG_SAMPCLK_VAL_ULPCLK FIELD_PREP(ADC12_CLKCFG_SAMPCLK, 0)
#define ADC12_CLKCFG_SAMPCLK_VAL_SYSOSC FIELD_PREP(ADC12_CLKCFG_SAMPCLK, 1)
#define ADC12_CLKCFG_SAMPCLK_VAL_HFCLK  FIELD_PREP(ADC12_CLKCFG_SAMPCLK, 2)

/* ctl0 bits */
#define ADC12_CTL0_ENC              BIT(0)
#define ADC12_CTL0_PWRDN            BIT(16)
#define ADC12_CTL0_PWRDN_VAL_AUTO   0U
#define ADC12_CTL0_PWRDN_VAL_MANUAL ADC12_CTL0_PWRDN
#define ADC12_CTL0_SCLKDIV          GENMASK(26, 24)

#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_1  0U
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_2  FIELD_PREP(ADC12_CTL0_SCLKDIV, 1)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_4  FIELD_PREP(ADC12_CTL0_SCLKDIV, 2)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_8  FIELD_PREP(ADC12_CTL0_SCLKDIV, 3)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_16 FIELD_PREP(ADC12_CTL0_SCLKDIV, 4)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_24 FIELD_PREP(ADC12_CTL0_SCLKDIV, 5)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_32 FIELD_PREP(ADC12_CTL0_SCLKDIV, 6)
#define ADC12_CTL0_SCLKDIV_VAL_DIV_BY_48 FIELD_PREP(ADC12_CTL0_SCLKDIV, 7)

/* ctl1 bits */
#define ADC12_CTL1_TRIGSRC              BIT(0)
#define ADC12_CTL1_TRIGSRC_VAL_SOFTWARE 0U
#define ADC12_CTL1_SC                   BIT(8)
#define ADC12_CTL1_CONSEQ               GENMASK(17, 16)
#define ADC12_CTL1_CONSEQ_VAL_SEQUENCE  FIELD_PREP(ADC12_CTL1_CONSEQ, 1)
#define ADC12_CTL1_SAMPMODE             BIT(20)
#define ADC12_CTL1_SAMPMODE_VAL_AUTO    0U
#define ADC12_CTL1_AVGN                 GENMASK(26, 24)
#define ADC12_CTL1_AVGD                 GENMASK(30, 28)

#define ADC12_CTL1_AVGN_VAL_DISABLED 0U
#define ADC12_CTL1_AVGN_VAL_ACC_2    FIELD_PREP(ADC12_CTL1_AVGN, 1)
#define ADC12_CTL1_AVGN_VAL_ACC_4    FIELD_PREP(ADC12_CTL1_AVGN, 2)
#define ADC12_CTL1_AVGN_VAL_ACC_8    FIELD_PREP(ADC12_CTL1_AVGN, 3)
#define ADC12_CTL1_AVGN_VAL_ACC_16   FIELD_PREP(ADC12_CTL1_AVGN, 4)
#define ADC12_CTL1_AVGN_VAL_ACC_32   FIELD_PREP(ADC12_CTL1_AVGN, 5)
#define ADC12_CTL1_AVGN_VAL_ACC_64   FIELD_PREP(ADC12_CTL1_AVGN, 6)
#define ADC12_CTL1_AVGN_VAL_ACC_128  FIELD_PREP(ADC12_CTL1_AVGN, 7)

#define ADC12_CTL1_AVGD_VAL_DIV_BY_1   0U
#define ADC12_CTL1_AVGD_VAL_DIV_BY_2   FIELD_PREP(ADC12_CTL1_AVGD, 1)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_4   FIELD_PREP(ADC12_CTL1_AVGD, 2)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_8   FIELD_PREP(ADC12_CTL1_AVGD, 3)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_16  FIELD_PREP(ADC12_CTL1_AVGD, 4)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_32  FIELD_PREP(ADC12_CTL1_AVGD, 5)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_64  FIELD_PREP(ADC12_CTL1_AVGD, 6)
#define ADC12_CTL1_AVGD_VAL_DIV_BY_128 FIELD_PREP(ADC12_CTL1_AVGD, 7)

/* ctl2 bits */
#define ADC12_CTL2_DF                   BIT(0)
#define ADC12_CTL2_DF_VAL_UNSIGNED      0U
#define ADC12_CTL2_RES                  GENMASK(2, 1)
#define ADC12_CTL2_RES_VAL_12_BIT       0U
#define ADC12_CTL2_RES_VAL_10_BIT       FIELD_PREP(ADC12_CTL2_RES, 1)
#define ADC12_CTL2_RES_VAL_8_BIT        FIELD_PREP(ADC12_CTL2_RES, 2)
#define ADC12_CTL2_STARTADD             GENMASK(20, 16)
#define ADC12_CTL2_STARTADD_VAL_ADDR_00 0U
#define ADC12_CTL2_ENDADD               GENMASK(28, 24)

/* scomp0/scomp1 bits */
#define ADC12_SCOMP_VAL GENMASK(9, 0)

/* memctl bits */
#define ADC12_MEMCTL_CHANSEL             GENMASK(4, 0)
#define ADC12_MEMCTL_VRSEL               GENMASK(10, 8)
#define ADC12_MEMCTL_VRSEL_VAL_VDDA      0U
#define ADC12_MEMCTL_VRSEL_VAL_EXTREF    1U
#define ADC12_MEMCTL_VRSEL_VAL_INTREF    2U
#define ADC12_MEMCTL_STIME               BIT(12)
#define ADC12_MEMCTL_AVGEN               BIT(16)
#define ADC12_MEMCTL_AVGEN_VAL_DISABLE   0U
#define ADC12_MEMCTL_AVGEN_VAL_ENABLE    ADC12_MEMCTL_AVGEN
#define ADC12_MEMCTL_BCSEN_VAL_DISABLE   0U
#define ADC12_MEMCTL_TRIG_VAL_AUTO_NEXT  0U
#define ADC12_MEMCTL_WINCOMP_VAL_DISABLE 0U

/*
 * ADC12 registers have an alias at 0x556000 offset, these registers can be read at MCLK rate
 * instead of ULPCLK which is used with unaliased registers. Subtract 0x1000 since there are no
 * power/clock registers in the aliased region.
 */
#define ADC12_ALIAS_OFFSET 0x555000U

/* cpu_int bits: only the last MEMCTL of a sequence raises MEMRESIFG(n) */
#define ADC12_CPU_INT_MEMRESIFG0_OFS 8U

/* cpu_int.iidx values for a MEMRES result-loaded interrupt */
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG0  9U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG1  10U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG2  11U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG3  12U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG4  13U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG5  14U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG6  15U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG7  16U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG8  17U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG9  18U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG10 19U
#define ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG11 20U

enum mspm0_oversampling {
	ADC_MSPM0_AVG_DISABLED,
	ADC_MSPM0_AVG_2X,
	ADC_MSPM0_AVG_4X,
	ADC_MSPM0_AVG_8X,
	ADC_MSPM0_AVG_16X,
	ADC_MSPM0_AVG_32X,
	ADC_MSPM0_AVG_64X,
	ADC_MSPM0_AVG_128X
};

struct adc_mspm0_channel_cfg {
	uint8_t vrsel: 2; /* raw ADC12_MEMCTL_VRSEL index: 0=VDDA, 1=EXTREF, 2=INTREF */
	uint8_t stime: 1; /* raw ADC12_MEMCTL_STIME index: 0=SCOMP0, 1=SCOMP1 */
	bool configured: 1;
} __packed;

struct adc_mspm0_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint16_t sample_time[ADC_MSPM0_NUM_SAMPLE_TIMERS];
	struct adc_mspm0_channel_cfg channel_cfg[ADC_MSPM0_CHANNEL_MAX];
	uint8_t channel_eoc;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	uint8_t vref_flags;
#endif
};

struct adc_mspm0_cfg {
	struct adc_mspm0_regs *regs;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_cfg_func)(void);
	const struct device *vref_config;
	const struct mspm0_sys_clock *clock_subsys;
	const uint32_t clkcfg_sampclk;
	uint32_t clock_div_reg;
	uint32_t clock_range;
	const uint8_t divider;
	const uint8_t max_result;
	const uint8_t num_channels;
	bool auto_pwdn;
};

static inline uint16_t adc_mspm0_get_mem_result(struct adc_mspm0_regs *regs, uint8_t idx)
{
	volatile struct adc_mspm0_regs *alias_regs =
		(struct adc_mspm0_regs *)((uintptr_t)regs + ADC12_ALIAS_OFFSET);

	return alias_regs->memres[idx];
}

static void adc_mspm0_isr(const struct device *dev)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	struct adc_mspm0_regs *regs = config->regs;
	uint8_t mem_ix;

	switch (regs->cpu_int.iidx) {
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG0:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG1:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG2:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG3:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG4:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG5:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG6:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG7:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG8:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG9:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG10:
	case ADC12_CPU_INT_IIDX_STAT_VAL_MEMRESIFG11:
		for (mem_ix = 0; mem_ix <= data->channel_eoc; mem_ix++) {
			data->buffer[mem_ix] = adc_mspm0_get_mem_result(regs, mem_ix);
		}
		regs->cpu_int.imask &=
			~((1 << (data->channel_eoc)) << ADC12_CPU_INT_MEMRESIFG0_OFS);
		break;
	default:
		LOG_ERR("unexpected interrupt");
		break;
	}
	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mspm0_data *data = CONTAINER_OF(ctx, struct adc_mspm0_data, ctx);
	const struct device *dev = data->dev;
	const struct adc_mspm0_cfg *config = dev->config;
	struct adc_mspm0_regs *regs = config->regs;
	uint32_t memresifg = (1 << data->channel_eoc) << ADC12_CPU_INT_MEMRESIFG0_OFS;

	regs->cpu_int.iclr |= memresifg;
	regs->cpu_int.imask |= memresifg;
	regs->ctl0 |= ADC12_CTL0_ENC;
	regs->ctl1 |= ADC12_CTL1_SC;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_mspm0_data *data = CONTAINER_OF(ctx, struct adc_mspm0_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->channel_eoc + 1;
	}
}

static int adc_mspm0_validate_sampling_time(const struct adc_mspm0_cfg *config, uint16_t acq_time)
{
	uint32_t clock_rate;
	uint32_t period_ns;
	int ret;
	uint8_t wakeup_cycles;

	ret = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(ckm)),
				     (struct mspm0_sys_clock *)config->clock_subsys, &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (!config->auto_pwdn) {
		if (acq_time == ADC_ACQ_TIME_DEFAULT) {
			return 0;
		}

		if ((ADC_ACQ_TIME_UNIT(acq_time) == ADC_ACQ_TIME_TICKS) &&
		    (ADC_ACQ_TIME_VALUE(acq_time) <= ADC12_SCOMP_VAL)) {
			return ADC_ACQ_TIME_VALUE(acq_time);
		} else {
			return -EINVAL;
		}
	} else {
		period_ns = ADC_MSPM0_NS_PER_SEC / (clock_rate / config->divider);
		/* Wakeup time is 5us for both l and g mspm0 series */
		wakeup_cycles = ADC_MSPM0_WAKEUP_TIME_NS / period_ns;

		if (acq_time == ADC_ACQ_TIME_DEFAULT) {
			return wakeup_cycles + 1;
		}

		if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
			return -EINVAL;
		}

		acq_time = ADC_ACQ_TIME_VALUE(acq_time) + wakeup_cycles;
		if (acq_time > ADC12_SCOMP_VAL) {
			return -EINVAL;
		}
		return acq_time;
	}
}

static int adc_mspm0_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	struct adc_mspm0_regs *regs = config->regs;
	const uint8_t ch = channel_cfg->channel_id;
	int sampling_time;
	int ret = 0;

	if ((ch >= config->num_channels) ||
	    (channel_cfg->differential) ||
	    (channel_cfg->gain != ADC_GAIN_1)) {
		return -EINVAL;
	}

	sampling_time = adc_mspm0_validate_sampling_time(dev->config,
							 channel_cfg->acquisition_time);
	if (sampling_time < 0) {
		return sampling_time;
	}

	adc_context_lock(&data->ctx, false, NULL);

	if (data->sample_time[ADC_MSPM0_SCOMP0] == UINT16_MAX) {
		regs->scomp0 = sampling_time;
		data->sample_time[ADC_MSPM0_SCOMP0] = sampling_time;
		data->channel_cfg[ch].stime = ADC_MSPM0_SCOMP0;
	} else if (data->sample_time[ADC_MSPM0_SCOMP0] == sampling_time) {
		data->channel_cfg[ch].stime = ADC_MSPM0_SCOMP0;
	} else if (data->sample_time[ADC_MSPM0_SCOMP1] == UINT16_MAX) {
		regs->scomp1 = sampling_time;
		data->sample_time[ADC_MSPM0_SCOMP1] = sampling_time;
		data->channel_cfg[ch].stime = ADC_MSPM0_SCOMP1;
	} else if (data->sample_time[ADC_MSPM0_SCOMP1] == sampling_time) {
		data->channel_cfg[ch].stime = ADC_MSPM0_SCOMP1;
	} else {
		ret = -EINVAL;
		goto unlock;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		data->channel_cfg[ch].vrsel = ADC12_MEMCTL_VRSEL_VAL_VDDA;
		break;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	case ADC_REF_INTERNAL:
		data->channel_cfg[ch].vrsel = ADC12_MEMCTL_VRSEL_VAL_INTREF;

		if ((data->vref_flags & EXT_VREF) || !(config->vref_config)) {
			ret = -EINVAL;
			goto unlock;
		}

		if (!(data->vref_flags & INT_VREF)) {
			if (regulator_enable(config->vref_config) < 0) {
				ret = -ENODEV;
				goto unlock;
			}
			data->vref_flags |= INT_VREF;
		}
		break;
	case ADC_REF_EXTERNAL0:
		data->channel_cfg[ch].vrsel = ADC12_MEMCTL_VRSEL_VAL_EXTREF;

		if ((data->vref_flags & INT_VREF) || !(config->vref_config)) {
			ret = -EINVAL;
			goto unlock;
		}

		if (regulator_is_enabled(config->vref_config)) {
			LOG_ERR("Cannot use external vref: regulator is already in use");
			ret = -EBUSY;
			goto unlock;
		}

		if (!(data->vref_flags & EXT_VREF)) {
			data->vref_flags |= EXT_VREF;
		}
		break;
#endif
	default:
		ret = -EINVAL;
		goto unlock;
	}

	data->channel_cfg[ch].configured = true;

unlock:
	adc_context_release(&data->ctx, 0);
	return ret;
}

static int adc_mspm0_config_sequence(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	struct adc_mspm0_regs *regs = config->regs;
	uint32_t channels = seq->channels;
	uint32_t resolution;
	uint32_t avg_enabled = 0;
	uint32_t avg_acc = 0;
	uint32_t avg_div = 0;
	uint32_t vrsel;
	uint32_t stime;
	uint8_t mem_ctl_count = 0;
	uint8_t ch;

	switch (seq->resolution) {
	case ADC_MSPM0_RESOLUTION_14:
	case ADC_MSPM0_RESOLUTION_12:
		resolution = ADC12_CTL2_RES_VAL_12_BIT;
		break;
	case ADC_MSPM0_RESOLUTION_10:
		resolution = ADC12_CTL2_RES_VAL_10_BIT;
		break;
	case ADC_MSPM0_RESOLUTION_8:
		resolution = ADC12_CTL2_RES_VAL_8_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (seq->oversampling) {
	case ADC_MSPM0_AVG_DISABLED:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_DISABLE;
		break;
	case ADC_MSPM0_AVG_2X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_2;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_2;
		break;
	case ADC_MSPM0_AVG_4X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_4;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_4;
		break;
	case ADC_MSPM0_AVG_8X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_8;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_8;
		break;
	case ADC_MSPM0_AVG_16X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_16;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_16;
		break;
	case ADC_MSPM0_AVG_32X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_32;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_32;
		break;
	case ADC_MSPM0_AVG_64X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_64;
		avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_64;
		break;
	case ADC_MSPM0_AVG_128X:
		avg_enabled = ADC12_MEMCTL_AVGEN_VAL_ENABLE;
		avg_acc = ADC12_CTL1_AVGN_VAL_ACC_128;
		if (seq->resolution == 14) {
			avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_32;
		} else {
			avg_div = ADC12_CTL1_AVGD_VAL_DIV_BY_128;
		}
		break;
	default:
		return -EINVAL;
	}

	while (channels) {
		ch = find_lsb_set(channels) - 1;
		if ((ch >= config->num_channels) || !data->channel_cfg[ch].configured) {
			return -EINVAL;
		}

		vrsel = FIELD_PREP(ADC12_MEMCTL_VRSEL, data->channel_cfg[ch].vrsel);
		stime = FIELD_PREP(ADC12_MEMCTL_STIME, data->channel_cfg[ch].stime);

		if (mem_ctl_count < config->max_result) {
			regs->memctl[mem_ctl_count] =
				FIELD_PREP(ADC12_MEMCTL_CHANSEL, ch) | vrsel | stime | avg_enabled |
				ADC12_MEMCTL_BCSEN_VAL_DISABLE | ADC12_MEMCTL_TRIG_VAL_AUTO_NEXT |
				ADC12_MEMCTL_WINCOMP_VAL_DISABLE;
		} else {
			return -EINVAL;
		}

		mem_ctl_count++;
		channels &= ~BIT(ch);
	}

	regs->ctl1 = (regs->ctl1 & ~(ADC12_CTL1_AVGN | ADC12_CTL1_AVGD)) | avg_acc | avg_div;

	if (mem_ctl_count - 1 != data->channel_eoc) {
		return -EINVAL;
	}

	regs->ctl1 =
		(regs->ctl1 & ~(ADC12_CTL1_SAMPMODE | ADC12_CTL1_CONSEQ | ADC12_CTL1_TRIGSRC)) |
		ADC12_CTL1_CONSEQ_VAL_SEQUENCE | ADC12_CTL1_SAMPMODE_VAL_AUTO |
		ADC12_CTL1_TRIGSRC_VAL_SOFTWARE;

	regs->ctl2 = (regs->ctl2 &
		      ~(ADC12_CTL2_ENDADD | ADC12_CTL2_STARTADD | ADC12_CTL2_RES | ADC12_CTL2_DF)) |
		     ADC12_CTL2_STARTADD_VAL_ADDR_00 |
		     FIELD_PREP(ADC12_CTL2_ENDADD, data->channel_eoc) | resolution |
		     ADC12_CTL2_DF_VAL_UNSIGNED;

	return 0;
}

static int adc_mspm0_read_internal(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	int sequence_ret;
	uint8_t ch_count = POPCOUNT(sequence->channels);
	size_t exp_size;

	if ((sequence->resolution != ADC_MSPM0_RESOLUTION_14) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_12) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_10) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_8)) {
		return -EINVAL;
	}

	if (sequence->resolution == ADC_MSPM0_RESOLUTION_14 &&
	    sequence->oversampling != ADC_MSPM0_MAX_OVERSAMPLING) {
		return -EINVAL;
	}

	if ((ch_count == 0) || (ch_count > config->max_result) ||
	    (sequence->oversampling > ADC_MSPM0_MAX_OVERSAMPLING)) {
		return -EINVAL;
	}

	if (sequence->calibrate) {
		return -ENOTSUP;
	}

	exp_size = ch_count * sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		return -ENOMEM;
	}

	data->channel_eoc = ch_count - 1;
	data->buffer = sequence->buffer;
	data->repeat_buffer = data->buffer;

	sequence_ret = adc_mspm0_config_sequence(dev, sequence);
	if (sequence_ret < 0) {
		return sequence_ret;
	}

	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_mspm0_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mspm0_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mspm0_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mspm0_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_mspm0_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_mspm0_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int adc_mspm0_init(const struct device *dev)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	struct adc_mspm0_regs *regs = config->regs;
	int ret;

	data->dev = dev;

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	regs->gprcm.pwren =
		FIELD_PREP(ADC12_PWREN_KEY, ADC12_PWREN_KEY_VAL_UNLOCK) | ADC12_PWREN_ENABLE;
	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

	regs->gprcm.clkcfg = (regs->gprcm.clkcfg & ~(ADC12_CLKCFG_KEY | ADC12_CLKCFG_SAMPCLK)) |
			     FIELD_PREP(ADC12_CLKCFG_KEY, ADC12_CLKCFG_KEY_VAL_UNLOCK) |
			     config->clkcfg_sampclk;
	regs->ctl0 = (regs->ctl0 & ~ADC12_CTL0_SCLKDIV) | config->clock_div_reg;
	regs->clkfreq = config->clock_range;

	if (config->auto_pwdn) {
		regs->ctl0 = (regs->ctl0 & ~ADC12_CTL0_PWRDN) | ADC12_CTL0_PWRDN_VAL_AUTO;
	} else {
		regs->ctl0 = (regs->ctl0 & ~ADC12_CTL0_PWRDN) | ADC12_CTL0_PWRDN_VAL_MANUAL;
	}

	data->sample_time[ADC_MSPM0_SCOMP0] = UINT16_MAX;
	data->sample_time[ADC_MSPM0_SCOMP1] = UINT16_MAX;

	for (int i = 0; i < ADC_MSPM0_CHANNEL_MAX; i++) {
		data->channel_cfg[i].configured = 0;
	}
	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define MSPM0_ADC_VREF_MV(index)								   \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, vref),						   \
		    (DT_PROP(DT_PHANDLE(DT_DRV_INST(index), vref), regulator_uv) / 1000), (0))	   \

#define MSPM0_ADC_VREF(index) | MSPM0_ADC_VREF_MV(index)

static DEVICE_API(adc, mspm0_driver_api) = {
	.channel_setup = adc_mspm0_channel_setup,
	.read = adc_mspm0_read,
	.ref_internal = (0 DT_INST_FOREACH_STATUS_OKAY(MSPM0_ADC_VREF)),
	IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_mspm0_read_async,))
};

#define ADC_CLOCK_DIV(x)    DT_INST_PROP(x, ti_clk_divider)
#define ADC_DT_CLOCK_DIV(x) _CONCAT(ADC12_CTL0_SCLKDIV_VAL_DIV_BY_, ADC_CLOCK_DIV(x))

#define ADC_DT_CLOCK_RANGE(x) DT_INST_PROP(x, ti_clk_range)

/*
 * CLKCFG.SAMPCLK only accepts ULPCLK/SYSOSC/HFCLK; resolve the DT clock cell
 * (already captured in mspm0_sys_clock.clk) to its 2-bit register value at
 * build time instead of a runtime lookup.
 */
#define ADC_DT_SAMPCLK(x)                                                                          \
	((DT_INST_CLOCKS_CELL(x, clk) == MSPM0_CLOCK_ULPCLK)   ? ADC12_CLKCFG_SAMPCLK_VAL_ULPCLK   \
	 : (DT_INST_CLOCKS_CELL(x, clk) == MSPM0_CLOCK_SYSOSC) ? ADC12_CLKCFG_SAMPCLK_VAL_SYSOSC   \
	 : (DT_INST_CLOCKS_CELL(x, clk) == MSPM0_CLOCK_HFCLK)  ? ADC12_CLKCFG_SAMPCLK_VAL_HFCLK    \
							       : -1)

#define MSPM0_ADC_INIT(index)                                                                      \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	BUILD_ASSERT(ADC_DT_SAMPCLK(index) != -1,                                                  \
		     "unsupported ADC sample clock source: must be ULPCLK, SYSOSC or HFCLK");      \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(index, ti_num_channels) <= ADC_MSPM0_CHANNEL_MAX,                \
		     "unsupported number of channels, ADC can only support max 32 channels");      \
                                                                                                   \
	static void adc_mspm0_cfg_func_##index(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), adc_mspm0_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}											   \
	static const struct mspm0_sys_clock mspm0_adc_sys_clock##index =                           \
						MSPM0_CLOCK_SUBSYS_FN(index);			   \
                                                                                                   \
	static const struct adc_mspm0_cfg adc_mspm0_cfg_##index = {                                \
		.regs = (struct adc_mspm0_regs *)DT_INST_REG_ADDR(index),                          \
		.irq_cfg_func = adc_mspm0_cfg_func_##index,                                        \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.clock_subsys = &mspm0_adc_sys_clock##index,                                       \
		.divider = ADC_CLOCK_DIV(index),                                                   \
		.clkcfg_sampclk = ADC_DT_SAMPCLK(index),                                           \
		.clock_range = ADC_DT_CLOCK_RANGE(index),                                          \
		.clock_div_reg = ADC_DT_CLOCK_DIV(index),                                          \
		.max_result = DT_INST_PROP(index, max_result_reg),                                 \
		.num_channels = DT_INST_PROP(index, ti_num_channels),                              \
		.auto_pwdn = DT_INST_PROP_OR(index, auto_powerdown, false),                        \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(index, vref),					   \
		(.vref_config = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(index), vref)),),		   \
		(.vref_config = NULL)) };							   \
	static struct adc_mspm0_data adc_mspm0_data_##index = {                                    \
		ADC_CONTEXT_INIT_TIMER(adc_mspm0_data_##index, ctx),                               \
		ADC_CONTEXT_INIT_LOCK(adc_mspm0_data_##index, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_mspm0_data_##index, ctx),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &adc_mspm0_init, NULL, &adc_mspm0_data_##index,               \
			      &adc_mspm0_cfg_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,       \
			      &mspm0_driver_api);						   \

DT_INST_FOREACH_STATUS_OKAY(MSPM0_ADC_INIT)
