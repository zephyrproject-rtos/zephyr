/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <uwp_hal.h>

#define APB_GLB_REG_BASE    0x40BC8000
#define SOFT_RST            (APB_GLB_REG_BASE + 0X0000)
#define RF_MASTER_CTRL      (APB_GLB_REG_BASE + 0x0180)

#define CTL_BASE_AON_APB 0x4083C000
#define REG_AON_APB_GNSS_RF_CTRL    (CTL_BASE_AON_APB + 0x0154)
#define REG_AON_APB_REG_CGM_AUTO_EN (CTL_BASE_AON_APB + 0x0330)
#define REG_AON_APB_ADDA_TEST_CTRL3 (CTL_BASE_AON_APB + 0x021c)
#define REG_AON_APB_CHIP_SLP        (CTL_BASE_AON_APB + 0x000C)
#define REG_AON_APB_PD_GNSS_IP_AON_CFG4 (CTL_BASE_AON_APB + 0x00D8)

#define SYS_CTRL_REG_BASE   0x40B18000
#define AHB_EB0         (SYS_CTRL_REG_BASE + 0x0000)
#define GNSS_BB_EN      (SYS_CTRL_REG_BASE + 0X0004)

#define BB_MEM_BASE     0x40C00000
#define BB_SW_RST       (BB_MEM_BASE + 0x00)
#define BB_PP_SEL       (BB_MEM_BASE + 0x04)
#define BB_ENA          (BB_MEM_BASE + 0x08)
#define BB_START        (BB_MEM_BASE + 0x0C)
#define BB_INT_START    (BB_MEM_BASE + 0x10)
#define BB_INT_MASK     (BB_MEM_BASE + 0x14)
#define BB_AE_FIFO_CTRL (BB_MEM_BASE + 0x18)
#define BB_TE_FIFO_CTRL (BB_MEM_BASE + 0x1C)
#define BB_LATCH        (BB_MEM_BASE + 0x20)
#define BB_EM_CTRL      (BB_MEM_BASE + 0x24)
#define BB_MEM_CONF     (BB_MEM_BASE + 0x30)
#define BB_VERSION      (BB_MEM_BASE + 0x40)
#define BB_PIPELINE_CTRL (BB_MEM_BASE + 0x44)
#define BB_PIPELINE_CTRL_EXT (BB_MEM_BASE + 0x48)
#define BB_POWER_CTRL   (BB_MEM_BASE + 0x4C)
#define BB_POWER_STATUS (BB_MEM_BASE + 0x50)

#define BB_SLEEP_COUNT     (BB_MEM_BASE + 0x54)
#define BB_SLEEP_NUM       (BB_MEM_BASE + 0x58)
#define BB_DBG_CLK_CTRL    (BB_MEM_BASE + 0x64)

#define DATA2RAM_BASE_ADDR  0x40F18000
#define DATA2RAM_CONF0_ADDR (DATA2RAM_BASE_ADDR + 0x0000)
#define DATA2RAM_CONF1_ADDR (DATA2RAM_BASE_ADDR + 0x0004)
#define DATA2RAM_CONF2_ADDR (DATA2RAM_BASE_ADDR + 0x0008)
#define DATA2RAM_STATUS0_ADDR (DATA2RAM_BASE_ADDR + 0x0010)
#define DATA2RAM_STATUS1_ADDR (DATA2RAM_BASE_ADDR + 0x0014)
#define DATA2RAM_STATUS2_ADDR (DATA2RAM_BASE_ADDR + 0x0018)

#define RF_CTL_MASTER_BASE  0x40B20000

#define RF_CTRL_REG_BASE    (0x00006000)
#define DEBUG_RF_STATE_CTRL0    (RF_CTRL_REG_BASE + 0x0000)
#define REG_RF_STATE_CTRL0      (RF_CTRL_REG_BASE + 0x0004)
#define DEBUG_RF_STATE_CTRL1    (RF_CTRL_REG_BASE + 0x0008)
#define REG_RF_STATE_CTRL1      (RF_CTRL_REG_BASE + 0x000C)
#define DEBUG_GE_SX_DEBUG0      (RF_CTRL_REG_BASE + 0x0010)
#define REG_GE_SX_DEBUG0        (RF_CTRL_REG_BASE + 0x0014)
#define GE_SX_AFC_ERR_MIN       (RF_CTRL_REG_BASE + 0x0018)
#define GE_SX_STATE              (RF_CTRL_REG_BASE + 0x001C)
#define GE_SX_VCO_VAL           (RF_CTRL_REG_BASE + 0x0020)
#define GE_SX_CP_CUR            (RF_CTRL_REG_BASE + 0x0024)
#define GE_SX_DIVN_CTRL0        (RF_CTRL_REG_BASE + 0x0028)
#define GE_SX_DIVNFRAC_CTRLH    (RF_CTRL_REG_BASE + 0x002C)
#define GE_SX_DIVNFRAC_CTRLL    (RF_CTRL_REG_BASE + 0x0030)
#define GE_SX_LDO_CTRL0         (RF_CTRL_REG_BASE + 0x0034)
#define GE_SX_LDO_CTRL1         (RF_CTRL_REG_BASE + 0x0038)
#define GE_SX_CP_CTRL0          (RF_CTRL_REG_BASE + 0x003C)
#define GE_SX_LPF_CTRL0         (RF_CTRL_REG_BASE + 0x0040)
#define GE_SX_VCO_CTRL0         (RF_CTRL_REG_BASE + 0x0044)
#define GE_SX_AFC_START         (RF_CTRL_REG_BASE + 0x0048)
#define GE_SX_AFC_CTRL0         (RF_CTRL_REG_BASE + 0x004C)
#define GE_SX_AFC_CTRL1         (RF_CTRL_REG_BASE + 0x0050)
#define GE_SX_VCOCAP_CTRL0      (RF_CTRL_REG_BASE + 0x0054)
#define GE_SX_VCOCAP_CTRL1      (RF_CTRL_REG_BASE + 0x0058)
#define GE_SX_PK_CTRL0          (RF_CTRL_REG_BASE + 0x005C)
#define DEBUG_GE_RX_RF_CTRL0    (RF_CTRL_REG_BASE + 0x0060)
#define REG_GE_RX_RF_CTRL0      (RF_CTRL_REG_BASE + 0x0064)
#define GE_RX_RF_CTRL1          (RF_CTRL_REG_BASE + 0x0068)
#define GE_RX_RF_CTRL2          (RF_CTRL_REG_BASE + 0x006C)
#define GE_RX_RF_CTRL3          (RF_CTRL_REG_BASE + 0x0070)
#define DEBUG_GE_RC_DCOC_CAL    (RF_CTRL_REG_BASE + 0x0074)
#define REG_GE_RC_DCOC_CAL      (RF_CTRL_REG_BASE + 0x0078)
#define S0_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x007C)
#define S1_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0080)
#define S2_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0084)
#define S3_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0088)
#define S4_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x008C)
#define S5_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0090)
#define S6_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0094)
#define S7_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x0098)
#define S8_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x009C)
#define S9_GE_RC_DCOC_CAL       (RF_CTRL_REG_BASE + 0x00A0)
#define S10_GE_RC_DCOC_CAL      (RF_CTRL_REG_BASE + 0x00A4)
#define S11_GE_RC_DCOC_CAL      (RF_CTRL_REG_BASE + 0x00A8)
#define S12_GE_RC_DCOC_CAL      (RF_CTRL_REG_BASE + 0x00AC)
#define S13_GE_RC_DCOC_CAL      (RF_CTRL_REG_BASE + 0x00B0)
#define GE_TIA_DCOC_CAL         (RF_CTRL_REG_BASE + 0x00B4)
#define GE_RX_PGA_CBANK_CTRL0   (RF_CTRL_REG_BASE + 0x00B8)
#define GE_RX_GAIN_CTRL0        (RF_CTRL_REG_BASE + 0x00BC)
#define TEST_PIN_DEBUG          (RF_CTRL_REG_BASE + 0x00C0)
#define RF_TRST_CTRL0           (RF_CTRL_REG_BASE + 0x00C4)
#define GE_RFTOP_RESERVED_CTRL0 (RF_CTRL_REG_BASE + 0x00C8)
#define GE_RFTOP_RESERVED_CTRL1 (RF_CTRL_REG_BASE + 0x00CC)
#define GE_RFTOP_RESERVED_CTRL2 (RF_CTRL_REG_BASE + 0x00D0)
#define AD_GE_RFTOP_RESERVED    (RF_CTRL_REG_BASE + 0x00D4)
#define PS_GE_SX_CTRL0          (RF_CTRL_REG_BASE + 0x00D8)
#define PS_GE_SX_CTRL1          (RF_CTRL_REG_BASE + 0x00DC)
#define PS_GE_SX_CTRL2          (RF_CTRL_REG_BASE + 0x00E0)
#define DEBUG_PS_GE_SX_CTRL3    (RF_CTRL_REG_BASE + 0x00E4)
#define REG_PS_GE_SX_CTRL3      (RF_CTRL_REG_BASE + 0x00E8)
#define DEBUG_PS_GE_ADC_CTRL0   (RF_CTRL_REG_BASE + 0x00EC)
#define REG_PS_GE_ADC_CTRL0     (RF_CTRL_REG_BASE + 0x00F0)
#define PS_GE_ADC_CTRL1         (RF_CTRL_REG_BASE + 0x00F4)
#define PS_GE_ADC_CTRL2         (RF_CTRL_REG_BASE + 0x00F8)
#define PS_GE_ADC_CTRL3         (RF_CTRL_REG_BASE + 0x00FC)
#define PS_GE_ADC_CTRL4         (RF_CTRL_REG_BASE + 0x0100)
#define PS_GE_ADC_CTRL5         (RF_CTRL_REG_BASE + 0x0104)
#define PS_GE_ADC_CTRL6         (RF_CTRL_REG_BASE + 0x0108)
#define PS_GE_RX_CTRL1          (RF_CTRL_REG_BASE + 0x010C)
#define PS_GE_RX_CTRL2          (RF_CTRL_REG_BASE + 0x0110)
#define PS_GE_RX_CTRL3          (RF_CTRL_REG_BASE + 0x0114)
#define PS_GE_RX_CTRL4          (RF_CTRL_REG_BASE + 0x0118)
#define DEBUG_PS_GE_RX_CTRL5    (RF_CTRL_REG_BASE + 0x011C)
#define REG_PS_GE_RX_CTRL5      (RF_CTRL_REG_BASE + 0X0120)
#define PS_GE_LDO_ADC_CTRL0     (RF_CTRL_REG_BASE + 0x0124)
#define PS_GE_SX_CTRL4          (RF_CTRL_REG_BASE + 0x0128)
#define DEBUG_PS_GE_CLK_GEN_CTRL0 (RF_CTRL_REG_BASE + 0x012C)
#define REG_PS_GE_CLK_GEN_CTRL0 (RF_CTRL_REG_BASE + 0x0130)
#define PS_GE_CLK_GEN_CTRL1     (RF_CTRL_REG_BASE + 0x0134)
#define PS_GE_CLK_GEN_CTRL2     (RF_CTRL_REG_BASE + 0x0138)
#define PS_GE_CLK_GEN_CTRL3     (RF_CTRL_REG_BASE + 0x013C)
#define PS_GE_LDO_CRIPPLE_CTRL0     (RF_CTRL_REG_BASE + 0x0140)
#define PS_GE_CLKGEN_ANA_ADC_CTRL0 (RF_CTRL_REG_BASE + 0x0144)
#define DEBUG_GNSS_CLK_ENABLE   (RF_CTRL_REG_BASE + 0x0148)
#define REG_GNSS_CLK_ENABLE   (RF_CTRL_REG_BASE + 0x014C)
#define RF_HW_RST_CTRL0        (RF_CTRL_REG_BASE + 0x0150)
#define S0_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0154)
#define S1_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0158)
#define S2_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x015C)
#define S3_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0160)
#define S4_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0164)
#define S5_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0168)
#define S6_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x016C)
#define S7_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0170)
#define S8_RF_HW_RST_CTRL1     (RF_CTRL_REG_BASE + 0x0174)
#define S0_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0178)
#define S1_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x017C)
#define S2_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0180)
#define S3_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0184)
#define S4_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0188)
#define S5_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x018C)
#define S6_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0190)
#define S7_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0194)
#define S8_GE_DECODE_CFG1      (RF_CTRL_REG_BASE + 0x0198)
#define GE_CHARGE_TIME0        (RF_CTRL_REG_BASE + 0x019C)
#define GE_CHARGE_TIME1        (RF_CTRL_REG_BASE + 0x01A0)
#define GE_CHARGE_TIME2        (RF_CTRL_REG_BASE + 0x01A4)
#define GE_CHARGE_TIME3        (RF_CTRL_REG_BASE + 0x01A8)
#define GE_CHARGE_TIME4        (RF_CTRL_REG_BASE + 0x01AC)
#define GE_CHARGE_TIME5        (RF_CTRL_REG_BASE + 0x01B0)
#define ADA_CLK_CFG            (RF_CTRL_REG_BASE + 0x01B4)
#define ADC_DAT_FORMAT         (RF_CTRL_REG_BASE + 0x01B8)
#define ADA_DAT_MINUS_EN       (RF_CTRL_REG_BASE + 0x01BC)
#define OW_CFG                 (RF_CTRL_REG_BASE + 0x01C0)
#define OW_ACTION              (RF_CTRL_REG_BASE + 0x01C4)
#define WB_ADC_CFG             (RF_CTRL_REG_BASE + 0x01C8)
#define WB_ADC_PD_DI           (RF_CTRL_REG_BASE + 0x01CC)
#define WB_ADC_PD_DQ           (RF_CTRL_REG_BASE + 0x01D0)
#define WB_RSSIADC_CFG         (RF_CTRL_REG_BASE + 0x01D4)
#define WB_RSSIADC_PD_DI       (RF_CTRL_REG_BASE + 0x01D8)
#define WB_RSSIADC_PD_DQ       (RF_CTRL_REG_BASE + 0x01DC)
#define GE_ADC_CFG             (RF_CTRL_REG_BASE + 0x01E0)
#define GE_ADC_PD_DI           (RF_CTRL_REG_BASE + 0x01E4)
#define GE_ADC_PD_DQ           (RF_CTRL_REG_BASE + 0x01E8)
#define GE_DCOC_MODE           (RF_CTRL_REG_BASE + 0x01EC)
#define S0_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x01F0)
#define S1_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x01F4)
#define S2_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x01F8)
#define S3_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x01FC)
#define S4_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x0200)
#define S5_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x0204)
#define S6_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x0208)
#define S7_RF_REG_RESERVED     (RF_CTRL_REG_BASE + 0x020C)
#define GE_ADC_TEST_RADDR      (RF_CTRL_REG_BASE + 0x0210)
#define GE_MODE_MUX            (RF_CTRL_REG_BASE + 0x0214)
#define DEBUG_GNSS_RSV_REG      (RF_CTRL_REG_BASE + 0x0218)
#define REG_GNSS_RSV_REG       (RF_CTRL_REG_BASE + 0x021C)
#define PS_CONTROL_TIME0       (RF_CTRL_REG_BASE + 0x0220)
#define PS_CONTROL_TIME1       (RF_CTRL_REG_BASE + 0x0224)
#define PS_CONTROL_TIME2       (RF_CTRL_REG_BASE + 0x0228)
#define PS_CONTROL_TIME3       (RF_CTRL_REG_BASE + 0x022C)
#define DEBUG_BG_GNSS_EN_FORCE_INSERT  (RF_CTRL_REG_BASE + 0x0230)
#define REG_BG_GNSS_EN_FORCE_INSERT (RF_CTRL_REG_BASE + 0x0234)
#define GE_CLK_GATE_CTRL       (RF_CTRL_REG_BASE + 0x0238)
#define GNSS_CAL_RC_CTRL       (RF_CTRL_REG_BASE + 0x023C)
#define GNSS_SINE_GEN_S0       (RF_CTRL_REG_BASE + 0x0240)
#define GNSS_SINE_GEN_S1       (RF_CTRL_REG_BASE + 0x0244)
#define GNSS_SINE_GEN_S2       (RF_CTRL_REG_BASE + 0x0248)
#define GNSS_SINE_GEN_S3       (RF_CTRL_REG_BASE + 0x024C)
#define GNSS_SINE_GEN_S4       (RF_CTRL_REG_BASE + 0x0250)
#define GNSS_SINE_GEN_S5       (RF_CTRL_REG_BASE + 0x0254)
#define GNSS_SINE_GEN_S6       (RF_CTRL_REG_BASE + 0x0258)
#define GNSS_SINE_GEN_S7       (RF_CTRL_REG_BASE + 0x025C)
#define GNSS_SINE_GEN_S8       (RF_CTRL_REG_BASE + 0x0260)
#define GNSS_SINE_GEN_S9       (RF_CTRL_REG_BASE + 0x0264)
#define GNSS_SINE_GEN_S10       (RF_CTRL_REG_BASE + 0x0268)
#define GNSS_SINE_GEN_S11      (RF_CTRL_REG_BASE + 0x026C)
#define GNSS_SINE_GEN_S12       (RF_CTRL_REG_BASE + 0x0270)
#define GNSS_SINE_GEN_S13       (RF_CTRL_REG_BASE + 0x0274)
#define GNSS_SINE_GEN_S14       (RF_CTRL_REG_BASE + 0x0278)
#define GNSS_SINE_GEN_S15       (RF_CTRL_REG_BASE + 0x027C)
#define PS_CONTROL_TIME4        (RF_CTRL_REG_BASE + 0x0280)
#define PS_CONTROL_TIME5        (RF_CTRL_REG_BASE + 0x0284)
#define PS_CONTROL_TIME6        (RF_CTRL_REG_BASE + 0x0288)
#define DEBUG_GNSS_BB_FORCE_INSERT_CTRL0    (RF_CTRL_REG_BASE + 0x028C)
#define REG_GNSS_BB_FORCE_INSERT_CTRL0    (RF_CTRL_REG_BASE + 0x0290)


// define Cal_AHB_IF Register
#define CAL_AHB_IF_BASE     0x40F1C000
#define GE_CALI_CTRL        (CAL_AHB_IF_BASE + 0x0000)
#define GE_SINE_CTRL        (CAL_AHB_IF_BASE + 0x0004)
#define GE_RSSI_CTRL        (CAL_AHB_IF_BASE + 0x0008)
#define GE_CLAI_STS         (CAL_AHB_IF_BASE + 0x000C)
#define GE_DC_EST           (CAL_AHB_IF_BASE + 0x0010)
#define GE_RSSI_PWR         (CAL_AHB_IF_BASE + 0x0014)
#define GE_NCO_CTRL         (CAL_AHB_IF_BASE + 0x0018)
#define GE_IQIM_CTRL        (CAL_AHB_IF_BASE + 0x001C)
#define GE_IQIM_ARG         (CAL_AHB_IF_BASE + 0x0020)
#define GE_CAL_INT_EN       (CAL_AHB_IF_BASE + 0x0024)
#define GE_CAL_INT_CLR      (CAL_AHB_IF_BASE + 0x0028)
#define GE_CAL_INT_SRC      (CAL_AHB_IF_BASE + 0x002C)
#define GE_CAL_INT_RAW      (CAL_AHB_IF_BASE + 0x0030)

#define CTL_BASE_AON_CLK_RF 0x40844200
#define REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG    (CTL_BASE_AON_CLK_RF + 0x0104)
#define REG_AON_CLK_RF_CGM_GNSS_ARM_CFG     (CTL_BASE_AON_CLK_RF + 0x0108)
#define REG_AON_CLK_RF_CGM_GNSS_BB_CFG      (CTL_BASE_AON_CLK_RF + 0x010C)
#define REG_AON_CLK_RF_CGM_GNSS_AE_CFG      (CTL_BASE_AON_CLK_RF + 0x0110)
#define REG_AON_CLK_RF_CGM_GNSS_APB_CFG     (CTL_BASE_AON_CLK_RF + 0X011C)


#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_DIV_ADDR REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_DIV_ADDR_SHIFT   8
#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_DIV_ADDR_MASK    0x00000300

#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_SEL_ADDR   REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_SEL_ADDR_SHIFT   0
#define BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_SEL_ADDR_MASK    0x00000003

#define BIT_AON_CLK_RF_CGM_GNSS_ARM_CFG_CGM_GNSS_ARM_DIV_ADDR   REG_AON_CLK_RF_CGM_GNSS_ARM_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_ARM_CFG_CGM_GNSS_ARM_DIV_ADDR_SHIFT   8
#define BIT_AON_CLK_RF_CGM_GNSS_ARM_CFG_CGM_GNSS_ARM_DIV_ADDR_MASK    0x00000300

#define BIT_AON_CLK_RF_CGM_GNSS_BB_CFG_CGM_GNSS_BB_DIV_ADDR   REG_AON_CLK_RF_CGM_GNSS_BB_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_BB_CFG_CGM_GNSS_BB_DIV_ADDR_SHIFT   8
#define BIT_AON_CLK_RF_CGM_GNSS_BB_CFG_CGM_GNSS_BB_DIV_ADDR_MASK    0x00000300

#define BIT_AON_CLK_RF_CGM_GNSS_AE_CFG_CGM_GNSS_AE_DIV_ADDR   REG_AON_CLK_RF_CGM_GNSS_AE_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_AE_CFG_CGM_GNSS_AE_DIV_ADDR_SHIFT   8
#define BIT_AON_CLK_RF_CGM_GNSS_AE_CFG_CGM_GNSS_AE_DIV_ADDR_MASK    0x00000700

#define BIT_AON_CLK_RF_CGM_GNSS_APB_CFG_CGM_GNSS_APB_SEL_ADDR   REG_AON_CLK_RF_CGM_GNSS_APB_CFG
#define BIT_AON_CLK_RF_CGM_GNSS_APB_CFG_CGM_GNSS_APB_SEL_ADDR_SHIFT   0
#define BIT_AON_CLK_RF_CGM_GNSS_APB_CFG_CGM_GNSS_APB_SEL_ADDR_MASK    0x00000001

#define SPI_HREADY_EN   BIT(7)
#define SPI_SEL         BIT(6)

#define GNSS_SLP        BIT(2)
#define PLL_PDN         BIT(5)
#define RF_PDN          BIT(6)

#define GNSS_SLEEPING   BIT(3)
#define GNSS_POWERON_FINISH BIT(4)
#define PLL_CNT_DONE    BIT(6)

#define RX_ADC_EN_REG   BIT(0)
#define RX_ADC_LDO_EN   BIT(1)

#define LNA_EN_REG      BIT(1)

#define GE_MODE_CTRL_SEL    BIT(15)

#define DCOC_CAL_ENB    BIT(8)
#define RC_CALI_MOD     BIT(5)
#define RC_CALI_ENB     BIT(4)

#define FREQ_MUX        BIT(9)
#define NCO_EN          BIT(2)

#define RSSI_ENB        BIT(2)
#define RSSI_UPD        BIT(1)

#define RSSI_CAL_DONE   BIT(2)
#define WBRSSI_UPD      BIT(1)
#define EST_DONE        BIT(0)

#define IQ_COMP_ENB     BIT(0)

typedef enum {
	RF_GPS_MODE = 0,
	RF_BD2_MODE,
	RF_GPS_BD2_MODE,
	RF_GPS_GLO_MODE,
	RF_GPS_BD_GLO_MODE
} RF_WORK_MODE;

#define BIT_AON_APB_GNSS_RF_CTRL_GNSS_RF_CTRL_ADDR          REG_AON_APB_GNSS_RF_CTRL
#define BIT_AON_APB_GNSS_RF_CTRL_GNSS_RF_CTRL_ADDR_SHIFT    0
#define BIT_AON_APB_GNSS_RF_CTRL_GNSS_RF_CTRL_ADDR_MASK     0xFFFFFFFF

#define BIT_AON_APB_REG_CGM_AUTO_EN_BB_REF_FREQ_SEL_ADDR          REG_AON_APB_REG_CGM_AUTO_EN
#define BIT_AON_APB_REG_CGM_AUTO_EN_BB_REF_FREQ_SEL_ADDR_SHIFT    16
#define BIT_AON_APB_REG_CGM_AUTO_EN_BB_REF_FREQ_SEL_ADDR_MASK     0x00010000

#define BIT_AON_APB_ADDA_TEST_CTRL3_GNSS_ADC_EN_ADDR        REG_AON_APB_ADDA_TEST_CTRL3
#define BIT_AON_APB_ADDA_TEST_CTRL3_GNSS_ADC_EN_ADDR_SHIFT  2
#define BIT_AON_APB_ADDA_TEST_CTRL3_GNSS_ADC_EN_ADDR_MASK   0x00000004

#define BIT_AON_APB_PD_GNSS_IP_AON_CFG4_GNSS_IP_POWER_DOWN_ADDR  REG_AON_APB_PD_GNSS_IP_AON_CFG4
#define BIT_AON_APB_PD_GNSS_IP_AON_CFG4_GNSS_IP_POWER_DOWN_ADDR_SHIFT   1
#define BIT_AON_APB_PD_GNSS_IP_AON_CFG4_GNSS_IP_POWER_DOWN_ADDR_MASK    0x00000002

#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_PWRON_FINISH_ADDR        REG_AON_APB_CHIP_SLP
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_PWRON_FINISH_ADDR_SHIFT  13
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_PWRON_FINISH_ADDR_MASK   0x00002000

#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SS_PWRON_FINISH_ADDR        REG_AON_APB_CHIP_SLP
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SS_PWRON_FINISH_ADDR_SHIFT  12
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SS_PWRON_FINISH_ADDR_MASK   0x00001000

#define BIT_AON_APB_CHIP_SLP_BTWF_SS_ARM_SYS_SLEEPING_ADDR        REG_AON_APB_CHIP_SLP
#define BIT_AON_APB_CHIP_SLP_BTWF_SS_ARM_SYS_SLEEPING_ADDR_SHIFT  11
#define BIT_AON_APB_CHIP_SLP_BTWF_SS_ARM_SYS_SLEEPING_ADDR_MASK   0x00000800

#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SLEEPING_ADDR        REG_AON_APB_CHIP_SLP
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SLEEPING_ADDR_SHIFT   5
#define BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SLEEPING_ADDR_MASK    0x00000020

#define RF_SYS_CLK  266

#define APB_ADC_ENABLE() \
	(sci_reg_or(REG_AON_APB_ADDA_TEST_CTRL3, BIT_AON_APB_ADDA_TEST_CTRL3_GNSS_ADC_EN_ADDR_MASK))

#define APB_ADC_DISABLE() \
	(sci_reg_and(REG_AON_APB_ADDA_TEST_CTRL3, ~BIT_AON_APB_ADDA_TEST_CTRL3_GNSS_ADC_EN_ADDR_MASK))

#define APB_GNSS_POWER_ON() \
	(sci_reg_and(REG_AON_APB_PD_GNSS_IP_AON_CFG4, ~BIT_AON_APB_PD_GNSS_IP_AON_CFG4_GNSS_IP_POWER_DOWN_ADDR_MASK))

#define APB_GNSS_POWER_OFF() \
	(sci_reg_or(REG_AON_APB_PD_GNSS_IP_AON_CFG4, BIT_AON_APB_PD_GNSS_IP_AON_CFG4_GNSS_IP_POWER_DOWN_ADDR_MASK))


#define APB_GET_GNSS_POWERON_FINISH() \
	((sci_read32(REG_AON_APB_CHIP_SLP) & BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_PWRON_FINISH_ADDR_MASK) >> BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_PWRON_FINISH_ADDR_SHIFT)

#define APB_GET_GNSS_SLEEPING()	\
	((sci_read32(REG_AON_APB_CHIP_SLP) & BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SLEEPING_ADDR_MASK) >> BIT_AON_APB_CHIP_SLP_GNSS_SS_GNSS_SLEEPING_ADDR_SHIFT)

#define APB_SET_FAKE_CLK(clk_sel, div)														      \
	do {																	      \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_DIV_ADDR_MASK));			      \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG, (((div) & 0x3) << BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_DIV_ADDR_SHIFT)));     \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_SEL_ADDR_MASK));			      \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_FAKE_CFG, (((clk_sel) & 0x3) << BIT_AON_CLK_RF_CGM_GNSS_FAKE_CFG_CGM_GNSS_FAKE_SEL_ADDR_SHIFT))); \
	} while (0)

#define APB_SET_ARM_CLK(div)														     \
	do {																     \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_ARM_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_ARM_CFG_CGM_GNSS_ARM_DIV_ADDR_MASK));		     \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_ARM_CFG, ((div) & 0x3) << BIT_AON_CLK_RF_CGM_GNSS_ARM_CFG_CGM_GNSS_ARM_DIV_ADDR_SHIFT)); \
	} while (0)

#define APB_SET_APB_CLK(clk_sel)														 \
	do {																	 \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_APB_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_APB_CFG_CGM_GNSS_APB_SEL_ADDR_MASK));			 \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_APB_CFG, ((clk_sel) & 0x1) << BIT_AON_CLK_RF_CGM_GNSS_APB_CFG_CGM_GNSS_APB_SEL_ADDR_SHIFT)); \
	} while (0)

#define APB_SET_BB_CLK(div)														  \
	do {																  \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_BB_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_BB_CFG_CGM_GNSS_BB_DIV_ADDR_MASK));		  \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_BB_CFG, ((div) & 0x3) << BIT_AON_CLK_RF_CGM_GNSS_BB_CFG_CGM_GNSS_BB_DIV_ADDR_SHIFT)); \
	} while (0)

#define APB_SET_AE_CLK(div)														  \
	do {																  \
		(sci_reg_and(REG_AON_CLK_RF_CGM_GNSS_AE_CFG, ~BIT_AON_CLK_RF_CGM_GNSS_AE_CFG_CGM_GNSS_AE_DIV_ADDR_MASK));		  \
		(sci_reg_or(REG_AON_CLK_RF_CGM_GNSS_AE_CFG, ((div) & 0x7) << BIT_AON_CLK_RF_CGM_GNSS_AE_CFG_CGM_GNSS_AE_DIV_ADDR_SHIFT)); \
	} while (0)

#define CAL_DCOC_DISABLE() \
	(sci_reg_and(GE_CALI_CTRL, ~DCOC_CAL_ENB))

#define CAL_SET_DCOC_SHIFTBIT(shiftbit)				      \
	do {							      \
		(sci_reg_and(GE_CALI_CTRL, 0xFFFF0FFF));	      \
		(sci_reg_or(GE_CALI_CTRL, ((shiftbit & 0xf) << 16))); \
	} while (0)

#define SYS_SET_GNSS_BB_EN(value) \
	(sci_write32(GNSS_BB_EN, value))

#define DELAY(time)									 \
	do {										 \
		u32_t delay_time;							 \
		for (delay_time = 0; delay_time < (time * RF_SYS_CLK * 2); delay_time++) \
		{}									 \
	} while (0)

typedef struct {
	u32_t divn;
	u32_t divfrach;
	u32_t divfracl;
} LO_FREQ_STRC;
#define RF_LO_FREQ_TABLE_CRYSTAL_OFFSET     5
const LO_FREQ_STRC g_RF_LO_FREQ[10] =
{
	{ 0x78, 0xD, 0xF131 },
	{ 0x77, 0xC, 0x50A9 },
	{ 0x78, 0xA, 0x2A38 },
	{ 0x7A, 0x3, 0x296F },
	{ 0x79, 0xA, 0x592B },

	{ 0x51, 0xD, 0x70A4 },
	{ 0x51, 0x1, 0x8148 },
	{ 0x51, 0xA, 0xE1EC },
	{ 0x52, 0xB, 0xCEB8 },
	{ 0x52, 0x5, 0xD70A }
};

void RF_readCtlReg(u32_t addr, u32_t *data)
{
	u32_t reg_data = 0;

	reg_data = ((addr & 0x7FFF) << 16) | 0x80000000;
	sci_write32(RF_CTL_MASTER_BASE, reg_data);
	DELAY(1);

	reg_data = sci_read32(RF_CTL_MASTER_BASE);
	*data = reg_data & 0xFFFF;
	DELAY(1);
}

void RF_writeCtlReg(u32_t addr, u32_t data)
{
	u32_t reg_data = 0;

	reg_data = ((addr & 0X7FFF) << 16) | (data & 0xFFFF);
	sci_write32(RF_CTL_MASTER_BASE, reg_data);

	DELAY(1);
}

void RF_softReset(void)
{
	sci_reg_or(SOFT_RST, BIT(1));
	DELAY(1);
	sci_reg_and(SOFT_RST, ~BIT(1));
}

void RF_setInterface(void)
{
	u32_t sel;

	sel = 1;

	if (sel) {
		sci_reg_or(RF_MASTER_CTRL, (SPI_SEL | SPI_HREADY_EN));
	} else {
		sci_reg_and(RF_MASTER_CTRL, ~SPI_SEL);
	}
}

void RF_setSWCtrlFSMState(void)
{
	u32_t data;

	RF_readCtlReg(GE_MODE_MUX, &data);
	data &= ~GE_MODE_CTRL_SEL;
	RF_writeCtlReg(GE_MODE_MUX, data);
}

void RF_switchState(u32_t state)
{
	u32_t data;

	RF_readCtlReg(RF_HW_RST_CTRL0, &data);
	data &= 0xFFFFE1FF;
	data |= (state << 9);
	RF_writeCtlReg(RF_HW_RST_CTRL0, data);
}

void RF_setSXFreqDivRatio(LO_FREQ_STRC regs_cfg)
{
	u32_t data;

	RF_readCtlReg(GE_SX_DIVN_CTRL0, &data);
	data &= 0xFFFF00FF;
	data |=  (regs_cfg.divn << 8);
	RF_writeCtlReg(GE_SX_DIVN_CTRL0, data);

	RF_readCtlReg(GE_SX_DIVNFRAC_CTRLH, &data);
	data &= 0xFFFF0FFF;
	data |= (regs_cfg.divfrach << 12);
	RF_writeCtlReg(GE_SX_DIVNFRAC_CTRLH, data);

	RF_readCtlReg(GE_SX_DIVNFRAC_CTRLL, &data);
	data &= 0xFFFF0000;
	data |= regs_cfg.divfracl;
	RF_writeCtlReg(GE_SX_DIVNFRAC_CTRLL, data);
}

void RF_setGNSSMode(u32_t sys_indx)
{
	u32_t data;

	RF_readCtlReg(PS_GE_RX_CTRL3, &data);
	data &= 0xFFFF1FFF;
	data |= ((sys_indx & 0x7) << 13);
	RF_writeCtlReg(PS_GE_RX_CTRL3, data);

}

void RF_setADCclk(u32_t divRatio1, u32_t divRatio2)
{
	u32_t data;

	RF_readCtlReg(PS_GE_CLK_GEN_CTRL1, &data);
	data &= 0xffffc3ff;
	data |= ((divRatio1 & 0xF) << 10);
	RF_writeCtlReg(PS_GE_CLK_GEN_CTRL1, data);

	RF_readCtlReg(PS_GE_CLK_GEN_CTRL3, &data);
	data &= 0xffffff9f;
	data |= ((divRatio2 & 0x3) << 5);
	RF_writeCtlReg(PS_GE_CLK_GEN_CTRL3, data);

	RF_readCtlReg(DEBUG_PS_GE_CLK_GEN_CTRL0, &data);
	data |= (BIT(12) | BIT(11));
	RF_writeCtlReg(DEBUG_PS_GE_CLK_GEN_CTRL0, data);
	RF_readCtlReg(REG_PS_GE_CLK_GEN_CTRL0, &data);
	data |= (BIT(12) | BIT(11));
	RF_writeCtlReg(REG_PS_GE_CLK_GEN_CTRL0, data);

	RF_readCtlReg(GE_CLK_GATE_CTRL, &data);
	data |= BIT(14);
	RF_writeCtlReg(GE_CLK_GATE_CTRL, data);
}

void RF_setPPclk(u32_t divRatio1, u32_t divRatio2)
{
	u32_t data;

	RF_readCtlReg(PS_GE_CLK_GEN_CTRL2, &data);
	data &= 0xfffff87f;
	data |= ((divRatio1 & 0xf) << 7);
	RF_writeCtlReg(PS_GE_CLK_GEN_CTRL2, data);

	RF_readCtlReg(PS_GE_CLK_GEN_CTRL3, &data);
	data &= 0xffffffe7;
	data |= ((divRatio2 & 0x3) << 3);
	RF_writeCtlReg(PS_GE_CLK_GEN_CTRL3, data);

	RF_readCtlReg(DEBUG_PS_GE_CLK_GEN_CTRL0, &data);
	data |= (BIT(8) | BIT(7));
	RF_writeCtlReg(DEBUG_PS_GE_CLK_GEN_CTRL0, data);
	RF_readCtlReg(REG_PS_GE_CLK_GEN_CTRL0, &data);
	data |= (BIT(8) | BIT(7));
	RF_writeCtlReg(REG_PS_GE_CLK_GEN_CTRL0, data);

	RF_readCtlReg(GE_CLK_GATE_CTRL, &data);
	data |= BIT(15);
	RF_writeCtlReg(GE_CLK_GATE_CTRL, data);
}

void RF_setADCEnable(u32_t divRatio1, u32_t divRatio2)
{
	RF_setADCclk(divRatio1, divRatio2);
	APB_ADC_ENABLE();
}

void RF_setPGAGain(u32_t gain)
{
	u32_t data;

	RF_readCtlReg(GE_RX_GAIN_CTRL0, &data);
	data &= 0xffff0fff;
	data |= ((gain & 0xf) << 12);
	RF_writeCtlReg(GE_RX_GAIN_CTRL0, data);
}

void RF_Control(s16_t RF_Mode)
{
	u32_t data_tmp;

	sci_reg_or(BIT_AON_APB_GNSS_RF_CTRL_GNSS_RF_CTRL_ADDR, BIT(1));

	RF_softReset();
	RF_setInterface();
	RF_setSWCtrlFSMState();
	RF_switchState(0);
	DELAY(1);

	RF_writeCtlReg(GE_CHARGE_TIME1, 0x5000);
	RF_writeCtlReg(GE_CHARGE_TIME2, 0x5000);
	RF_writeCtlReg(GE_CHARGE_TIME3, 0x5000);
	RF_writeCtlReg(GE_CHARGE_TIME4, 0x5000);
	RF_writeCtlReg(GE_CHARGE_TIME5, 0x5000);

	RF_switchState(1);
	DELAY(70);
	RF_switchState(2);
	DELAY(1);

	if ((sci_read32(BIT_AON_APB_REG_CGM_AUTO_EN_BB_REF_FREQ_SEL_ADDR) & BIT_AON_APB_REG_CGM_AUTO_EN_BB_REF_FREQ_SEL_ADDR_MASK) == 0) {
		RF_setSXFreqDivRatio(g_RF_LO_FREQ[RF_Mode + RF_LO_FREQ_TABLE_CRYSTAL_OFFSET]);
	} else {
		RF_setSXFreqDivRatio(g_RF_LO_FREQ[RF_Mode]);
	}

	RF_switchState(3);
	DELAY(5);
	DELAY(200);

	RF_switchState(4);
	DELAY(1);

	RF_setGNSSMode(RF_Mode);
	RF_setADCEnable(9, 1);
	RF_setPPclk(9, 1);

	APB_SET_FAKE_CLK(2, 0);

#if 1
	APB_SET_ARM_CLK(0);
	APB_SET_APB_CLK(1);
	APB_SET_BB_CLK(1);
	APB_SET_AE_CLK(1);

	APB_GNSS_POWER_ON();
	DELAY(1);

	u32_t gnss_pwron_finish_flag = 0;
	while (!gnss_pwron_finish_flag) {
		gnss_pwron_finish_flag  = APB_GET_GNSS_POWERON_FINISH();
		DELAY(1);
	}

	SYS_SET_GNSS_BB_EN(0XFFF);

	RF_readCtlReg(GE_RX_PGA_CBANK_CTRL0, &data_tmp);
	data_tmp &= 0xffff0fff;
	data_tmp |= 0x1 << 12;
	data_tmp |= 0x1 << 14;
	RF_writeCtlReg(GE_RX_PGA_CBANK_CTRL0, data_tmp);

#endif
}

void RF_CFG(void)
{
	s16_t rf_mode = 0;

	rf_mode = RF_GPS_BD_GLO_MODE;
	RF_Control(rf_mode);
}

void BB_RESET(void)
{
	u32_t regValue = 0x0;

	regValue = sci_read32(SOFT_RST);
	sci_write32(SOFT_RST, (regValue | 0x100000));
	DELAY(1);
	sci_write32(SOFT_RST, (regValue & 0xffefffff));
}

void M4_CLK_CFG(void)
{
	sci_write32(GNSS_BB_EN, 0x4FF);
	BB_RESET();
}

void GNSS_Start(void)
{
	SYS_LOG_DBG("gnss init start");
	RF_CFG();

	M4_CLK_CFG();
	sci_reg_or(GNSS_BB_EN, BIT(4) | BIT(6) | BIT(7));
	sci_reg_or(BB_DBG_CLK_CTRL, BIT(0));
	sci_write32(DATA2RAM_CONF0_ADDR, 0x04000002);
	SYS_LOG_DBG("gnss init done");
}
