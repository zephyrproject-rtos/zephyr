/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov7675

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ov7675, CONFIG_VIDEO_LOG_LEVEL);

struct ov7675_config {
	struct i2c_dt_spec bus;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

struct ov7675_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct ov7675_data {
	struct ov7675_ctrls ctrls;
	struct video_format fmt;
};

/* ov7675 registers */
#ifndef OMV_OV7670_CLKRC
#define OMV_OV7670_CLKRC   (0x00)
#endif

#define GAIN                    0x00 /* AGC - Gain control gain setting     */
#define BLUE                    0x01 /* AWB - Blue channel gain setting     */
#define RED                     0x02 /* AWB - Red channel gain setting      */
#define VREF                    0x03 /* Vertical Frame Control              */
#define COM1                    0x04 /* Common Control 1                    */
#define BAVE                    0x05 /* U/B Average Level                   */
#define GBAVE                   0x06 /* Y/Gb Average Level                  */
#define AECHH                   0x07 /* Exposure Value - AEC MSB 5 bits     */
#define RAVE                    0x08 /* V/R Average Level                   */

#define COM2                    0x09 /* Common Control 2            */
#define COM2_SOFT_SLEEP         0x10 /* Soft sleep mode             */
#define COM2_OUT_DRIVE_1x       0x00 /* Output drive capability 1x  */
#define COM2_OUT_DRIVE_2x       0x01 /* Output drive capability 2x  */
#define COM2_OUT_DRIVE_3x       0x02 /* Output drive capability 3x  */
#define COM2_OUT_DRIVE_4x       0x03 /* Output drive capability 4x  */

#define OV7675_PID              0x0A /* Product ID Number MSB */
#define VER                     0x0B /* Product ID Number LSB */

#define COM3                    0x0C /* Common Control 3                                       */
#define COM3_SWAP_MSB           0x40 /* Output data MSB and LSB swap                           */
#define COM3_TRI_CLOCK          0x20 /* Tri-state option for output clock at power-down period */
#define COM3_TRI_DATA           0x10 /* Tri-state option for output data at power-down period  */
#define COM3_SCALE_EN           0x08 /* Scale enable                                           */
#define COM3_DCW_EN             0x04 /* DCW enable                                             */

#define COM4                    0x0D /* Common Control 4         */
#define COM4_AEC_FULL           0x00 /* AEC evaluate full window */
#define COM4_AEC_1_2            0x10 /* AEC evaluate 1/2 window  */
#define COM4_AEC_1_4            0x20 /* AEC evaluate 1/4 window  */
#define COM4_AEC_2_3            0x30 /* AEC evaluate 2/3 window  */

#define COM5                    0x0E /* Reserved                 */

#define COM6                    0x0F /* Common Control 6                     */
#define COM6_HREF_BLACK_EN      0x80 /* Enable HREF at optical black         */
#define COM6_RESET_TIMING       0x02 /* Reset all timing when format changes */

#define AECH                    0x10 /* AEC[9:2] (see registers AECHH for AEC[15:10] and COM1 for AEC[1:0]) */

#define CLKRC                   0x11 /* Internal Clock              */
#define CLKRC_PRESCALER_BYPASS  0x40 /* Use external clock directly */
#define CLKRC_PRESCALER         0x3F /* Internal clock pre-scaler   */

#define COM7                    0x12 /* Common Control 7         */
#define COM7_RESET              0x80 /* SCCB Register Reset      */
#define COM7_RES_VGA            0x00 /* Resolution VGA           */
#define COM7_RES_CIF            0x20 /* Resolution CIF           */
#define COM7_RES_QVGA           0x10 /* Resolution QVGA          */
#define COM7_RES_QCIF           0x08 /* Resolution QCIF          */
#define COM7_RGB_FMT            0x04 /* Output format RGB        */
#define COM7_CBAR_EN            0x02 /* Color bar selection      */
#define COM7_FMT_RAW            0x01 /* Output format Bayer RAW  */

#define COM8                    0x13 /* Common Control 8                            */
#define COM8_FAST_AEC           0x80 /* Enable fast AGC/AEC algorithm               */
#define COM8_AEC_STEP           0x40 /* AEC - Step size unlimited step size         */
#define COM8_BANDF_EN           0x20 /* Banding filter ON/OFF                       */
#define COM8_AGC_EN             0x04 /* AGC Enable                                  */
#define COM8_AWB_EN             0x02 /* AWB Enable                                  */
#define COM8_AEC_EN             0x01 /* AEC Enable                                  */

#define COM9                    0x14 /* Common Control 9            */
#define COM9_AGC_GAIN_2x        0x00 /* Automatic Gain Ceiling 2x   */
#define COM9_AGC_GAIN_4x        0x10 /* Automatic Gain Ceiling 4x   */
#define COM9_AGC_GAIN_8x        0x20 /* Automatic Gain Ceiling 8x   */
#define COM9_AGC_GAIN_16x       0x30 /* Automatic Gain Ceiling 16x  */
#define COM9_AGC_GAIN_32x       0x40 /* Automatic Gain Ceiling 32x  */
#define COM9_AGC_GAIN_64x       0x50 /* Automatic Gain Ceiling 64x  */
#define COM9_AGC_GAIN_128x      0x60 /* Automatic Gain Ceiling 128x */
#define COM9_SET_AGC(r, x)      ((r&0x8F)|((x&0x07)<<4))

#define COM10                   0x15 /* Common Control 10                                   */
#define COM10_HSYNC_EN          0x40 /* HREF changes to HSYNC                               */
#define COM10_PCLK_FREE         0x00 /* PCLK output option: free running PCLK               */
#define COM10_PCLK_MASK         0x20 /* PCLK output option: masked during horizontal blank  */
#define COM10_PCLK_REV          0x10 /* PCLK reverse                                        */
#define COM10_HREF_REV          0x08 /* HREF reverse                                        */
#define COM10_VSYNC_FALLING     0x00 /* VSYNC changes on falling edge of PCLK               */
#define COM10_VSYNC_RISING      0x04 /* VSYNC changes on rising edge of PCLK                */
#define COM10_VSYNC_NEG         0x02 /* VSYNC negative                                      */
#define COM10_HSYNC_NEG         0x01 /* HSYNC negative                                      */

#define HSTART                  0x17 /* Output Format - Horizontal Frame (HREF column) start high 8-bit */
#define HSTOP                   0x18 /* Output Format - Horizontal Frame (HREF column) end high 8-bit   */
#define VSTART                  0x19 /* Output Format - Vertical Frame (row) start high 8-bit           */
#define VSTOP                   0x1A /* Output Format - Vertical Frame (row) end high 8-bit             */
#define PSHFT                   0x1B /* Data Format - Pixel Delay Select (delays timing of the D[7:0])  */
#define MIDH                    0x1C /* Manufacturer ID Byte - High                                     */
#define MIDL                    0x1D /* Manufacturer ID Byte - Low                                      */

#define MVFP                    0x1E /* Mirror/VFlip Enable */
#define MVFP_MIRROR             0x20 /* Mirror image        */
#define MVFP_VFLIP              0x10 /* VFlip image         */
#define MVFP_BLACK_SUN_EN       0x04 /* Black sun enable    */

#define LAEC                    0x1F /* Reserved */

#define ADCCTR0                 0x20 /* ADC Control                     */
#define ADCCTR0_ADC_RANGE_1X    0x00 /* ADC range adjustment 1x range   */
#define ADCCTR0_ADC_RANGE_1_5X  0x08 /* ADC range adjustment 1.5x range */
#define ADCCTR0_ADC_REF_0_8     0x00 /* ADC reference adjustment 0.8x   */
#define ADCCTR0_ADC_REF_1_0     0x04 /* ADC reference adjustment 1.0x   */
#define ADCCTR0_ADC_REF_1_2     0x07 /* ADC reference adjustment 1.2x   */

#define ADCCTR1                 0x21 /* Reserved */
#define ADCCTR2                 0x22 /* Reserved */
#define ADCCTR3                 0x23 /* Reserved */

#define AEW                     0x24 /* AGC/AEC - Stable Operating Region (Upper Limit)                 */
#define AEB                     0x25 /* AGC/AEC - Stable Operating Region (Lower Limit)                 */
#define VPT                     0x26 /* AGC/AEC Fast Mode Operating Region                              */
#define BBIAS                   0x27 /* B Channel Signal Output Bias (effective only when COM6[3] = 1)  */
#define GBIAS                   0x28 /* Gb Channel Signal Output Bias (effective only when COM6[3] = 1) */
#define EXHCH                   0x2A /* Dummy Pixel Insert MSB                                          */
#define EXHCL                   0x2B /* Dummy Pixel Insert LSB                                          */
#define RBIAS                   0x2C /* R Channel Signal Output Bias (effective only when COM6[3] = 1)  */
#define ADVFL                   0x2D /* LSB of insert dummy lines in vertical direction (1 bit equals 1 line) */
#define ADVFH                   0x2E /* MSB of insert dummy lines in vertical direction                 */
#define YAVE                    0x2F /* Y/G Channel Average Value                                       */

#define HSYST                   0x30 /* HSYNC Rising Edge Delay (low 8 bits)    */
#define HSYEN                   0x31 /* HSYNC Falling Edge Delay (low 8 bits)   */
#define HREF                    0x32 /* HREF Control                            */
#define CHLF                    0x33 /* Array Current Control                   */
#define ARBLM                   0x34 /* Array Reference Control                 */
#define ADC                     0x37 /* ADC Control                             */
#define ACOM                    0x38 /* ADC and Analog Common Mode Control      */
#define OFON                    0x39 /* ADC Offset Control                      */

#define TSLB                    0x3A /* Line Buffer Test Option                         */
#define TSLB_NEGATIVE           0x20 /* Negative image enable                           */
#define TSLB_UV_NORMAL          0x00 /* UV output value normal                          */
#define TSLB_UV_FIXED           0x10 /* UV output value set in registers MANU and MANV  */
#define TSLB_AUTO_WINDOW        0x01 /* automatically sets output window when resolution changes */

#define COM11                   0x3B /* Common Control 11 */
#define COM11_AFR               0x80 /* Auto frame rate control ON/OFF selection (night mode) */
#define COM11_AFR_0             0x00 /* No reduction of frame rate          */
#define COM11_AFR_1_2           0x20 /* Max reduction to 1/2 frame rate     */
#define COM11_AFR_1_4           0x40 /* Max reduction to 1/4 frame rate     */
#define COM11_AFR_1_8           0x60 /* Max reduction to 1/8 frame rate     */
#define COM11_HZAUTO_EN         0x10 /* Enable 50/60 Hz auto detection      */
#define COM11_BANDF_SELECT      0x08 /* Banding filter value selection      */
#define COM11_AEC_BANDF         0x02 /* Enable AEC below banding value      */

#define COM12                   0x3C /* Common Control 12   */
#define COM12_HREF_EN           0x80 /* Always has HREF     */

#define COM13                   0x3D /* Common Control 13 */
#define COM13_GAMMA_EN          0x80 /* Gamma enable */
#define COM13_UVSAT_AUTO        0x40 /* UV saturation level - UV auto adjustment. */
#define COM13_UV_SWAP           0x01 /* UV swap                                   */

#define COM14                   0x3E /* Common Control 14                                   */
#define COM14_DCW_EN            0x10 /* DCW and scaling PCLK                                */
#define COM14_MANUAL_SCALE      0x08 /* Manual scaling enable for pre-defined resolutions   */
#define COM14_PCLK_DIV_1        0x00 /* PCLK divider. Divided by 1                          */
#define COM14_PCLK_DIV_2        0x01 /* PCLK divider. Divided by 2                          */
#define COM14_PCLK_DIV_4        0x02 /* PCLK divider. Divided by 4                          */
#define COM14_PCLK_DIV_8        0x03 /* PCLK divider. Divided by 8                          */
#define COM14_PCLK_DIV_16       0x04 /* PCLK divider. Divided by 16                         */

#define EDGE                    0x3F /* Edge Enhancement Adjustment                         */
#define EDGE_FACTOR_MASK        0x1F /* Edge enhancement factor                             */

#define COM15                   0x40 /* Common Control 15           */
#define COM15_OUT_10_F0         0x00 /* Output data range 10 to F0  */
#define COM15_OUT_01_FE         0x80 /* Output data range 01 to FE  */
#define COM15_OUT_00_FF         0xC0 /* Output data range 00 to FF  */
#define COM15_FMT_RGB_NORMAL    0x00 /* Normal RGB normal output    */
#define COM15_FMT_RGB565        0x10 /* Normal RGB 565 output       */
#define COM15_FMT_RGB555        0x30 /* Normal RGB 555 output       */

#define COM16                   0x41 /* Common Control 16                                   */
#define COM16_EDGE_EN           0x20 /* Enable edge enhancement threshold auto-adjustment   */
#define COM16_DENOISE_EN        0x10 /* De-noise threshold auto-adjustment                  */
#define COM16_AWB_GAIN_EN       0x08 /* AWB gain enable                                     */
#define COM16_COLOR_MTX_EN      0x02 /* Color matrix coefficient double option              */

#define COM17                   0x42 /* Common Control 17        */
#define COM17_AEC_FULL          0x00 /* AEC evaluate full window */
#define COM17_AEC_1_2           0x40 /* AEC evaluate 1/2 window  */
#define COM17_AEC_1_4           0x80 /* AEC evaluate 1/4 window  */
#define COM17_AEC_2_3           0xC0 /* AEC evaluate 2/3 window  */
#define COM17_DSP_CBAR_EN       0x08 /* DSP color bar enable     */

#define AWBC1                   0x43 /* Reserved */
#define AWBC2                   0x44 /* Reserved */
#define AWBC3                   0x45 /* Reserved */
#define AWBC4                   0x46 /* Reserved */
#define AWBC5                   0x47 /* Reserved */
#define AWBC6                   0x48 /* Reserved */

#define REG4B                   0x4B /* REG4B             */
#define REG4B_UV_AVG_EN         0x01 /* UV average enable */

#define DNSTH                   0x4C /* De-noise Threshold*/

#define MTX1                    0x4F /* Matrix Coefficient 1 */
#define MTX2                    0x50 /* Matrix Coefficient 2 */
#define MTX3                    0x51 /* Matrix Coefficient 3 */
#define MTX4                    0x52 /* Matrix Coefficient 4 */
#define MTX5                    0x53 /* Matrix Coefficient 5 */
#define MTX6                    0x54 /* Matrix Coefficient 6 */

#define BRIGHTNESS              0x55 /* Brightness Control  */
#define CONTRAST                0x56 /* Contrast Control    */
#define CONTRAST_CENTER         0x57 /* Contrast Center     */
#define MTXS                    0x58 /* Matrix Coefficient Sign for coefficient 5 to 0*/
#define LCC1                    0x62 /* Lens Correction Option 1 */
#define LCC2                    0x63 /* Lens Correction Option 2 */
#define LCC3                    0x64 /* Lens Correction Option 3 */
#define LCC4                    0x65 /* Lens Correction Option 4 */
#define LCC5                    0x66 /* Lens Correction Control */
#define LCC5_LC_EN              0x01 /* Lens Correction Enable */
#define LCC5_LC_CTRL            0x04 /* Lens correction control select */

#define MANU                    0x67 /* Manual U Value (effective only when register TSLB[4] is high) */
#define MANV                    0x68 /* Manual V Value (effective only when register TSLB[4] is high) */

#define GFIX                    0x69 /* Fix Gain Control            */
#define GGAIN                   0x6A /* G Channel AWB Gain          */

#define DBLV                    0x6B /* PLL control                 */
#define DBLV_PLL_BYPASS         0x00 /* Bypass PLL                  */
#define DBLV_PLL_4x             0x40 /* Input clock 4x              */
#define DBLV_PLL_6x             0x80 /* Input clock 8x              */
#define DBLV_PLL_8x             0xC0 /* Input clock 16x             */
#define DBLV_REG_BYPASS         0x10 /* Bypass internal regulator   */

#define AWBCTR3                 0x6C /* AWB Control 3 */
#define AWBCTR2                 0x6D /* AWB Control 2 */
#define AWBCTR1                 0x6E /* AWB Control 1 */
#define AWBCTR0                 0x6F /* AWB Control 0 */

#define SCALING_XSC             0x70 /* Test Pattern[0] */
#define SCALING_YSC             0x71 /* Test Pattern[1] */
#define SCALING_DCWCTR          0x72 /* DCW control parameter */

#define SCALING_PCLK_DIV        0x73 /* Clock divider control for DSP.*/
#define SCALING_PCLK_DIV_BYPASS 0x08 /* Bypass clock divider for DSP scale control */
#define SCALING_PCLK_DIV_1      0x00 /* Divided by 1     */
#define SCALING_PCLK_DIV_2      0x01 /* Divided by 2     */
#define SCALING_PCLK_DIV_4      0x02 /* Divided by 4     */
#define SCALING_PCLK_DIV_8      0x03 /* Divided by 8     */
#define SCALING_PCLK_DIV_16     0x04 /* Divided by 16    */

#define REG74                   0x74 /* REG74 */
#define REG74_DGAIN_EN          0x10 /* Digital gain control by REG74[1:0] */
#define REG74_DGAIN_BYPASS      0x00 /* Bypass */
#define REG74_DGAIN_1X          0x01 /* 1x */
#define REG74_DGAIN_2X          0x02 /* 2x */
#define REG74_DGAIN_4X          0x03 /* 4x */

#define REG75                   0x75 /* REG75 */
#define REG75_EDGE_LOWER        0x1F /* Edge enhancement lower limit */

#define REG76                   0x76 /* REG76 */
#define REG76_WHITE_PIX_CORR    0x80 /* White pixel correction enable */
#define REG76_BLACK_PIX_CORR    0x40 /* Black pixel correction enable */
#define REG76_EDGE_HIGHER       0x1F /* Edge enhancement higher limit */

#define REG77                   0x77 /* Offset, de-noise range control */

#define SLOP                    0x7A /* Gamma Curve Highest Segment Slope                          */
#define GAM1                    0x7B /* Gamma Curve 1st Segment Input End Point 0x04 Output Value  */
#define GAM2                    0x7C /* Gamma Curve 2nd Segment Input End Point 0x08 Output Value  */
#define GAM3                    0x7D /* Gamma Curve 3rd Segment Input End Point 0x10 Output Value  */
#define GAM4                    0x7E /* Gamma Curve 4th Segment Input End Point 0x20 Output Value  */
#define GAM5                    0x7F /* Gamma Curve 5th Segment Input End Point 0x28 Output Value  */
#define GAM6                    0x80 /* Gamma Curve 6th Segment Input End Point 0x30 Output Value  */
#define GAM7                    0x81 /* Gamma Curve 7th Segment Input End Point 0x38 Output Value  */
#define GAM8                    0x82 /* Gamma Curve 8th Segment Input End Point 0x40 Output Value  */
#define GAM9                    0x83 /* Gamma Curve 9th Segment Input Enpd Point 0x48 Output Value */
#define GAM10                   0x84 /* Gamma Curve 10th Segment Input End Point 0x50 Output Value */
#define GAM11                   0x85 /* Gamma Curve 11th Segment Input End Point 0x60 Output Value */
#define GAM12                   0x86 /* Gamma Curve 12th Segment Input End Point 0x70 Output Value */
#define GAM13                   0x87 /* Gamma Curve 13th Segment Input End Point 0x90 Output Value */
#define GAM14                   0x88 /* Gamma Curve 14th Segment Input End Point 0xB0 Output Value */
#define GAM15                   0x89 /* Gamma Curve 15th Segment Input End Point 0xD0 Output Value */

#define RGB444                  0x8C /* REG444 */
#define RGB444_RGB444_EN        0x02 /* RGB444 enable, effective only when COM15[4] is high */
#define RGB444_RGB444_FMT       0x01 /* RGB444 word format. 0 = xR GB 1 = RG Bx */

#define DM_LNL                  0x92 /* Dummy Line low 8 bits    */
#define DM_LNH                  0x93 /* Dummy Line high 8 bits   */
#define LCC6                    0x94 /* RW Lens Correction Option 6 (effective only when LCC5[2] is high) */
#define LCC7                    0x95 /* RW Lens Correction Option 7 (effective only when LCC5[2] is high) */

#define BD50ST                  0x9D /* Hz Banding Filter Value (effective only when COM8[5] is high and COM11[3] is high) */
#define BD60ST                  0x9E /* Hz Banding Filter Value (effective only when COM8[5] is high and COM11[3] is low)  */
#define HAECC1                  0x9F /* Histogram-based AEC/AGC Control 1 */
#define HAECC2                  0xA0 /* Histogram-based AEC/AGC Control 2 */

#define SCALING_PCLK            0xA2 /* Pixel Clock Delay */
#define SCALING_PCLK_DELAY      0x7F /* Scaling output delay */

#define NT_CTRL                 0xA4 /* Auto frame rate adjustment control */
#define NT_CTRL_AFR_HALF        0x08 /* Reduce frame rate by half */
#define NT_CTRL_AFR_PT_2X       0x00 /* Insert dummy row at 2x gain */
#define NT_CTRL_AFR_PT_4X       0x01 /* Insert dummy row at 4x gain */
#define NT_CTRL_AFR_PT_8X       0x02 /* Insert dummy row at 8x gain */

#define BD50MAX                 0xA5 /* 50Hz Banding Step Limit             */
#define HAECC3                  0xA6 /* Histogram-based AEC/AGC Control 3   */
#define HAECC4                  0xA7 /* Histogram-based AEC/AGC Control 4   */
#define HAECC5                  0xA8 /* Histogram-based AEC/AGC Control 5   */
#define HAECC6                  0xA9 /* Histogram-based AEC/AGC Control 6   */

#define HAECC7                  0xAA /* AEC algorithm selection             */
#define HAECC7_AEC_AVG          0x00 /* Average-based AEC algorithm         */
#define HAECC7_AEC_HIST         0x80 /* Histogram-based AEC algorithm       */

#define BD60MAX                 0xAB /* 60Hz Banding Step Limit */

#define STR_OPT                 0xAC /* Strobe Control */
#define STR_OPT_EN              0x80 /* Strobe Enable */
#define STR_OPT_CTRL            0x40 /* R/G/B gain controlled by STR_R (0xAD) STR_G (0xAE) STR_B (0xAF) for LED output frame */
#define STR_OPT_XENON_1ROW      0x00 /* Xenon mode option 1 row  */
#define STR_OPT_XENON_2ROW      0x10 /* Xenon mode option 2 rows */
#define STR_OPT_XENON_3ROW      0x20 /* Xenon mode option 3 rows */
#define STR_OPT_XENON_4ROW      0x30 /* Xenon mode option 4 rows */
#define STR_OPT_MODE_XENON      0x00 /* Mode select */
#define STR_OPT_MODE_LED1       0x01 /* Mode select */
#define STR_OPT_MODE_LED2       0x02 /* Mode select */

#define STR_R                   0xAD /* R Gain for LED Output Frame */
#define STR_G                   0xAE /* G Gain for LED Output Frame */
#define STR_B                   0xAF /* B Gain for LED Output Frame */

#define ABLC1                   0xB1 /* ABLC Control */
#define ABLC1_EN                0x04 /* Enable ABLC function */

#define THL_ST                  0xB3 /* ABLC Target */

#define AD_CHB                  0xBE /* Blue Channel Black Level Compensation */  
#define AD_CHR                  0xBF /* Red Channel Black Level Compensation */  
#define AD_CHGB                 0xC0 /* Gb Channel Black Level Compensation */  
#define AD_CHGR                 0xC1 /* Gr Channel Black Level Compensation */  
#define SATCTR                  0xC9 /* UV Saturation Control */

/* ov7675 definitions */
#define ov7675_PROD_ID 0x76
#define ov7675_MVFP_HFLIP 0x20
#define ov7675_MVFP_VFLIP 0x10

/* Initialization register structure */
static const struct video_reg8 ov7675_default_regs[] = {
    // OV7670 reference registers
    { CLKRC,            CLKRC_PRESCALER_BYPASS | OMV_OV7670_CLKRC },
    { TSLB,             0x04 },
    { COM7,             0x00 },
    { HSTART,           0x13 },
    { HSTOP,            0x01 },
    { HREF,             0xb6 },
    { VSTART,           0x02 },
    { VSTOP,            0x7a },
    { VREF,             0x0a },

    { COM3,             0x00 },
    { COM14,            0x00 },
    { SCALING_XSC,      0x3a },
    { SCALING_YSC,      0x35 },
    { SCALING_DCWCTR,   0x11 },
    { SCALING_PCLK_DIV, 0xf0 },
    { SCALING_PCLK,     0x02 },
    { COM10,            COM10_VSYNC_NEG},

    /* Gamma curve values */
    { SLOP,             0x20 },
    { GAM1,             0x10 },
    { GAM2,             0x1e },
    { GAM3,             0x35 },
    { GAM4,             0x5a },
    { GAM5,             0x69 },
    { GAM6,             0x76 },
    { GAM7,             0x80 },
    { GAM8,             0x88 },
    { GAM9,             0x8f },
    { GAM10,            0x96 },
    { GAM11,            0xa3 },
    { GAM12,            0xaf },
    { GAM13,            0xc4 },
    { GAM14,            0xd7 },
    { GAM15,            0xe8 },

    /* AGC and AEC parameters */
    { COM8,             COM8_FAST_AEC | COM8_AEC_STEP | COM8_BANDF_EN },
    { GAIN,             0x00 },
    { AECH,             0x00 },
    { COM4,             0x40 },
    { COM9,             0x18 },
    { BD50MAX,          0x05 },
    { BD60MAX,          0x07 },
    { AEW,              0x95 },
    { AEB,              0x33 },
    { VPT,              0xe3 },
    { HAECC1,           0x78 },
    { HAECC2,           0x68 },
    { 0xa1,             0x03 },
    { HAECC3,           0xd8 },
    { HAECC4,           0xd8 },
    { HAECC5,           0xf0 },
    { HAECC6,           0x90 },
    { HAECC7,           0x94 },
    { COM8,             COM8_FAST_AEC | COM8_AEC_STEP | COM8_BANDF_EN | COM8_AGC_EN | COM8_AEC_EN },

    /* Almost all of these are magic "reserved" values.  */
    { COM5,             0x61 },
    { COM6,             0x4b },
    { 0x16,             0x02 },
    { MVFP,             0x07 },
    { 0x21,             0x02 },
    { 0x22,             0x91 },
    { 0x29,             0x07 },
    { 0x33,             0x0b },
    { 0x35,             0x0b },
    { 0x37,             0x1d },
    { 0x38,             0x71 },
    { 0x39,             0x2a },
    { COM12,            0x78 },
    { 0x4d,             0x40 },
    { 0x4e,             0x20 },
    { GFIX,             0x00 },
    { 0x6b,             0x00 }, // PLL Bypass
    { 0x74,             0x10 },
    { 0x8d,             0x4f },
    { 0x8e,             0x00 },
    { 0x8f,             0x00 },
    { 0x90,             0x00 },
    { 0x91,             0x00 },
    { 0x96,             0x00 },
    { 0x9a,             0x00 },
    { 0xb0,             0x84 },
    { 0xb1,             0x0c },
    { 0xb2,             0x0e },
    { 0xb3,             0x82 },
    { 0xb8,             0x0a },
    { 0x43,             0x0a },
    { 0x44,             0xf0 },
    { 0x45,             0x34 },
    { 0x46,             0x58 },
    { 0x47,             0x28 },
    { 0x48,             0x3a },
    { 0x59,             0x88 },
    { 0x5a,             0x88 },
    { 0x5b,             0x44 },
    { 0x5c,             0x67 },
    { 0x5d,             0x49 },
    { 0x5e,             0x0e },
    { 0x6c,             0x0a },
    { 0x6d,             0x55 },
    { 0x6e,             0x11 },
    { 0x6f,             0x9e },
    { 0x6a,             0x40 },
    { BLUE,             0x40 },
    { RED,              0x60 },
    { COM8,             COM8_FAST_AEC | COM8_AEC_STEP | COM8_BANDF_EN | COM8_AGC_EN | COM8_AEC_EN | COM8_AWB_EN },

    /* Matrix coefficients */
    { MTX1,             0x80 },
    { MTX2,             0x80 },
    { MTX3,             0x00 },
    { MTX4,             0x22 },
    { MTX5,             0x5e },
    { MTX6,             0x80 },
    { MTXS,             0x9e },

    { COM16,            COM16_AWB_GAIN_EN },
    { EDGE,             0x00 },
    { 0x75,             0x05 },
    { 0x76,             0xe1 },
    { 0x4c,             0x00 },
    { 0x77,             0x01 },
    { COM13,            0xc3 },
    { 0x4b,             0x09 },
    { 0xc9,             0x60 },
    { COM16,            0x38 },
    { 0x56,             0x40 },

    { 0x34,             0x11 },
    { COM11,            COM11_AEC_BANDF | COM11_HZAUTO_EN },
    { 0xa4,             0x88 },
    { 0x96,             0x00 },
    { 0x97,             0x30 },
    { 0x98,             0x20 },
    { 0x99,             0x30 },
    { 0x9a,             0x84 },
    { 0x9b,             0x29 },
    { 0x9c,             0x03 },
    { 0x9d,             0x4c },
    { 0x9e,             0x3f },
    { 0x78,             0x04 },
    { 0x79,             0x01 },
    { 0xc8,             0xf0 },
    { 0x79,             0x0f },
    { 0xc8,             0x00 },
    { 0x79,             0x10 },
    { 0xc8,             0x7e },
    { 0x79,             0x0a },
    { 0xc8,             0x80 },
    { 0x79,             0x0b },
    { 0xc8,             0x01 },
    { 0x79,             0x0c },
    { 0xc8,             0x0f },
    { 0x79,             0x0d },
    { 0xc8,             0x20 },
    { 0x79,             0x09 },
    { 0xc8,             0x80 },
    { 0x79,             0x02 },
    { 0xc8,             0xc0 },
    { 0x79,             0x03 },
    { 0xc8,             0x40 },
    { 0x79,             0x05 },
    { 0xc8,             0x30 },
    { 0x79,             0x26 },
    
    { COM7,         COM7_RGB_FMT },        /* Selects RGB mode */
    { RGB444,       0x00 },                /* No RGB444 please */
    { COM1,         0x00 },                /* CCIR601 */
    { COM15,        COM15_OUT_00_FF | COM15_FMT_RGB565},
    { COM9,         0x38 },                /* 16x gain ceiling; 0x8 is reserved bit */
    { 0x4f,         0xb3 },                /* "matrix coefficient 1" */
    { 0x50,         0xb3 },                /* "matrix coefficient 2" */
    { 0x51,         0x00 },                /* "matrix Coefficient 3" */
    { 0x52,         0x3d },                /* "matrix coefficient 4" */
    { 0x53,         0xa7 },                /* "matrix coefficient 5" */
    { 0x54,         0xe4 },                /* "matrix coefficient 6" */
    { COM13,        COM13_GAMMA_EN | COM13_UVSAT_AUTO },
    
    { COM3,         COM3_DCW_EN },
    { COM14,        0x11 },      // Divide by 2
    { 0x72,         0x22 },      // This has no effect on OV7675
    { 0x73,         0xf2 },      // This has no effect on OV7675
    { HSTART,       0x15 },
    { HSTOP,        0x03 },
    { HREF,         0xC0 },
    { VSTART,       0x03 },
    { VSTOP,        0x7B },
    { VREF,         0xF0 },
};

static const struct video_reg8 ov7675_rgb565_regs[] = {
    { COM7,         COM7_RGB_FMT },        /* Selects RGB mode */
    { RGB444,       0x00 },                /* No RGB444 please */
    { COM1,         0x00 },                /* CCIR601 */
    { COM15,        COM15_OUT_00_FF | COM15_FMT_RGB565},
    { COM9,         0x38 },                /* 16x gain ceiling; 0x8 is reserved bit */
    { 0x4f,         0xb3 },                /* "matrix coefficient 1" */
    { 0x50,         0xb3 },                /* "matrix coefficient 2" */
    { 0x51,         0x00 },                /* "matrix Coefficient 3" */
    { 0x52,         0x3d },                /* "matrix coefficient 4" */
    { 0x53,         0xa7 },                /* "matrix coefficient 5" */
    { 0x54,         0xe4 },                /* "matrix coefficient 6" */
    { COM13,        COM13_GAMMA_EN | COM13_UVSAT_AUTO },
};

static const struct video_reg8 ov7675_vga_regs[] = {
    { COM3,         0x00 },
    { COM14,        0x00 },
    { 0x72,         0x11 },     // downsample by 4
    { 0x73,         0xf0 },     // divide by 4
    { HSTART,       0x12 },
    { HSTOP,        0x00 },
    { HREF,         0xb6 },
    { VSTART,       0x02 },
    { VSTOP,        0x7a },
    { VREF,         0x00 },
};

static const struct video_reg8 ov7675_qvga_regs[] = {
    { COM3,         COM3_DCW_EN },
    { COM14,        0x11 },      // Divide by 2
    { 0x72,         0x22 },      // This has no effect on OV7675
    { 0x73,         0xf2 },      // This has no effect on OV7675
    { HSTART,       0x15 },
    { HSTOP,        0x03 },
    { HREF,         0xC0 },
    { VSTART,       0x03 },
    { VSTOP,        0x7B },
    { VREF,         0xF0 },
};

static const struct video_reg8 ov7675_qqvga_regs[] = {
    { COM3,         COM3_DCW_EN },
    { COM14,        0x11 },     // Divide by 2
    { 0x72,         0x22 },     // This has no effect on OV7675
    { 0x73,         0xf2 },     // This has no effect on OV7675
    { HSTART,       0x16 },
    { HSTOP,        0x04 },
    { HREF,         0xa4 },
    { VSTART,       0x22 },
    { VSTOP,        0x7a },
    { VREF,         0xfa },
};

// TODO: These registers probably need to be fixed too.
static const struct video_reg8 ov7675_yuv422_regs[] = {
    { COM7,         0x00 },             /* Selects YUV mode */
    { RGB444,       0x00 },             /* No RGB444 please */
    { COM1,         0x00 },             /* CCIR601 */
    { COM15,        COM15_OUT_00_FF },
    { COM9,         0x48 },             /* 32x gain ceiling; 0x8 is reserved bit */
    { 0x4f,         0x80 },             /* "matrix coefficient 1" */
    { 0x50,         0x80 },             /* "matrix coefficient 2" */
    { 0x51,         0x00 },             /* vb */
    { 0x52,         0x22 },             /* "matrix coefficient 4" */
    { 0x53,         0x5e },             /* "matrix coefficient 5" */
    { 0x54,         0x80 },             /* "matrix coefficient 6" */
    { COM13,        COM13_GAMMA_EN | COM13_UVSAT_AUTO },
};


#define ov7675_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	ov7675_VIDEO_FORMAT_CAP(160, 240, VIDEO_PIX_FMT_RGB565), /* QQVGA  */
	ov7675_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565), /* QVGA  */
	ov7675_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565), /* VGA   */
	ov7675_VIDEO_FORMAT_CAP(160, 240, VIDEO_PIX_FMT_YUYV), /* QQVGA  */
	ov7675_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV), /* QVGA  */
	ov7675_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV), /* VGA   */
	{0}};


static int ov7675_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int ov7675_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov7675_config *config = dev->config;
  struct ov7675_data *data = dev->data;
	int ret;
	uint16_t i = 0U;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Only RGB565 and YUYV supported!");
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

  k_msleep(300);

  // Set RGB Format
  if(fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
    ret = video_write_cci_multiregs8(&config->bus, ov7675_rgb565_regs, ARRAY_SIZE(ov7675_rgb565_regs));
  } else {
    ret = video_write_cci_multiregs8(&config->bus, ov7675_yuv422_regs, ARRAY_SIZE(ov7675_yuv422_regs));
  }
  if (ret < 0) {
    LOG_ERR("Format not set!");
    return ret;
  }
    
  k_msleep(200);
  
	/* Set output resolution */
	while (fmts[i].pixelformat) {
		if (fmts[i].width_min == fmt->width && fmts[i].height_min == fmt->height && fmts[i].pixelformat == fmt->pixelformat) {
			/* Set output format */
			switch (fmts[i].width_min) {
			case 160: /* QQVGA */
        ret = video_write_cci_multiregs8(&config->bus, ov7675_qqvga_regs, ARRAY_SIZE(ov7675_qqvga_regs));
				break;
			case 320: /* QVGA */
        ret = video_write_cci_multiregs8(&config->bus, ov7675_qvga_regs, ARRAY_SIZE(ov7675_qvga_regs));
				break;
			default: /* VGA */
        ret = video_write_cci_multiregs8(&config->bus, ov7675_vga_regs, ARRAY_SIZE(ov7675_vga_regs));
				break;
			}
			if (ret < 0) {
        LOG_ERR("Resolution not set!");
				return ret;
			}
		}
		i++;
	}

  k_msleep(200);

	return 1;
}

static int ov7675_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov7675_data *data = dev->data;

	memcpy(fmt, &data->fmt, sizeof(data->fmt));
	return 0;
}

static int ov7675_init_controls(const struct device *dev)
{
	int ret;
	struct ov7675_data *drv_data = dev->data;
	struct ov7675_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int ov7675_init(const struct device *dev)
{
	const struct ov7675_config *config = dev->config;
  
	int ret;

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = 320,
		.height = 240,
	};

	if (!i2c_is_ready_dt(&config->bus)) {
		/* I2C device is not ready, return */
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_ACTIVE);
    		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
	}
  gpio_pin_set_dt(&config->pwdn, 1);
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Reset camera module */
	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
		/* Reset is active low, has 1ms settling time*/
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 1);
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
	}
#endif

	/*
	 * Read product ID from camera. This camera implements the SCCB,
	 * spec- which *should* be I2C compatible, but in practice does
	 * not seem to respond when I2C repeated start commands are used.
	 * To work around this, use a write then a read to interface with
	 * registers.
	 */
	uint8_t cmd = OV7675_PID;
	uint32_t pid;
  
	ret = video_read_cci_reg(&config->bus, cmd, &pid);

	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}

	if (pid != ov7675_PROD_ID) {
		LOG_ERR("Incorrect product ID: 0x%02X", pid);
		return -ENODEV;
	}

	/* Reset camera registers */
	ret = video_write_cci_reg(&config->bus, COM7, 0x80);
	if (ret < 0) {
		LOG_ERR("Could not reset camera: %d", ret);
		return ret;
	}
	/* Delay after reset */
	k_msleep(5);

  ret = video_write_cci_multiregs8(&config->bus, ov7675_default_regs, ARRAY_SIZE(ov7675_default_regs));
  if (ret < 0) {
		return ret;
	}
  
	ret = ov7675_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}
  
	/* Initialize controls */
	return ov7675_init_controls(dev);
}

static int ov7675_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return 0;
}

static int ov7675_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct ov7675_config *config = dev->config;
	struct ov7675_data *drv_data = dev->data;
	struct ov7675_ctrls *ctrls = &drv_data->ctrls;

	switch (id) {
	case VIDEO_CID_HFLIP:
		return i2c_reg_update_byte_dt(&config->bus, MVFP, ov7675_MVFP_HFLIP,
					      ctrls->hflip.val ? ov7675_MVFP_HFLIP : 0);
	case VIDEO_CID_VFLIP:
		return i2c_reg_update_byte_dt(&config->bus, MVFP, ov7675_MVFP_VFLIP,
					      ctrls->vflip.val ? ov7675_MVFP_VFLIP : 0);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, ov7675_api) = {
	.set_format = ov7675_set_fmt,
	.get_format = ov7675_get_fmt,
	.get_caps = ov7675_get_caps,
	.set_stream = ov7675_set_stream,
	.set_ctrl = ov7675_set_ctrl,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define OV7675_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define OV7675_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define OV7675_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define OV7675_PWDN_GPIO(inst)
#endif

#define OV7675_INIT(inst)                                                                          \
	const struct ov7675_config ov7675_config_##inst = { \
          .bus = I2C_DT_SPEC_INST_GET(inst),      \
					OV7675_RESET_GPIO(inst)                 \
					OV7675_PWDN_GPIO(inst)};        \
	struct ov7675_data ov7675_data_##inst;                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ov7675_init, NULL, &ov7675_data_##inst, &ov7675_config_##inst, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov7675_api);               \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(ov7675_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV7675_INIT)
