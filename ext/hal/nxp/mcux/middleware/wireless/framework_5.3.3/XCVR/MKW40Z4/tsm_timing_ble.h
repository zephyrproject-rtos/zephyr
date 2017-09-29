/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file tsm_timing_ble.h
* Header file for the BLE TSM timing definitions.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TSM_TIMING_BLE_H
#define TSM_TIMING_BLE_H

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "BLEDefaults.h"

/*! *********************************************************************************
*************************************************************************************
* Macros
*************************************************************************************
********************************************************************************** */

/* *** Common TSM defines *** */
#define TSM_SIG_DIS    (0xFF)  /* Setting start and end time to 0xFF disables a signal */
/*Assertion time setting for signal or group TX sequence.*/
#define TSM_TX_HI_MASK  0xFF
#define TSM_TX_HI_SHIFT 0
/*Deassertion time setting for signal or group TX sequence.*/
#define TSM_TX_LO_MASK  0xFF00
#define TSM_TX_LO_SHIFT 8
/*Assertion time setting for signal or group RX sequence.*/
#define TSM_RX_HI_MASK  0xFF0000
#define TSM_RX_HI_SHIFT 16
/*Deassertion time setting for signal or group TX sequence.*/
#define TSM_RX_LO_MASK  0xFF000000
#define TSM_RX_LO_SHIFT 24
/* Make a register write value */
#define TSM_REG_VALUE(rx_wd,rx_wu,tx_wd,tx_wu) ((((unsigned int)rx_wd<<TSM_RX_LO_SHIFT)&TSM_RX_LO_MASK) | \
                                                (((unsigned int)rx_wu<<TSM_RX_HI_SHIFT)&TSM_RX_HI_MASK) | \
                                                (((unsigned int)tx_wd<<TSM_TX_LO_SHIFT)&TSM_TX_LO_MASK) | \
                                                (((unsigned int)tx_wu<<TSM_TX_HI_SHIFT)&TSM_TX_HI_MASK))

/* Shortcuts for TSM disabled signals */
#define TSM_SIGNAL_TX_RX_DIS            (0xFFFFFFFF)    /* Entire signal is disabled */
#define TSM_SIGNAL_RX_DIS               (0xFFFF0000)    /* RX portion of signal is disabled */
#define TSM_SIGNAL_TX_DIS               (0x0000FFFF)    /* TX portion of signal is disabled */

/* Define some delays that apply to entire TX or RX WU/WD sequences */
#define TX_WU_START_DELAY       (0)     /* Delay for the whole startup sequence */
#define TX_WD_START_DELAY       (0)     /* Delay for the whole shutdown sequence */
#define RX_WU_START_DELAY       (0)     /* Delay for the whole startup sequence */
#define RX_WD_START_DELAY       (0)     /* Delay for the whole shutdown sequence */
#define RX_WD_COMMON_DELAY      (3)     /* Delay for the end of the whole shutdown sequence */

/* Define TX PA Ramp UP/DOWN time */
#define PA_RAMP_2US             (2+1)   /* 2 microseconds ramp plus 1 for settling */

/* Define PA_BIAS_TBL entries  */
#define PA_BIAS_ENTRIES_2US     {1,2,4,6,8,10,13,15}

#define PA_RAMP_TIME            (PA_RAMP_2US)   /* Ramp up/down time in microseconds */
#define TXDIG_RAMP_DOWN_TIME    (PA_RAMP_2US)   /* Ramp up/down time in microseconds */
#define PA_BIAS_ENTRIES         PA_BIAS_ENTRIES_2US

/* Data padding time constant */
#define DATA_PADDING_TIME       (8) /* Data padding is always ON */

/* Define some specifics about regulators,PLL, ADC, how long to settle and whether to sequence startup */
#define REG_WU_TIME             (10)    /* Regulators are completely up at this point */
#define REG_WU_SKEW             (0)     /* Each reg starts up SKEW microsec later than the last */
#define PLL_CTUNE_TIME          (15)    /* CTUNE algorithm time */
#define PLL_HPMCAL1_TIME        (25)    /* HPMCAL1 algorithm time */
#define PLL_HPMCAL2_TIME        (25)    /* HPMCAL2 algorithm time */
#define PLL_SEQ_SETTLE          (1)     /* Settling time between PLL sequence changes. */
#define TX_PLL_LOCK_TIME        (16)    /* Time from completion of CTUNE/HPMCAL process */
#define RX_PLL_LOCK_TIME        (25)    /* Time from completion of CTUNE/HPMCAL process */
#define ADC_PRECHARGE_TIME      (6)     /* Time for ADC precharge enable to be held high */
#define ADC_RST_TIME            (1)     /* Time for ADC reset enable to be held high */
#define AGC_SEQ_SETTLE          (8)     /* Settling time between AGC sequence changes. */
#define AGC_ADAPT_TIME          (10)    /* Time for AGC to adapt in adaptation mode */
#define DCOC_INIT_TIME          (1)     /* Length of DCOC init pulse */
#define DCOC_TIME               (70)    /* Time for DCOC to complete, compare to PLL Lock for RX */
#define RX_INIT_TIME            (1)     /* Length of RX init pulse */
#define RX_INIT_SETTLE_TIME     (1)     /* Setting time after RX init pulse deasserts */

#define DCOC_ADJUST_WORKAROUND  (0)     /* DEPRECATED - SHOULD ALWAYS BE ZERO! */
#define PDET_RESET_WORKAROUND   (0)     /* DEPRECATED - SHOULD ALWAYS BE ZERO! */

/* Define overlaps between different startup groups where some startup can be done
 * in parallel. Overlap between later REG startup and initial PLL startup 
 */
#define REG_PLL_WU_OVERLAP      (5)     /* Start PLL WU this long before regs startup completes. */

/* Prep time for PLL lock start, how much earlier sigma-delta, PHDET, & PLL filter can start */
#define PLL_LOCK_PREP_TIME      (10)    /* Start a set of signals this amount before PLL_LOCK_START_TIME */

/* TX warmup sequence timings */
#define PLL_REG_EN_TX_WU        (TX_WU_START_DELAY)     /* This is the first signal in the sequences */
#define PLL_VCO_REG_EN_TX_WU    (PLL_REG_EN_TX_WU+REG_WU_SKEW) /* Same as PLL_REG_EN_TX_WU or +SKEW */
#define PLL_QGEN_REG_EN_TX_WU   (PLL_VCO_REG_EN_TX_WU+REG_WU_SKEW) /* Same as PLL_VCO_REG_EN_TX_WU or +SKEW */
#define PLL_TCA_TX_REG_EN_TX_WU (PLL_QGEN_REG_EN_TX_WU+REG_WU_SKEW) /* Same as PLL_QGEN_REG_EN_TX_WU or +SKEW */

#define PLL_VCO_AUTOTUNE_TX_WU  (TX_WU_START_DELAY)     /* Enable AUTOTUNE at the start of the WU sequence */

#define PLL_VCO_EN_TX_WU        (TX_WU_START_DELAY+REG_WU_TIME+(4*REG_WU_SKEW)- 7 /*REG_PLL_WU_OVERLAP*/)  /* Account for overlap in REG & PLL startups */
#define PLL_DIG_EN_TX_WU        (PLL_VCO_EN_TX_WU+16)  /* This is the start of PLL calibration sequence */
#define PLL_VCO_BUF_TX_EN_TX_WU (PLL_VCO_EN_TX_WU+2)   /* Turn on buffer 2us after vco_en */
#define PLL_TX_LDV_RIPPLE_MUX_EN_TX_WU (PLL_VCO_BUF_TX_EN_TX_WU) /* Ripple counter on */

#define PLL_LOCK_START_TIME_TX  (PLL_DIG_EN_TX_WU+PLL_CTUNE_TIME+PLL_HPMCAL1_TIME+PLL_HPMCAL2_TIME+(4*PLL_SEQ_SETTLE))
#define PLL_REF_CLK_EN_TX_WU    (PLL_LOCK_START_TIME_TX-PLL_LOCK_PREP_TIME-5)  /* Relative to start of PLL lock */
#define PLL_FILTER_CHARGE_EN_TX_WU (PLL_LOCK_START_TIME_TX-PLL_LOCK_PREP_TIME) /* Relative to start of PLL lock */
#define PLL_PHDET_EN_TX_WU      (PLL_LOCK_START_TIME_TX-PLL_LOCK_PREP_TIME)    /* Relative to start of PLL lock */
#define SIGMA_DELTA_EN_TX_WU    (PLL_LOCK_START_TIME_TX-PLL_LOCK_PREP_TIME)    /* Relative to start of PLL lock */
#define PLL_LDV_EN_TX_WU        (PLL_LOCK_START_TIME_TX)    /* Can start LDV enable right at lock start */
#define PLL_LOCKED_TIME_TX      (PLL_LOCK_START_TIME_TX+TX_PLL_LOCK_TIME)
#define TX_EN_TX_WU             (PLL_LOCKED_TIME_TX) 
#define PLL_PA_BUF_EN_TX_WU     (TX_EN_TX_WU)   /* 1 usec before TX_EN */
#define PLL_CYCLE_SLIP_LD_EN_TX_WU (PLL_LOCKED_TIME_TX+3)
#define TX_DIG_EN_TX_WU         (PLL_CYCLE_SLIP_LD_EN_TX_WU-DATA_PADDING_TIME)  /* Affected by padding (must start earlier for padding) */
#define FREQ_TARG_LD_EN_TX_WU   (PLL_CYCLE_SLIP_LD_EN_TX_WU+2)      /* Asserted concurrent with end of warmup */
#define END_OF_TX_WARMUP        (PLL_CYCLE_SLIP_LD_EN_TX_WU+6)      /* End of warmup is driven by 6us delay from end of ramp to allow for settling */

/* Unused signals */
#define ADC_REG_EN_TX_WU (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_CLK_EN_TX_WU (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define PLL_VCO_BUF_RX_EN_TX_WU (TSM_SIG_DIS)        /* Not used in TX scenarios */
#define PLL_RX_LDV_RIPPLE_MUX_EN_TX_WU (TSM_SIG_DIS) /* Not used in TX scenarios */
#define QGEN25_EN_TX_WU (TSM_SIG_DIS)                /* Not used in TX scenarios */
#define ADC_EN_TX_WU (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define ADC_I_Q_EN_TX_WU (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_DAC_EN_TX_WU (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_RST_EN_TX_WU (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define BBF_EN_TX_WU (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define TCA_EN_TX_WU (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define RX_DIG_EN_TX_WU (TSM_SIG_DIS)                /* Not used in TX scenarios */
#define RX_INIT_TX_WU (TSM_SIG_DIS)                  /* Not used in TX scenarios */
#define ZBDEM_RX_EN_TX_WU (TSM_SIG_DIS)              /* Not used in TX scenarios */
#define DCOC_EN_TX_WU (TSM_SIG_DIS)                  /* Not used in TX scenarios */
#define DCOC_INIT_EN_TX_WU (TSM_SIG_DIS)             /* Not used in TX scenarios */
#define SAR_ADC_TRIG_EN_TX_WU (TSM_SIG_DIS)          /* Not used in TX scenarios */
#define TSM_SPARE0_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE1_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE2_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE03_EN_TX_WU (TSM_SIG_DIS)           /* Not used in TX scenarios */
#define GPIO0_TRIG_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO1_TRIG_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO2_TRIG_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO3_TRIG_EN_TX_WU (TSM_SIG_DIS)            /* Not used in TX scenarios */

#define END_OF_TX_WU_BLE        (END_OF_TX_WARMUP)   /* This is the last signal */

/* TX warmdown sequence timings */
#define PLL_REG_EN_TX_WD        (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_VCO_REG_EN_TX_WD    (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_QGEN_REG_EN_TX_WD   (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_TCA_TX_REG_EN_TX_WD (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */

#define PLL_VCO_AUTOTUNE_TX_WD  (PLL_LOCK_START_TIME_TX)                /* End of PLL Lock sequence */
#define PLL_VCO_EN_TX_WD        (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_DIG_EN_TX_WD        (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_VCO_BUF_TX_EN_TX_WD (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_TX_LDV_RIPPLE_MUX_EN_TX_WD (END_OF_TX_WU_BLE+PA_RAMP_TIME)  /* End of warmdown */
#define PLL_REF_CLK_EN_TX_WD    (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_FILTER_CHARGE_EN_TX_WD (PLL_LOCK_START_TIME_TX)             /* End of PLL cal. sequence */
#define PLL_PHDET_EN_TX_WD      (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define SIGMA_DELTA_EN_TX_WD    (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_LDV_EN_TX_WD        (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define PLL_CYCLE_SLIP_LD_EN_TX_WD (END_OF_TX_WU_BLE+1)                 /* End of warmdown, before PA ramps down */
#define FREQ_TARG_LD_EN_TX_WD   (END_OF_TX_WU_BLE+1)                    /* End of warmdown, before PA ramps down */
#define TX_EN_TX_WD             (END_OF_TX_WU_BLE+TXDIG_RAMP_DOWN_TIME) /* TX_DIG_EN has a special case for noramp warmdown! */
#define PLL_PA_BUF_EN_TX_WD     (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */
#define TX_DIG_EN_TX_WD         (END_OF_TX_WU_BLE+PA_RAMP_TIME)         /* End of warmdown */

/* Unused signals */
#define ADC_REG_EN_TX_WD (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_CLK_EN_TX_WD (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define PLL_VCO_BUF_RX_EN_TX_WD (TSM_SIG_DIS)        /* Not used in TX scenarios */
#define PLL_RX_LDV_RIPPLE_MUX_EN_TX_WD (TSM_SIG_DIS) /* Not used in TX scenarios */
#define QGEN25_EN_TX_WD (TSM_SIG_DIS)                /* Not used in TX scenarios */
#define ADC_EN_TX_WD (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define ADC_I_Q_EN_TX_WD (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_DAC_EN_TX_WD (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define ADC_RST_EN_TX_WD (TSM_SIG_DIS)               /* Not used in TX scenarios */
#define BBF_EN_TX_WD (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define TCA_EN_TX_WD (TSM_SIG_DIS)                   /* Not used in TX scenarios */
#define RX_DIG_EN_TX_WD (TSM_SIG_DIS)                /* Not used in TX scenarios */
#define RX_INIT_TX_WD (TSM_SIG_DIS)                  /* Not used in TX scenarios */
#define ZBDEM_RX_EN_TX_WD (TSM_SIG_DIS)              /* Not used in TX scenarios */
#define DCOC_EN_TX_WD (TSM_SIG_DIS)                  /* Not used in TX scenarios */
#define DCOC_INIT_EN_TX_WD (TSM_SIG_DIS)             /* Not used in TX scenarios */
#define SAR_ADC_TRIG_EN_TX_WD (TSM_SIG_DIS)          /* Not used in TX scenarios */
#define TSM_SPARE0_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE1_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE2_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define TSM_SPARE03_EN_TX_WD (TSM_SIG_DIS)           /* Not used in TX scenarios */
#define GPIO0_TRIG_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO1_TRIG_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO2_TRIG_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */
#define GPIO3_TRIG_EN_TX_WD (TSM_SIG_DIS)            /* Not used in TX scenarios */

#define END_OF_TX_WD_BLE        (END_OF_TX_WU_BLE+PA_RAMP_TIME) /* TX warmdown lasts only as long a PA rampdown */

/* RX warmup sequence timings */
#define PLL_REG_EN_RX_WU        (RX_WU_START_DELAY)     /* This is the first signal in the sequences */
#define PLL_VCO_REG_EN_RX_WU    (PLL_REG_EN_RX_WU+REG_WU_SKEW) /* Same as PLL_REG_EN_RX_WU or +SKEW */
#define PLL_QGEN_REG_EN_RX_WU   (PLL_VCO_REG_EN_RX_WU+REG_WU_SKEW) /* Same as PLL_VCO_REG_EN_RX_WU or +SKEW */
#define PLL_TCA_TX_REG_EN_RX_WU (PLL_QGEN_REG_EN_RX_WU+REG_WU_SKEW) /* Same as PLL_QGEN_REG_EN_RX_WU or +SKEW */
#define ADC_REG_EN_RX_WU        (PLL_TCA_TX_REG_EN_RX_WU+REG_WU_SKEW) /* Same as PLL_TCA_TX_REG_EN_RX_WU or +SKEW */

/* PLL startup */
#define PLL_VCO_AUTOTUNE_RX_WU  (RX_WU_START_DELAY)     /* Enable AUTOTUNE at the start of the WU sequence */
#define PLL_VCO_EN_RX_WU        (RX_WU_START_DELAY+REG_WU_TIME+(5*REG_WU_SKEW)-REG_PLL_WU_OVERLAP-2)
#define QGEN25_EN_RX_WU         (PLL_VCO_EN_RX_WU)      /* Same as PLL_VCO_EN_RX_WU */
#define PLL_VCO_BUF_RX_EN_RX_WU (RX_WU_START_DELAY+REG_WU_TIME+(5*REG_WU_SKEW)-REG_PLL_WU_OVERLAP)
#define PLL_RX_LDV_RIPPLE_MUX_EN_RX_WU (RX_WU_START_DELAY+REG_WU_TIME+(5*REG_WU_SKEW)-REG_PLL_WU_OVERLAP)
#define PLL_DIG_EN_RX_WU        (PLL_RX_LDV_RIPPLE_MUX_EN_RX_WU+14)

/* RX PLL locking definitions */
#define PLL_LOCK_START_TIME_RX  (PLL_DIG_EN_RX_WU+PLL_CTUNE_TIME+(2*PLL_SEQ_SETTLE)) /* When locking starts */
#define PLL_LOCKED_TIME_RX      (PLL_LOCK_START_TIME_RX+RX_PLL_LOCK_TIME)            /* When locking is complete */
#define PLL_FILTER_CHARGE_EN_RX_WU  (PLL_LOCK_START_TIME_RX-10)   /* Starts 10us before locking starts */
#define PLL_PHDET_EN_RX_WU      (PLL_FILTER_CHARGE_EN_RX_WU)      /* Same as PLL_FILTER_CHARGE_EN_RX_WU */
#define PLL_LDV_EN_RX_WU        (PLL_LOCK_START_TIME_RX)          /* Not used in RX scenarios */
#define SIGMA_DELTA_EN_RX_WU    (PLL_FILTER_CHARGE_EN_RX_WU)      /* Same as PLL_FILTER_CHARGE_EN_RX_WU */
#define PLL_REF_CLK_EN_RX_WU    (PLL_FILTER_CHARGE_EN_RX_WU-5)    /* Starts 5us before PLL_FILTER_CHARGE_EN_RX_WU */
#define FREQ_TARG_LD_EN_RX_WU   (PLL_LOCKED_TIME_RX)              /* Starts when PLL locking is complete */
#define PLL_CYCLE_SLIP_LD_EN_RX_WU  (PLL_LOCKED_TIME_RX)          /* Starts when PLL locking is complete */
#define ADC_EN_RX_WU            (PLL_LOCK_START_TIME_RX-1)        /* 1us before start of PLL locking */
#define ADC_DAC_EN_RX_WU        (ADC_EN_RX_WU)                    /* 1us before start of PLL locking */
#define ADC_CLK_EN_RX_WU        (ADC_EN_RX_WU)                    /* 1us before start of PLL locking */
#define ADC_I_Q_EN_RX_WU        (ADC_EN_RX_WU)                    /* 1us before start of PLL locking */
#define ADC_RST_EN_RX_WU        (ADC_EN_RX_WU+1)                  /* 1us after ADC enabled */
#define BBF_EN_RX_WU            (PLL_LOCK_START_TIME_RX-2)        /* 2us before start of PLL locking */
#define TCA_EN_RX_WU            (BBF_EN_RX_WU)                    /* Same as BBF_EN_RX_WU */

/* DCOC startup */
#define DCOC_EN_RX_WU           (ADC_RST_EN_RX_WU+ADC_RST_TIME+AGC_SEQ_SETTLE) /* Start DCOC after ADC reset */
#define DCOC_INIT_EN_RX_WU      (DCOC_EN_RX_WU) /* Same as DCOC_EN_RX_WU */

/* Don't enable RX dig until both DCOC completes and PLL lock time is complete */
#define DCOC_COMPLETED_TIME_RX  (DCOC_EN_RX_WU+DCOC_TIME)
#define RX_DIG_EN_RX_WU         ((DCOC_COMPLETED_TIME_RX>PLL_LOCKED_TIME_RX) ? DCOC_COMPLETED_TIME_RX : PLL_LOCKED_TIME_RX) 
#define RX_INIT_RX_WU           (RX_DIG_EN_RX_WU)
#define END_OF_RX_WU_BLE        (RX_DIG_EN_RX_WU+RX_INIT_TIME+RX_INIT_SETTLE_TIME) /* End of RX WU */

/* Unused signals */
#define PLL_VCO_BUF_TX_EN_RX_WU  (TSM_SIG_DIS)  /* Not used in RX scenarios */
#define PLL_PA_BUF_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define PLL_TX_LDV_RIPPLE_MUX_EN_RX_WU  (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TX_EN_RX_WU  (TSM_SIG_DIS)              /* Not used in RX scenarios */
#define TX_DIG_EN_RX_WU  (TSM_SIG_DIS)          /* Not used in RX scenarios */
#define ZBDEM_RX_EN_RX_WU  (TSM_SIG_DIS)        /* Not used in RX scenarios */
#define SAR_ADC_TRIG_EN_RX_WU  (TSM_SIG_DIS)    /* Not used in RX scenarios */
#define TSM_SPARE0_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define TSM_SPARE1_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define TSM_SPARE2_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define TSM_SPARE03_EN_RX_WU  (TSM_SIG_DIS)     /* Not used in RX scenarios */
#define GPIO0_TRIG_EN_RX_WU     (TSM_SIG_DIS)   /* Not used in RX scenarios */
#define GPIO1_TRIG_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define GPIO2_TRIG_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */
#define GPIO3_TRIG_EN_RX_WU  (TSM_SIG_DIS)      /* Not used in RX scenarios */

/* RX warmdown sequence timings */
#define PLL_REG_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_VCO_REG_EN_RX_WD    (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_QGEN_REG_EN_RX_WD   (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_TCA_TX_REG_EN_RX_WD (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_REG_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_VCO_AUTOTUNE_RX_WD  (PLL_LOCK_START_TIME_RX) /* Turn off AUTOTUNE at the start of the PLL locking sequence */
#define PLL_VCO_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define QGEN25_EN_RX_WD         (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_VCO_BUF_RX_EN_RX_WD (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_DIG_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_RX_LDV_RIPPLE_MUX_EN_RX_WD (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_FILTER_CHARGE_EN_RX_WD  (PLL_FILTER_CHARGE_EN_RX_WU+10) /* 10us pulse */
#define PLL_PHDET_EN_RX_WD      (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_LDV_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define SIGMA_DELTA_EN_RX_WD    (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_REF_CLK_EN_RX_WD    (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define FREQ_TARG_LD_EN_RX_WD   (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define PLL_CYCLE_SLIP_LD_EN_RX_WD  (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_EN_RX_WD            (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_DAC_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_CLK_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_I_Q_EN_RX_WD        (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define ADC_RST_EN_RX_WD        (ADC_RST_EN_RX_WU+ADC_RST_TIME) /* Pulse length = ADC_RST_TIME */
#define BBF_EN_RX_WD            (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define TCA_EN_RX_WD            (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define DCOC_EN_RX_WD           (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define DCOC_INIT_EN_RX_WD      (DCOC_INIT_EN_RX_WU+DCOC_INIT_TIME) /* Pulse length = DCOC_INIT_TIME */
#define RX_DIG_EN_RX_WD         (END_OF_RX_WU_BLE+1)     /* Common end of warmdown */
#define RX_INIT_RX_WD           (RX_INIT_RX_WU+RX_INIT_TIME) /* Pulse length = RX_INIT_TIME */

#if (DCOC_ADJUST_WORKAROUND)
#define GPIO0_TRIG_EN_RX_WD     (DCOC_COMPLETED_TIME_RX) /* Release this signal at end of allocated time */
#else
#define GPIO0_TRIG_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#endif

/* Unused signals */
#define PLL_VCO_BUF_TX_EN_RX_WD (TSM_SIG_DIS) /* Not used in RX scenarios */
#define PLL_PA_BUF_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define PLL_TX_LDV_RIPPLE_MUX_EN_RX_WD  (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TX_EN_RX_WD             (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TX_DIG_EN_RX_WD         (TSM_SIG_DIS) /* Not used in RX scenarios */
#define ZBDEM_RX_EN_RX_WD       (TSM_SIG_DIS) /* Not used in RX scenarios */
#define SAR_ADC_TRIG_EN_RX_WD   (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TSM_SPARE0_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TSM_SPARE1_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TSM_SPARE2_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define TSM_SPARE03_EN_RX_WD    (TSM_SIG_DIS) /* Not used in RX scenarios */
#define GPIO1_TRIG_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define GPIO2_TRIG_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */
#define GPIO3_TRIG_EN_RX_WD     (TSM_SIG_DIS) /* Not used in RX scenarios */

#define END_OF_RX_WD_BLE        (END_OF_RX_WU_BLE+1) /* RX warmdown has no delay */

#define END_OF_SEQ_VALUE        ( (END_OF_RX_WD_BLE<<END_OF_SEQ_END_OF_RX_WD_SHIFT) | \
                                (END_OF_RX_WU_BLE<<END_OF_SEQ_END_OF_RX_WU_SHIFT) | \
                                (END_OF_TX_WD_BLE<<END_OF_SEQ_END_OF_TX_WD_SHIFT) | \
                                (END_OF_TX_WU_BLE<<END_OF_SEQ_END_OF_TX_WU_SHIFT))

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
#ifdef __cplusplus
extern "C"
{
#endif

/* No functions defined, only timing data */

#ifdef __cplusplus
}
#endif


#endif /* TSM_TIMING_BLE_H */
