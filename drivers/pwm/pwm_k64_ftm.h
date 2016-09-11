/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Header file for the PWM driver for Freescale K64 FlexTimer Module (FTM)
 */

#ifndef __PWM_K64_FTM_H__
#define __PWM_K64_FTM_H__

#include <stdbool.h>

/* Valid prescale values */
#define PWM_K64_FTM_PRESCALE_1       1
#define PWM_K64_FTM_PRESCALE_2       2
#define PWM_K64_FTM_PRESCALE_4       4
#define PWM_K64_FTM_PRESCALE_8       8
#define PWM_K64_FTM_PRESCALE_16     16
#define PWM_K64_FTM_PRESCALE_32     32
#define PWM_K64_FTM_PRESCALE_64     64
#define PWM_K64_FTM_PRESCALE_128   128

/* flags are not used.  This value can be passed into pwm_pin_configure */
#define PWM_K64_FTM_FLAG_NONE        0


/* FTM register bit definitions */


#define PWM_K64_FTM_SC(base)       ((base)+0x00) /* Status And Control */
#define PWM_K64_FTM_CNT(base)      ((base)+0x04) /* Counter */
#define PWM_K64_FTM_MOD(base)      ((base)+0x08) /* Modulo */

#define PWM_K64_FTM_CNSC(base, ch) ((base)+0x0C+(ch)*8) /* Channel Status&Ctrl*/
#define PWM_K64_FTM_CNV(base, ch)  ((base)+0x10+(ch)*8) /* Channel Value */

#define PWM_K64_FTM_CNTIN(base)    ((base)+0x4C) /* Counter Initial Value */
#define PWM_K64_FTM_STATUS(base)   ((base)+0x50) /* Capture And Compare Status*/
#define PWM_K64_FTM_MODE(base)     ((base)+0x54) /* Features Mode Selection */
#define PWM_K64_FTM_SYNC(base)     ((base)+0x58) /* Synchronization */
#define PWM_K64_FTM_OUTINIT(base)  ((base)+0x5C) /* Initial Channels Output */
#define PWM_K64_FTM_OUTMASK(base)  ((base)+0x60) /* Output Mask */
#define PWM_K64_FTM_COMBINE(base)  ((base)+0x64) /* Function For Linked Chans */
#define PWM_K64_FTM_DEADTIME(base) ((base)+0x68) /* Deadtime Insertion Ctrl */
#define PWM_K64_FTM_EXTTRIG(base)  ((base)+0x6C) /* FTM External Trigger */
#define PWM_K64_FTM_POL(base)      ((base)+0x70) /* Channels Polarity */
#define PWM_K64_FTM_FMS(base)      ((base)+0x74) /* Fault Mode Status */
#define PWM_K64_FTM_FILTER(base)   ((base)+0x78) /* Input Capture Filter Ctrl */
#define PWM_K64_FTM_FLTCTRL(base)  ((base)+0x7C) /* Fault Control */
#define PWM_K64_FTM_QDCTRL(base)   ((base)+0x80) /* Quadrature Decoder Ctrl */
#define PWM_K64_FTM_CONF(base)     ((base)+0x84) /* Configuration */
#define PWM_K64_FTM_FLTPOL(base)   ((base)+0x88) /* FTM Fault Input Polarity */
#define PWM_K64_FTM_SYNCONF(base)  ((base)+0x8C) /* Synchronization Config */
#define PWM_K64_FTM_INVCTRL(base)  ((base)+0x90) /* FTM Inverting Control */
#define PWM_K64_FTM_SWOCTRL(base)  ((base)+0x94) /* FTM Software Output Ctrl */
#define PWM_K64_FTM_PWMLOAD(base)  ((base)+0x98) /* FTM PWM Load */

/* PWM_K64_FTM_SC Status And Control */

#define PWM_K64_FTM_SC_CLKS_MASK      0x18
#define PWM_K64_FTM_SC_CLKS_SHIFT     3

#define PWM_K64_FTM_SC_CLKS_DISABLE   0x0
#define PWM_K64_FTM_SC_CLKS_SYSTEM    0x1
#define PWM_K64_FTM_SC_CLKS_FIXED     0x2
#define PWM_K64_FTM_SC_CLKS_EXTERNAL  0x3

#define PWM_K64_FTM_SC_PS_D1   (0x0<<0)
#define PWM_K64_FTM_SC_PS_D2   (0x1<<0)
#define PWM_K64_FTM_SC_PS_D4   (0x2<<0)
#define PWM_K64_FTM_SC_PS_D8   (0x3<<0)
#define PWM_K64_FTM_SC_PS_D16  (0x4<<0)
#define PWM_K64_FTM_SC_PS_D32  (0x5<<0)
#define PWM_K64_FTM_SC_PS_D64  (0x6<<0)
#define PWM_K64_FTM_SC_PS_D128 (0x7<<0)

#define PWM_K64_FTM_SC_PS_MASK (0x7<<0)

/* PWM_K64_FTM_CNSC (FTMx_CnSC) Channel-n Status And Control */
#define PWM_K64_FTM_CNSC_DMA  (0x1<<0)
#define PWM_K64_FTM_CNSC_ELSA (0x1<<2)
#define PWM_K64_FTM_CNSC_ELSB (0x1<<3)
#define PWM_K64_FTM_CNSC_MSA  (0x1<<4)
#define PWM_K64_FTM_CNSC_MSB  (0x1<<5)
#define PWM_K64_FTM_CNSC_CHIE (0x1<<6)
#define PWM_K64_FTM_CNSC_CHF  (0x1<<7)

/* PWM_K64_FTM_MODE  Features Mode Selection */
#define PWM_K64_FTM_MODE_FTMEN        (0x1<<0)
#define PWM_K64_FTM_MODE_INIT         (0x1<<1)
#define PWM_K64_FTM_MODE_WPDIS        (0x1<<2)
#define PWM_K64_FTM_MODE_PWMSYNC      (0x1<<3)
#define PWM_K64_FTM_MODE_CAPTEST      (0x1<<4)

#define PWM_K64_FTM_MODE_FAULTM_DISABLE (0x0<<5)
#define PWM_K64_FTM_MODE_FAULTM_EVEN    (0x1<<5)
#define PWM_K64_FTM_MODE_FAULTM_MANUAL  (0x2<<5)
#define PWM_K64_FTM_MODE_FAULTM_AUTO    (0x3<<5)
#define PWM_K64_FTM_MODE_FAULTM_MASK    (0x3<<5)

#define PWM_K64_FTM_MODE_FAULTIE        (0x1<<7)

/* PWM_K64_FTM_SYNC PWM Synchronization */
#define PWM_K64_FTM_SYNC_CNTMIN         (0x1<<0)
#define PWM_K64_FTM_SYNC_CNTMAX         (0x1<<1)
#define PWM_K64_FTM_SYNC_REINIT         (0x1<<2)
#define PWM_K64_FTM_SYNC_SYNCHOM        (0x1<<3)
#define PWM_K64_FTM_SYNC_TRIG0          (0x1<<4)
#define PWM_K64_FTM_SYNC_TRIG1          (0x1<<5)
#define PWM_K64_FTM_SYNC_TRIG2          (0x1<<6)
#define PWM_K64_FTM_SYNC_SWSYNC         (0x1<<7)

/* PWM_K64_FTM_EXTTRIG FTM External Trigger */
#define PWM_K64_FTM_EXTTRIG_CH2TRIG     (0x1<<0)
#define PWM_K64_FTM_EXTTRIG_CH3TRIG     (0x1<<1)
#define PWM_K64_FTM_EXTTRIG_CH4TRIG     (0x1<<2)
#define PWM_K64_FTM_EXTTRIG_CH5TRIG     (0x1<<3)
#define PWM_K64_FTM_EXTTRIG_CH0TRIG     (0x1<<4)
#define PWM_K64_FTM_EXTTRIG_CH1TRIG     (0x1<<5)
#define PWM_K64_FTM_EXTTRIG_INTTRIGEN   (0x1<<6)
#define PWM_K64_FTM_EXTTRIG_TRIGF       (0x1<<7)

/* PWM_K64_FTM_QDCTRL Quadrature Decoder Ctrl&Status */
#define PWM_K64_FTM_QDCTRL_QUADEN       (0x1<<0)

/* PWM_K64_FTM_SYNCONF Syncronization Configuration */
#define PWM_K64_FTM_SYNCONF_HWTRIGMODE  (0x1<<0)
#define PWM_K64_FTM_SYNCONF_CNTINC      (0x1<<2)
#define PWM_K64_FTM_SYNCONF_INVC        (0x1<<4)
#define PWM_K64_FTM_SYNCONF_SWOC        (0x1<<5)
#define PWM_K64_FTM_SYNCONF_SYNCMODE    (0x1<<7)

/**
 * @brief Initialization function for FlexTimer Module FTM (PWM mode)
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise
 */
extern int pwm_ftm_init(struct device *dev);

/** Configuration data */
struct pwm_ftm_config {

	/* FTM register base address */
	uint32_t ftm_num;

	/* FTM register base address */
	uint32_t reg_base;

	/* FTM prescale (1,2,4,8,16,32,64,128) */
	uint32_t prescale;

	/* FTM clock source  */
	uint32_t clock_source;

	/* If phase is not 0, the odd-numbered channel is not available */
	bool phase_enable0;  /* combine pwm0, pwm1 for phase capability */
	bool phase_enable2;  /* combine pwm2, pwm3 for phase capability */
	bool phase_enable4;  /* combine pwm4, pwm4 for phase capability */
	bool phase_enable6;  /* combine pwm6, pwm5 for phase capability */

	/* FTM period (clock ticks) */
	uint32_t period;

};

/** Runtime driver data */
struct pwm_ftm_drv_data {
	uint32_t phase[4];
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

#endif /* __PWM_K64_FTM_H__ */
