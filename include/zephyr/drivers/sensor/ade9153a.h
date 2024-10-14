/*
 * Copyright (c) 2024 Plentify (Pty) Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_SENSOR_ADE9153A_H_
#define _ZEPHYR_DRIVERS_SENSOR_ADE9153A_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#define MASK_ADE9153A                  0xFFFF
/* Phase A current gain adjust. */
#define ADE9153A_REG_AIGAIN            0x0000
/* Phase A phase correction factor. */
#define ADE9153A_REG_APHASECAL         0x0001
/* Phase A voltage gain adjust. */
#define ADE9153A_REG_AVGAIN            0x0002
/* Phase A current rms offset for filter-based AIRMS calculation. */
#define ADE9153A_REG_AIRMS_OS          0x0003
/* Phase A voltage rms offset for filter-based AVRMS calculation. */
#define ADE9153A_REG_AVRMS_OS          0x0004
/* Phase A power gain adjust for AWATT, AVA, and AFVAR calculations. */
#define ADE9153A_REG_APGAIN            0x0005
/* Phase A total active power offset correction for AWATT calculation. */
#define ADE9153A_REG_AWATT_OS          0x0006
/* Phase A fundamental reactive power offset correction for AFVAR calculation. */
#define ADE9153A_REG_AFVAR_OS          0x0007
/* Phase A voltage rms offset for fast rms, AVRMS_OC calculation. */
#define ADE9153A_REG_AVRMS_OC_OS       0x0008
/* Phase A current rms offset for fast rms, AIRMS_OC calculation. */
#define ADE9153A_REG_AIRMS_OC_OS       0x0009
/* Phase B current gain adjust. */
#define ADE9153A_REG_BIGAIN            0x0010
/* Phase B current rms offset for filter-based BIRMS calculation. */
#define ADE9153A_REG_BIRMS_OS          0x0013
/* Phase B current rms offset for fast rms, BIRMS_OC calculation. */
#define ADE9153A_REG_BIRMS_OC_OS       0x0019
/* DSP configuration register. */
#define ADE9153A_REG_CONFIG0           0x0020
/* Nominal phase voltage rms used in the calculation of apparent power, AVA, when the VNOMA_EN */
/* bit is set in the CONFIG0 register. */
#define ADE9153A_REG_VNOM              0x0021
/* Value used in the digital integrator algorithm. If the integrator is turned on, with INTEN_BI
 * equal to 1 in the CONFIG0 register, it is recommended to leave this register at the default
 * value.
 */
#define ADE9153A_REG_DICOEFF           0x0022
/* PGA gain for Current Channel B ADC. */
#define ADE9153A_REG_BI_PGAGAIN        0x0023
/* MSure autocalibration configuration register. */
#define ADE9153A_REG_MS_ACAL_CFG       0x0030
/* Phase delay of the CT used on Current Channel B. This register is in 5.27 format and expressed in
 * degrees.
 */
#define ADE9153A_REG_CT_PHASE_DELAY    0x0049
/* Corner frequency of the CT. This value is calculated from the CT_PHASE_DELAY value. */
#define ADE9153A_REG_CT_CORNER         0x004A
/* This register holds the resistance value, in Ω, of the small resistor in the resistor divider.*/
#define ADE9153A_REG_VDIV_RSMALL       0x004C
/* Instantaneous Current Channel A waveform processed by the DSP, at 4kSPS. */
#define ADE9153A_REG_AI_WAV            0x0200
/* Instantaneous Voltage Channel waveform processed by the DSP, at 4kSPS. */
#define ADE9153A_REG_AV_WAV            0x0201
/* Phase A filter-based current rms value updated at 4kSPS. */
#define ADE9153A_REG_AIRMS             0x0202
/* Phase A filter-based voltage rms value updated at 4kSPS. */
#define ADE9153A_REG_AVRMS             0x0203
/* Phase A low-pass filtered total active power updated at 4kSPS. */
#define ADE9153A_REG_AWATT             0x0204
/* Phase A total apparent power updated at 4kSPS. */
#define ADE9153A_REG_AVA               0x0206
/* Phase A fundamental reactive power updated at 4kSPS. */
#define ADE9153A_REG_AFVAR             0x0207
/* Phase A power factor updated at 1.024 sec. */
#define ADE9153A_REG_APF               0x0208
/* Phase A current fast rms calculation; one cycle rms updated every half cycle. */
#define ADE9153A_REG_AIRMS_OC          0x0209
/* Phase A voltage fast rms calculation; one cycle rms updated every half cycle. */
#define ADE9153A_REG_AVRMS_OC          0x020A
/* Instantaneous Phase B Current Channel waveform processed by the DSP at 4kSPS. */
#define ADE9153A_REG_BI_WAV            0x0210
/* Phase B filter-based current rms value updated at 4kSPS. */
#define ADE9153A_REG_BIRMS             0x0212
/* Phase B Current fast rms calculation; one cycle rms updated every half cycle. */
#define ADE9153A_REG_BIRMS_OC          0x0219
/* Current Channel A mSure CC estimation from autocalibration. */
#define ADE9153A_REG_MS_ACAL_AICC      0x0220
/* Current Channel A mSure certainty of autocalibration. */
#define ADE9153A_REG_MS_ACAL_AICERT    0x0221
/* Current Channel B mSure CC estimation from autocalibration. */
#define ADE9153A_REG_MS_ACAL_BICC      0x0222
/* Current Channel B mSure certainty of autocalibration. */
#define ADE9153A_REG_MS_ACAL_BICERT    0x0223
/* Voltage Channel mSure CC estimation from autocalibration. */
#define ADE9153A_REG_MS_ACAL_AVCC      0x0224
/* Voltage Channel mSure certainty of autocalibration. */
#define ADE9153A_REG_MS_ACAL_AVCERT    0x0225
/* The MS_STATUS_CURRENT register contains bits that reflect the present state of the mSure system.
 */
#define ADE9153A_REG_MS_STATUS_CURRENT 0x0240
/* This register indicates the version of the ADE9153A DSP after the user writes RUN=1 to start
 * measurements.
 */
#define ADE9153A_REG_VERSION_DSP       0x0241
/* This register indicates the version of the product being used. */
#define ADE9153A_REG_VERSION_PRODUCT   0x0242
/* Phase A accumulated total active power; updated after PWR_TIME 4kSPS samples. */
#define ADE9153A_REG_AWATT_ACC         0x039D
/* Phase A accumulated total active energy, least significant bits (LSBs). Updated according to the
 * settings in the EP_CFG and EGY_TIME registers.
 */
#define ADE9153A_REG_AWATTHR_LO        0x039E
/* Phase A accumulated total active energy, most significant bits (MSBs). Updated according to the
 * settings in the EP_CFG and EGY_TIME registers.
 */
#define ADE9153A_REG_AWATTHR_HI        0x039F
/* Phase A accumulated total apparent power; updated after PWR_TIME 4kSPS samples. */
#define ADE9153A_REG_AVA_ACC           0x03B1
/* Phase A accumulated total apparent energy, LSBs. Updated according to the settings in the EP_CFG
 * and EGY_TIME registers.
 */
#define ADE9153A_REG_AVAHR_LO          0x03B2
/* Phase A accumulated total apparent energy, MSBs. Updated according to the settings in the EP_CFG
 * and EGY_TIME registers.
 */
#define ADE9153A_REG_AVAHR_HI          0x03B3
/* Phase A accumulated fundamental reactive power; updated after PWR_TIME 4kSPS samples. */
#define ADE9153A_REG_AFVAR_ACC         0x03BB
/* Phase A accumulated fundamental reactive energy, LSBs. Updated according to the settings in the
 * EP_CFG and EGY_TIME registers.
 */
#define ADE9153A_REG_AFVARHR_LO        0x03BC
/* Phase A accumulated fundamental reactive energy, MSBs. Updated according to the settings in the
 * EP_CFG and EGY_TIME registers.
 */
#define ADE9153A_REG_AFVARHR_HI        0x03BD
/* Accumulated positive total active power from the AWATT register; updated after PWR_TIME 4 kSPS
 * samples.
 */
#define ADE9153A_REG_PWATT_ACC         0x03EB
/* Accumulated negative total active power from the AWATT register; updated after PWR_TIME 4 kSPS
 * samples.
 */
#define ADE9153A_REG_NWATT_ACC         0x03EF
/* Accumulated positive fundamental reactive power from the AFVAR register; updated after PWR_TIME 4
 * kSPS samples.
 */
#define ADE9153A_REG_PFVAR_ACC         0x03F3
/* Accumulated negative fundamental reactive power from the AFVAR register, updated after PWR_TIME 4
 * kSPS samples.
 */
#define ADE9153A_REG_NFVAR_ACC         0x03F7
/* Current peak register. */
#define ADE9153A_REG_IPEAK             0x0400
/* Voltage peak register. */
#define ADE9153A_REG_VPEAK             0x0401
/* Tier 1 interrupt status register. */
#define ADE9153A_REG_STATUS            0x0402
/* Tier 1 interrupt enable register. */
#define ADE9153A_REG_MASK              0x0405
/* Overcurrent RMS_OC detection threshold level. */
#define ADE9153A_REG_OI_LVL            0x0409
/* Phase A overcurrent RMS_OC value. If overcurrent detection on this channel is enabled with OIA_EN
 * in the CONFIG3 register and AIRMS_OC is greater than the OILVL threshold, this value is updated.
 */
#define ADE9153A_REG_OIA               0x040A
/* Phase B overcurrent RMS_OC value. See the OIA description. */
#define ADE9153A_REG_OIB               0x040B
/* User configured line period value used for RMS_OC when the UPERIOD_SEL bit in the CONFIG2
 * register is set.
 */
#define ADE9153A_REG_USER_PERIOD       0x040E
/* Register used in the algorithm that computes the fundamental reactive power. */
#define ADE9153A_REG_VLEVEL            0x040F
/* Voltage RMS_OC dip detection threshold level. */
#define ADE9153A_REG_DIP_LVL           0x0410
/* Phase A voltage RMS_OC value during a dip condition. */
#define ADE9153A_REG_DIPA              0x0411
/* Voltage RMS_OC swell detection threshold level. */
#define ADE9153A_REG_SWELL_LVL         0x0414
/* Phase A voltage RMS_OC value during a swell condition. */
#define ADE9153A_REG_SWELLA            0x0415
/* Line period on the Phase A voltage. */
#define ADE9153A_REG_APERIOD           0x0418
/* No load threshold in the total active power datapath. */
#define ADE9153A_REG_ACT_NL_LVL        0x041C
/* No load threshold in the fundamental reactive power datapath. */
#define ADE9153A_REG_REACT_NL_LVL      0x041D
/* No load threshold in the total apparent power datapath. */
#define ADE9153A_REG_APP_NL_LVL        0x041E
/* Phase no load register. */
#define ADE9153A_REG_PHNOLOAD          0x041F
/* Sets the maximum output rate from the digital to frequency converter of the total active power
 * for the CF calibration pulse output. It is recommended to leave this at WTHR = 0x00100000.
 */
#define ADE9153A_REG_WTHR              0x0420
/* See WTHR. It is recommended to leave this value at VARTHR = 0x00100000. */
#define ADE9153A_REG_VARTHR            0x0421
/* See WTHR. It is recommended to leave this value at VATHR = 0x00100000. */
#define ADE9153A_REG_VATHR             0x0422
/* This register holds the data read or written during the last 32-bit transaction on the SPI port.
 */
#define ADE9153A_REG_LAST_DATA_32      0x0423
/* CF calibration pulse width configuration register. */
#define ADE9153A_REG_CF_LCFG           0x0425
/* Temperature sensor gain and offset, calculated during the manufacturing process. */
#define ADE9153A_REG_TEMP_TRIM         0x0471
/* Chip identification, 32 MSBs. */
#define ADE9153A_REG_CHIP_ID_HI        0x0472
/* Chip identification, 32 LSBs. */
#define ADE9153A_REG_CHIP_ID_LO        0x0473

/* 16-bit below */
/* Write this register to 1 to start the measurements. */
#define ADE9153A_REG_RUN              0x0480
/* Configuration Register 1. */
#define ADE9153A_REG_CONFIG1          0x0481
/* Time between positive to negative zero crossings on Phase A voltage and current. */
#define ADE9153A_REG_ANGL_AV_AI       0x0485
/* Time between positive to negative zero crossings on Phase A and Phase B currents. */
#define ADE9153A_REG_ANGL_AI_BI       0x0488
/* Voltage RMS_OC dip detection cycle configuration. */
#define ADE9153A_REG_DIP_CYC          0x048B
/* Voltage RMS_OC swell detection cycle configuration. */
#define ADE9153A_REG_SWELL_CYC        0x048C
/* CFx configuration register. */
#define ADE9153A_REG_CFMODE           0x0490
/* Computation mode register. Set this register to 0x0005. */
#define ADE9153A_REG_COMPMODE         0x0491
/* Accumulation mode register. */
#define ADE9153A_REG_ACCMODE          0x0492
/* Configuration Register 3 for configuration of power quality settings. */
#define ADE9153A_REG_CONFIG3          0x0493
/* CF1 denominator register. */
#define ADE9153A_REG_CF1DEN           0x0494
/* CF2 denominator register. */
#define ADE9153A_REG_CF2DEN           0x0495
/* Zero-crossing timeout configuration register. */
#define ADE9153A_REG_ZXTOUT           0x0498
/* Voltage channel zero-crossing threshold register. */
#define ADE9153A_REG_ZXTHRSH          0x0499
/* Zero-crossing detection configuration register. */
#define ADE9153A_REG_ZX_CFG           0x049A
/* Power sign register. */
#define ADE9153A_REG_PHSIGN           0x049D
/* This register holds the CRC of the configuration registers. */
#define ADE9153A_REG_CRC_RSLT         0x04A8
/* The register holds the 16-bit CRC of the data sent out on the MOSI/RX pin during the last SPI
 * register read.
 */
#define ADE9153A_REG_CRC_SPI          0x04A9
/* This register holds the data read or written during the last 16-bit transaction on the SPI port.
 * When using UART, this register holds the lower 16 bits of the last data read or write.
 */
#define ADE9153A_REG_LAST_DATA_16     0x04AC
/* This register holds the address and the read/write operation request (CMD_HDR) for the last
 * transaction on the SPI port.
 */
#define ADE9153A_REG_LAST_CMD         0x04AE
/* Configuration Register 2. This register controls the high-pass filter (HPF) corner and the user
 * period selection.
 */
#define ADE9153A_REG_CONFIG2          0x04AF
/* Energy and power accumulation configuration. */
#define ADE9153A_REG_EP_CFG           0x04B0
/* Power update time configuration. */
#define ADE9153A_REG_PWR_TIME         0x04B1
/* Energy accumulation update time configuration. */
#define ADE9153A_REG_EGY_TIME         0x04B2
/* This register forces an update of the CRC of configuration registers. */
#define ADE9153A_REG_CRC_FORCE        0x04B4
/* Temperature sensor configuration register. */
#define ADE9153A_REG_TEMP_CFG         0x04B6
/* Temperature measurement result. */
#define ADE9153A_REG_TEMP_RSLT        0x04B7U
/* This register configures the PGA gain for Current Channel A. */
#define ADE9153A_REG_AI_PGAGAIN       0x04B9
/* This register enables the configuration lock feature. */
#define ADE9153A_REG_WR_LOCK          0x04BF
/* Tier 2 status register for the autocalibration and monitoring mSure system related interrupts.
 * Any bit set in this register causes the corresponding bit in the status register to be set. This
 * register is cleared on a read and all bits are reset. If a new status bit arrives on the same
 * clock on which the read occurs, the new status bit remains set; in this way, no status bit is
 * missed.
 */
#define ADE9153A_REG_MS_STATUS_IRQ    0x04C0
/* Tier 2 status register for power quality event related interrupts. See the MS_STATUS_IRQ
 * description.
 */
#define ADE9153A_REG_EVENT_STATUS     0x04C1
/* Tier 2 status register for chip error related interrupts. See the MS_STATUS_IRQ description. */
#define ADE9153A_REG_CHIP_STATUS      0x04C2
/* This register switches the UART Baud rate between 4800 and 115,200 Baud. Writing a value of
 * 0x0052 sets the Baud rate to 115,200 Baud; any other value maintains a Baud rate of 4800.
 */
#define ADE9153A_REG_UART_BAUD_SWITCH 0x04DC
/* Version of the ADE9153 IC. */
#define ADE9153A_REG_VERSION          0x04FE
/* SPI burst read accessible registers organized functionally. See AI_WAV. */
#define ADE9153A_REG_AI_WAV_1         0x0600
/* SPI burst read accessible registers organized functionally. See AV_WAV. */
#define ADE9153A_REG_AV_WAV_1         0x0601
/* SPI burst read accessible registers organized functionally. See BI_WAV. */
#define ADE9153A_REG_BI_WAV_1         0x0602
/* SPI burst read accessible registers organized functionally. See AIRMS. */
#define ADE9153A_REG_AIRMS_1          0x0604
/* SPI burst read accessible registers organized functionally. See BIRMS. */
#define ADE9153A_REG_BIRMS_1          0x0605
/* SPI burst read accessible registers organized functionally. See AVRMS. */
#define ADE9153A_REG_AVRMS_1          0x0606
/* SPI burst read accessible registers organized functionally. See AWATT. */
#define ADE9153A_REG_AWATT_1          0x0608
/* SPI burst read accessible registers organized functionally. See AFVAR. */
#define ADE9153A_REG_AFVAR_1          0x060A
/* SPI burst read accessible registers organized functionally. See AVA. */
#define ADE9153A_REG_AVA_1            0x060C
/* SPI burst read accessible registers organized functionally. See APF. */
#define ADE9153A_REG_APF_1            0x060E
/* SPI burst read accessible registers organized by phase. See AI_WAV. */
#define ADE9153A_REG_AI_WAV_2         0x0610
/* SPI burst read accessible registers organized by phase. See AV_WAV. */
#define ADE9153A_REG_AV_WAV_2         0x0611
/* SPI burst read accessible registers organized by phase. See AIRMS. */
#define ADE9153A_REG_AIRMS_2          0x0612
/* SPI burst read accessible registers organized by phase. See AVRMS. */
#define ADE9153A_REG_AVRMS_2          0x0613
/* SPI burst read accessible registers organized by phase. See AWATT. */
#define ADE9153A_REG_AWATT_2          0x0614
/* SPI burst read accessible registers organized by phase. See AVA. */
#define ADE9153A_REG_AVA_2            0x0615
/* SPI burst read accessible registers organized by phase. See AFVAR. */
#define ADE9153A_REG_AFVAR_2          0x0616
/* SPI burst read accessible registers organized by phase. See APF. */
#define ADE9153A_REG_APF_2            0x0617
/* SPI burst read accessible registers organized by phase. See BI_WAV. */
#define ADE9153A_REG_BI_WAV_2         0x0618
/* SPI burst read accessible registers organized by phase. See BIRMS. */
#define ADE9153A_REG_BIRMS_2          0x061A

union ade9153a_status_reg {
	struct {                        /* BITS */
		uint32_t REVAPA: 1;     /* [00] */
		uint32_t RESERVED_0: 1; /* [01] */
		uint32_t REVRPA: 1;     /* [02] */
		uint32_t RESERVED_1: 1; /* [03] */
		uint32_t REVPCF1: 1;    /* [04] */
		uint32_t REVPCF2: 1;    /* [05] */
		uint32_t CF1: 1;        /* [06] */
		uint32_t CF2: 1;        /* [07] */
		uint32_t EGYRDY: 1;     /* [08] */
		uint32_t DREADY: 1;     /* [09] */
		uint32_t PWRRDY: 1;     /* [10] */
		uint32_t RMS_OC_RDY: 1; /* [11] */
		uint32_t TEMP_RDY: 1;   /* [12] */
		uint32_t WATTNL: 1;     /* [13] */
		uint32_t VANL: 1;       /* [14] */
		uint32_t FVARNL: 1;     /* [15] */
		uint32_t RSTDONE: 1;    /* [16] */
		uint32_t ZXAV: 1;       /* [17] */
		uint32_t RESERVED_2: 1; /* [18] */
		uint32_t ZXAI: 1;       /* [19] */
		uint32_t ZXBI: 1;       /* [20] */
		uint32_t ZXTOAV: 1;     /* [21] */
		uint32_t RESERVED_3: 1; /* [22] */
		uint32_t CRC_DONE: 1;   /* [23] */
		uint32_t CRC_CHG: 1;    /* [24] */
		uint32_t PF_RDY: 1;     /* [25] */
		uint32_t RESERVED_4: 1; /* [28:26] */
		uint32_t MS_STAT: 1;    /* [29] */
		uint32_t EVENT_STAT: 1; /* [30] */
		uint32_t CHIP_STAT: 1;  /* [31] */
	} bits;
	uint32_t as_uint32;
};

union ade9153a_auto_calibration_cfg_reg {
	struct {                         /* BITS */
		uint32_t ACAL_MODE: 1;   /* [00] */
		uint32_t ACAL_RUN: 1;    /* [01] */
		uint32_t ACALMODE_AI: 1; /* [02] */
		uint32_t ACALMODE_BI: 1; /* [03] */
		uint32_t AUTOCAL_AI: 1;  /* [04] */
		uint32_t AUTOCAL_BI: 1;  /* [05] */
		uint32_t AUTOCAL_AV: 1;  /* [06] */
		uint32_t RESERVED: 25;   /* [31:07] */
	} bits;
	uint32_t as_uint32;
};

union ade9153a_register {
	struct sensor_value as_sensor_value;
	struct {
		uint32_t value;
		uint16_t addr;
		uint16_t size;
	};
};

enum sensor_trigger_ade9153a {
	SENSOR_TRIG_ADE9153A_IRQ = SENSOR_TRIG_PRIV_START,
	SENSOR_TRIG_ADE9153A_CF,
};

enum sensor_attribute_ade9153a {
	SENSOR_ATTR_ADE9153A_REGISTER = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_ADE9153A_START_AUTOCALIBRATION,
};

#endif /* _ZEPHYR_DRIVERS_SENSOR_ADE9153A_H_ */
