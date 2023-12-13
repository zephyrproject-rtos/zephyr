#ifndef __XLNX_ZYNQ_SDHC_H__
#define __XLNX_ZYNQ_SDHC_H__

#include <zephyr/kernel.h>

/* Bit map for command Register */
#define ZYNQ_SDHC_HOST_CMD_RESP_TYPE_LOC    0
#define ZYNQ_SDHC_HOST_CMD_CRC_CHECK_EN_LOC 3
#define ZYNQ_SDHC_HOST_CMD_IDX_CHECK_EN_LOC 4
#define ZYNQ_SDHC_HOST_CMD_DATA_PRESENT_LOC 5
#define ZYNQ_SDHC_HOST_CMD_TYPE_LOC         6
#define ZYNQ_SDHC_HOST_CMD_INDEX_LOC        8

/* Bit map for Transfer Mode Register */
#define ZYNQ_SDHC_HOST_XFER_DMA_EN_LOC          0
#define ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_LOC    1
#define ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_LOC     2
#define ZYNQ_SDHC_HOST_XFER_DATA_DIR_LOC        4
#define ZYNQ_SDHC_HOST_XFER_MULTI_BLOCK_SEL_LOC 5

#define ZYNQ_SDHC_HOST_XFER_DMA_EN_MASK          0x01
#define ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_MASK    0x01
#define ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_MASK     0x03
#define ZYNQ_SDHC_HOST_XFER_DATA_DIR_MASK        0x01
#define ZYNQ_SDHC_HOST_XFER_MULTI_BLOCK_SEL_MASK 0x01

/* Bit map for Block Size and GAP Register */
#define ZYNQ_SDHC_HOST_BLOCK_SIZE_LOC    0
#define ZYNQ_SDHC_HOST_BLOCK_SIZE_MASK   0xFFF
#define ZYNQ_SDHC_HOST_DMA_BUF_SIZE_LOC  12
#define ZYNQ_SDHC_HOST_DMA_BUF_SIZE_MASK 0x07
#define ZYNQ_SDHC_HOST_BLOCK_GAP_LOC     3
#define ZYNQ_SDHC_HOST_BLOCK_GAP_MASK    0x01

#define ZYNQ_SDHC_HOST_ADMA_BUFF_ADD_LOC   32
#define ZYNQ_SDHC_HOST_ADMA_BUFF_LEN_LOC   16
#define ZYNQ_SDHC_HOST_ADMA_BUFF_LINK_NEXT (0x3 << 4)
#define ZYNQ_SDHC_HOST_ADMA_BUFF_LINK_LAST (0x2 << 4)
#define ZYNQ_SDHC_HOST_ADMA_INTR_EN        BIT(2)
#define ZYNQ_SDHC_HOST_ADMA_BUFF_LAST      BIT(1)
#define ZYNQ_SDHC_HOST_ADMA_BUFF_VALID     BIT(0)

/* Bit Map and length details for Clock Control Register */
#define ZYNQ_SDHC_HOST_CLK_SDCLCK_FREQ_SEL_LOC       8
#define ZYNQ_SDHC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_LOC 6

#define ZYNQ_SDHC_HOST_CLK_SDCLCK_FREQ_SEL_MASK       0xFF
#define ZYNQ_SDHC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_MASK 0x03

/* Bit Map for Host Control 1 Register */
#define ZYNQ_SDHC_HOST_CTRL1_DAT_WIDTH_LOC     1
#define ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_LOC       3
#define ZYNQ_SDHC_HOST_CTRL1_EXT_DAT_WIDTH_LOC 5

#define ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_MASK       0x03
#define ZYNQ_SDHC_HOST_CTRL1_EXT_DAT_WIDTH_MASK 0x01
#define ZYNQ_SDHC_HOST_CTRL1_DAT_WIDTH_MASK     0x01

/** Constants Software Reset register */
#define ZYNQ_SDHC_HOST_SW_RESET_REG_ALL  BIT(0)
#define ZYNQ_SDHC_HOST_SW_RESET_REG_CMD  BIT(1)
#define ZYNQ_SDHC_HOST_SW_RESET_REG_DATA BIT(2)

#define ZYNQ_SDHC_HOST_RESPONSE_SIZE      4
#define ZYNQ_SDHC_HOST_OCR_BUSY_BIT       BIT(31)
#define ZYNQ_SDHC_HOST_OCR_CAPACITY_MASK  0x40000000U
#define ZYNQ_SDHC_HOST_DUAL_VOLTAGE_RANGE 0x40FF8080U
#define ZYNQ_SDHC_HOST_BLOCK_SIZE         512

#define ZYNQ_SDHC_HOST_RCA_SHIFT                16
#define ZYNQ_SDHC_HOST_EXTCSD_SEC_COUNT         53
#define ZYNQ_SDHC_HOST_EXTCSD_GENERIC_CMD6_TIME 62
#define ZYNQ_SDHC_HOST_EXTCSD_BUS_WIDTH_ADDR    0xB7
#define ZYNQ_SDHC_HOST_EXTCSD_HS_TIMING_ADDR    0xB9
#define ZYNQ_SDHC_HOST_BUS_SPEED_HIGHSPEED      1

#define ZYNQ_SDHC_HOST_CMD_COMPLETE_RETRY 10000
#define ZYNQ_SDHC_HOST_XFR_COMPLETE_RETRY 2000000

#define ZYNQ_SDHC_HOST_CMD1_RETRY_TIMEOUT 1000
#define ZYNQ_SDHC_HOST_CMD6_TIMEOUT_MULT  10

#define ZYNQ_SDHC_HOST_NORMAL_INTR_MASK     0x3f
#define ZYNQ_SDHC_HOST_ERROR_INTR_MASK      0x13ff
#define ZYNQ_SDHC_HOST_NORMAL_INTR_MASK_CLR 0x60ff

#define ZYNQ_SDHC_HOST_POWER_CTRL_SD_BUS_POWER    0x1
#define ZYNQ_SDHC_HOST_POWER_CTRL_SD_BUS_VOLT_SEL 0x5

#define ZYNQ_SDHC_HOST_UHSMODE_SDR12  0x0
#define ZYNQ_SDHC_HOST_UHSMODE_SDR25  0x1
#define ZYNQ_SDHC_HOST_UHSMODE_SDR50  0x2
#define ZYNQ_SDHC_HOST_UHSMODE_SDR104 0x3
#define ZYNQ_SDHC_HOST_UHSMODE_DDR50  0x4
#define ZYNQ_SDHC_HOST_UHSMODE_HS400  0x5

#define ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_EN       1
#define ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_LOC      3
#define ZYNQ_SDHC_HOST_CTRL2_UHS_MODE_SEL_LOC  0
#define ZYNQ_SDHC_HOST_CTRL2_UHS_MODE_SEL_MASK 0x07

/* Event/command status */
#define ZYNQ_SDHC_HOST_CMD_COMPLETE   BIT(0)
#define ZYNQ_SDHC_HOST_XFER_COMPLETE  BIT(1)
#define ZYNQ_SDHC_HOST_BLOCK_GAP_INTR BIT(2)
#define ZYNQ_SDHC_HOST_DMA_INTR       BIT(3)
#define ZYNQ_SDHC_HOST_BUF_WR_READY   BIT(4)
#define ZYNQ_SDHC_HOST_BUF_RD_READY   BIT(5)

#define ZYNQ_SDHC_HOST_CMD_TIMEOUT_ERR  BIT(0)
#define ZYNQ_SDHC_HOST_CMD_CRC_ERR      BIT(1)
#define ZYNQ_SDHC_HOST_CMD_END_BIT_ERR  BIT(2)
#define ZYNQ_SDHC_HOST_CMD_IDX_ERR      BIT(3)
#define ZYNQ_SDHC_HOST_DATA_TIMEOUT_ERR BIT(4)
#define ZYNQ_SDHC_HOST_DATA_CRC_ERR     BIT(5)
#define ZYNQ_SDHC_HOST_DATA_END_BIT_ERR BIT(6)
#define ZYNQ_SDHC_HOST_CUR_LMT_ERR      BIT(7)
#define ZYNQ_SDHC_HOST_DMA_TXFR_ERR     BIT(12)
#define ZYNQ_SDHC_HOST_ERR_STATUS       0xFFF

/** PState register bits */
#define ZYNQ_SDHC_HOST_PSTATE_CMD_INHIBIT     BIT(0)
#define ZYNQ_SDHC_HOST_PSTATE_DAT_INHIBIT     BIT(1)
#define ZYNQ_SDHC_HOST_PSTATE_DAT_LINE_ACTIVE BIT(2)

#define ZYNQ_SDHC_HOST_PSTATE_WR_DMA_ACTIVE BIT(8)
#define ZYNQ_SDHC_HOST_PSTATE_RD_DMA_ACTIVE BIT(9)

#define ZYNQ_SDHC_HOST_PSTATE_BUF_READ_EN  BIT(11)
#define ZYNQ_SDHC_HOST_PSTATE_BUF_WRITE_EN BIT(10)

#define ZYNQ_SDHC_HOST_PSTATE_CARD_INSERTED BIT(16)

#define ZYNQ_SDHC_HOST_MAX_TIMEOUT 0xe
#define ZYNQ_SDHC_HOST_MSEC_DELAY  1000

/** Constants for Clock Control register */
#define ZYNQ_SDHC_HOST_INTERNAL_CLOCK_EN     BIT(0)
#define ZYNQ_SDHC_HOST_INTERNAL_CLOCK_STABLE BIT(1)
#define ZYNQ_SDHC_HOST_SD_CLOCK_EN           BIT(2)

#define ZYNQ_SDHC_HOST_TUNING_SUCCESS BIT(7)
#define ZYNQ_SDHC_HOST_START_TUNING   BIT(6)

#define ZYNQ_SDHC_HOST_VOL_3_3_V_SUPPORT BIT(24)
#define ZYNQ_SDHC_HOST_VOL_3_3_V_SELECT  (7 << 1)
#define ZYNQ_SDHC_HOST_VOL_3_0_V_SUPPORT BIT(25)
#define ZYNQ_SDHC_HOST_VOL_3_0_V_SELECT  (6 << 1)
#define ZYNQ_SDHC_HOST_VOL_1_8_V_SUPPORT BIT(26)
#define ZYNQ_SDHC_HOST_VOL_1_8_V_SELECT  (5 << 1)

#define ZYNQ_SDHC_HOST_CMD_WAIT_TIMEOUT_US    3000
#define ZYNQ_SDHC_HOST_CMD_CMPLETE_TIMEOUT_US 9000
#define ZYNQ_SDHC_HOST_XFR_CMPLETE_TIMEOUT_US 1000
#define ZYNQ_SDHC_HOST_SDMA_BOUNDARY          0x0
#define ZYNQ_SDHC_HOST_RCA_ADDRESS            0x2

#define ZYNQ_SDHC_HC_SPEC_V3 0x0002U /**< HC spec version 3 */
#define ZYNQ_SDHC_HC_SPEC_V2 0x0001U /**< HC spec version 2 */
#define ZYNQ_SDHC_HC_SPEC_V1 0x0000U /**< HC spec version 1 */

#define ERR_INTR_STATUS_EVENT(reg_bits) ((reg_bits) << 16)

#define GET_BITS(reg_name, start, width) ((reg_name) & (((1 << (width)) - 1) << (start)))

#define SET_BITS(reg, pos, bit_width, val)                                                         \
	reg &= ~((bit_width) << (pos));                                                            \
	reg |= (((val) & (bit_width)) << (pos))

#define ADDRESS_32BIT_MASK 0xFFFFFFFF

#define XSDPS_HC_VENDOR_VER    0xFF00U /**< Vendor Specification version mask */
#define XSDPS_HC_SPEC_VER_MASK 0x00FFU /**< Host Specification version mask */

#define XSDPS_SDMA_SYS_ADDR_OFFSET 0x00U /**< SDMA System Address Register */
#define XSDPS_SDMA_SYS_ADDR_LO_OFFSET                                                              \
	XSDPS_SDMA_SYS_ADDR_OFFSET          /**< SDMA System Address Low Register */
#define XSDPS_ARGMT2_LO_OFFSET        0x00U /**< Argument2 Low Register */
#define XSDPS_SDMA_SYS_ADDR_HI_OFFSET 0x02U /**< SDMA System Address High Register */
#define XSDPS_ARGMT2_HI_OFFSET        0x02U /**< Argument2 High Register */

#define XSDPS_BLK_SIZE_OFFSET  0x04U              /**< Block Size Register */
#define XSDPS_BLK_CNT_OFFSET   0x06U              /**< Block Count Register */
#define XSDPS_ARGMT_OFFSET     0x08U              /**< Argument Register */
#define XSDPS_ARGMT1_LO_OFFSET XSDPS_ARGMT_OFFSET /**< Argument1 Register */
#define XSDPS_ARGMT1_HI_OFFSET 0x0AU              /**< Argument1 Register */

#define XSDPS_XFER_MODE_OFFSET        0x0CU /**< Transfer Mode Register */
#define XSDPS_CMD_OFFSET              0x0EU /**< Command Register */
#define XSDPS_RESP0_OFFSET            0x10U /**< Response0 Register */
#define XSDPS_RESP1_OFFSET            0x14U /**< Response1 Register */
#define XSDPS_RESP2_OFFSET            0x18U /**< Response2 Register */
#define XSDPS_RESP3_OFFSET            0x1CU /**< Response3 Register */
#define XSDPS_BUF_DAT_PORT_OFFSET     0x20U /**< Buffer Data Port */
#define XSDPS_PRES_STATE_OFFSET       0x24U /**< Present State */
#define XSDPS_HOST_CTRL1_OFFSET       0x28U /**< Host Control 1 */
#define XSDPS_POWER_CTRL_OFFSET       0x29U /**< Power Control */
#define XSDPS_BLK_GAP_CTRL_OFFSET     0x2AU /**< Block Gap Control */
#define XSDPS_WAKE_UP_CTRL_OFFSET     0x2BU /**< Wake Up Control */
#define XSDPS_CLK_CTRL_OFFSET         0x2CU /**< Clock Control */
#define XSDPS_TIMEOUT_CTRL_OFFSET     0x2EU /**< Timeout Control */
#define XSDPS_SW_RST_OFFSET           0x2FU /**< Software Reset */
#define XSDPS_NORM_INTR_STS_OFFSET    0x30U /**< Normal Interrupt Status Register */
#define XSDPS_ERR_INTR_STS_OFFSET     0x32U /**< Error Interrupt Status Register */
#define XSDPS_NORM_INTR_STS_EN_OFFSET 0x34U /**< Normal Interrupt Status Enable Register */
#define XSDPS_ERR_INTR_STS_EN_OFFSET  0x36U /**< Error Interrupt Status Enable Register */
#define XSDPS_NORM_INTR_SIG_EN_OFFSET 0x38U /**< Normal Interrupt Signal Enable Register */
#define XSDPS_ERR_INTR_SIG_EN_OFFSET  0x3AU /**< Error Interrupt Signal Enable Register */

#define XSDPS_AUTO_CMD12_ERR_STS_OFFSET 0x3CU /**< Auto CMD12 Error Status Register */
#define XSDPS_HOST_CTRL2_OFFSET         0x3EU /**< Host Control2 Register */
#define XSDPS_CAPS_OFFSET               0x40U /**< Capabilities Register */
#define XSDPS_CAPS_EXT_OFFSET           0x44U /**< Capabilities Extended */
#define XSDPS_MAX_CURR_CAPS_OFFSET      0x48U /**< Maximum Current Capabilities Register */
#define XSDPS_MAX_CURR_CAPS_EXT_OFFSET  0x4CU /**< Maximum Current Capabilities Ext Register */
#define XSDPS_FE_ERR_INT_STS_OFFSET     0x52U /**< Force Event for Error Interrupt Status */
#define XSDPS_FE_AUTO_CMD12_EIS_OFFSET  0x50U /**< Auto CM12 Error Interrupt Status Register */
#define XSDPS_ADMA_ERR_STS_OFFSET       0x54U /**< ADMA Error Status Register */
#define XSDPS_ADMA_SAR_OFFSET           0x58U /**< ADMA System Address Register */
#define XSDPS_ADMA_SAR_EXT_OFFSET       0x5CU /**< ADMA System Address Extended Register */
#define XSDPS_PRE_VAL_1_OFFSET          0x60U /**< Preset Value Register */
#define XSDPS_PRE_VAL_2_OFFSET          0x64U /**< Preset Value Register */
#define XSDPS_PRE_VAL_3_OFFSET          0x68U /**< Preset Value Register */
#define XSDPS_PRE_VAL_4_OFFSET          0x6CU /**< Preset Value Register */
#define XSDPS_BOOT_TOUT_CTRL_OFFSET     0x70U /**< Boot timeout control register */

#define XSDPS_SHARED_BUS_CTRL_OFFSET 0xE0U /**< Shared Bus Control Register */
#define XSDPS_SLOT_INTR_STS_OFFSET   0xFCU /**< Slot Interrupt Status Register */
#define XSDPS_HOST_CTRL_VER_OFFSET   0xFEU /**< Host Controller Version Register */

/** @} */

/** @name Control Register - Host control, Power control,
 * 			Block Gap control and Wakeup control
 *
 * This register contains bits for various configuration options of
 * the SD host controller. Read/Write apart from the reserved bits.
 * @{
 */

#define XSDPS_HC_LED_MASK          0x00000001U /**< LED Control */
#define XSDPS_HC_WIDTH_MASK        0x00000002U /**< Bus width */
#define XSDPS_HC_BUS_WIDTH_4       0x00000002U
#define XSDPS_HC_SPEED_MASK        0x00000004U /**< High Speed */
#define XSDPS_HC_DMA_MASK          0x00000018U /**< DMA Mode Select */
#define XSDPS_HC_DMA_SDMA_MASK     0x00000000U /**< SDMA Mode */
#define XSDPS_HC_DMA_ADMA1_MASK    0x00000008U /**< ADMA1 Mode */
#define XSDPS_HC_DMA_ADMA2_32_MASK 0x00000010U /**< ADMA2 Mode - 32 bit */
#define XSDPS_HC_DMA_ADMA2_64_MASK 0x00000018U /**< ADMA2 Mode - 64 bit */
#define XSDPS_HC_EXT_BUS_WIDTH     0x00000020U /**< Bus width - 8 bit */
#define XSDPS_HC_CARD_DET_TL_MASK  0x00000040U /**< Card Detect Tst Lvl */
#define XSDPS_HC_CARD_DET_SD_MASK  0x00000080U /**< Card Detect Sig Det */

#define XSDPS_PC_BUS_PWR_MASK      0x00000001U /**< Bus Power Control */
#define XSDPS_PC_BUS_VSEL_MASK     0x0000000EU /**< Bus Voltage Select */
#define XSDPS_PC_BUS_VSEL_3V3_MASK 0x0000000EU /**< Bus Voltage 3.3V */
#define XSDPS_PC_BUS_VSEL_3V0_MASK 0x0000000CU /**< Bus Voltage 3.0V */
#define XSDPS_PC_BUS_VSEL_1V8_MASK 0x0000000AU /**< Bus Voltage 1.8V */
#define XSDPS_PC_EMMC_HW_RST_MASK  0x00000010U /**< HW reset for eMMC */

#define XSDPS_BGC_STP_REQ_MASK     0x00000001U /**< Block Gap Stop Req */
#define XSDPS_BGC_CNT_REQ_MASK     0x00000002U /**< Block Gap Cont Req */
#define XSDPS_BGC_RWC_MASK         0x00000004U /**< Block Gap Rd Wait */
#define XSDPS_BGC_INTR_MASK        0x00000008U /**< Block Gap Intr */
#define XSDPS_BGC_SPI_MODE_MASK    0x00000010U /**< Block Gap SPI Mode */
#define XSDPS_BGC_BOOT_EN_MASK     0x00000020U /**< Block Gap Boot Enb */
#define XSDPS_BGC_ALT_BOOT_EN_MASK 0x00000040U /**< Block Gap Alt BootEn */
#define XSDPS_BGC_BOOT_ACK_MASK    0x00000080U /**< Block Gap Boot Ack */

#define XSDPS_WC_WUP_ON_INTR_MASK  0x00000001U /**< Wakeup Card Intr */
#define XSDPS_WC_WUP_ON_INSRT_MASK 0x00000002U /**< Wakeup Card Insert */
#define XSDPS_WC_WUP_ON_REM_MASK   0x00000004U /**< Wakeup Card Removal */

/** @} */

/** @name Control Register - Clock control, Timeout control & Software reset
 *
 * This register contains bits for configuration options of clock, timeout and
 * software reset.
 * Read/Write except for Inter_Clock_Stable bit (read only) and reserved bits.
 * @{
 */

#define XSDPS_CC_INT_CLK_EN_MASK         0x00000001U /**< INT clk enable */
#define XSDPS_CC_INT_CLK_STABLE_MASK     0x00000002U /**< INT clk stable */
#define XSDPS_CC_SD_CLK_EN_MASK          0x00000004U /**< SD clk enable */
#define XSDPS_CC_SD_CLK_GEN_SEL_MASK     0x00000020U /**< SD clk gen selection */
#define XSDPS_CC_SDCLK_FREQ_SEL_EXT_MASK 0x00000003U /**< SD clk freq sel upper */
#define XSDPS_CC_SDCLK_FREQ_SEL_MASK     0x000000FFU /**< SD clk freq sel */
#define XSDPS_CC_SDCLK_FREQ_D256_MASK    0x00008000U /**< Divider 256 */
#define XSDPS_CC_SDCLK_FREQ_D128_MASK    0x00004000U /**< Divider 128 */
#define XSDPS_CC_SDCLK_FREQ_D64_MASK     0x00002000U /**< Divider 64 */
#define XSDPS_CC_SDCLK_FREQ_D32_MASK     0x00001000U /**< Divider 32 */
#define XSDPS_CC_SDCLK_FREQ_D16_MASK     0x00000800U /**< Divider 16 */
#define XSDPS_CC_SDCLK_FREQ_D8_MASK      0x00000400U /**< Divider 8 */
#define XSDPS_CC_SDCLK_FREQ_D4_MASK      0x00000200U /**< Divider 4 */
#define XSDPS_CC_SDCLK_FREQ_D2_MASK      0x00000100U /**< Divider 2 */
#define XSDPS_CC_SDCLK_FREQ_BASE_MASK    0x00000000U /**< Base clock */
#define XSDPS_CC_MAX_DIV_CNT             256U        /**< Max divider count */
#define XSDPS_CC_EXT_MAX_DIV_CNT         2046U       /**< Max extended divider count */
#define XSDPS_CC_EXT_DIV_SHIFT           6U          /**< Ext divider shift */

#define XSDPS_TC_CNTR_VAL_MASK 0x0000000FU /**< Data timeout counter */

#define XSDPS_SWRST_ALL_MASK      0x00000001U /**< Software Reset All */
#define XSDPS_SWRST_CMD_LINE_MASK 0x00000002U /**< Software reset for CMD line */
#define XSDPS_SWRST_DAT_LINE_MASK 0x00000004U /**< Software reset for DAT line */

#define XSDPS_CC_MAX_NUM_OF_DIV 9U /**< Max number of Clock dividers */
#define XSDPS_CC_DIV_SHIFT      8U /**< Clock Divider shift */

/** @} */

/** @name SD Interrupt Registers
 *
 * <b> Normal and Error Interrupt Status Register </b>
 * This register shows the normal and error interrupt status.
 * Status enable register affects reads of this register.
 * If Signal enable register is set and the corresponding status bit is set,
 * interrupt is generated.
 * Write to clear except
 * Error_interrupt and Card_Interrupt bits - Read only
 *
 * <b> Normal and Error Interrupt Status Enable Register </b>
 * Setting this register bits enables Interrupt status.
 * Read/Write except Fixed_to_0 bit (Read only)
 *
 * <b> Normal and Error Interrupt Signal Enable Register </b>
 * This register is used to select which interrupt status is
 * indicated to the Host System as the interrupt.
 * Read/Write except Fixed_to_0 bit (Read only)
 *
 * All three registers have same bit definitions
 * @{
 */

#define XSDPS_INTR_CC_MASK            0x00000001U /**< Command Complete */
#define XSDPS_INTR_TC_MASK            0x00000002U /**< Transfer Complete */
#define XSDPS_INTR_BGE_MASK           0x00000004U /**< Block Gap Event */
#define XSDPS_INTR_DMA_MASK           0x00000008U /**< DMA Interrupt */
#define XSDPS_INTR_BWR_MASK           0x00000010U /**< Buffer Write Ready */
#define XSDPS_INTR_BRR_MASK           0x00000020U /**< Buffer Read Ready */
#define XSDPS_INTR_CARD_INSRT_MASK    0x00000040U /**< Card Insert */
#define XSDPS_INTR_CARD_REM_MASK      0x00000080U /**< Card Remove */
#define XSDPS_INTR_CARD_MASK          0x00000100U /**< Card Interrupt */
#define XSDPS_INTR_INT_A_MASK         0x00000200U /**< INT A Interrupt */
#define XSDPS_INTR_INT_B_MASK         0x00000400U /**< INT B Interrupt */
#define XSDPS_INTR_INT_C_MASK         0x00000800U /**< INT C Interrupt */
#define XSDPS_INTR_RE_TUNING_MASK     0x00001000U /**< Re-Tuning Interrupt */
#define XSDPS_INTR_BOOT_ACK_RECV_MASK 0x00002000U /**< Boot Ack Recv Interrupt */
#define XSDPS_INTR_BOOT_TERM_MASK     0x00004000U /**< Boot Terminate Interrupt */
#define XSDPS_INTR_ERR_MASK           0x00008000U /**< Error Interrupt */
#define XSDPS_NORM_INTR_ALL_MASK      0x0000FFFFU

#define XSDPS_INTR_ERR_CT_MASK         0x00000001U /**< Command Timeout Error */
#define XSDPS_INTR_ERR_CCRC_MASK       0x00000002U /**< Command CRC Error */
#define XSDPS_INTR_ERR_CEB_MASK        0x00000004U /**< Command End Bit Error */
#define XSDPS_INTR_ERR_CI_MASK         0x00000008U /**< Command Index Error */
#define XSDPS_INTR_ERR_DT_MASK         0x00000010U /**< Data Timeout Error */
#define XSDPS_INTR_ERR_DCRC_MASK       0x00000020U /**< Data CRC Error */
#define XSDPS_INTR_ERR_DEB_MASK        0x00000040U /**< Data End Bit Error */
#define XSDPS_INTR_ERR_CUR_LMT_MASK    0x00000080U /**< Current Limit Error */
#define XSDPS_INTR_ERR_AUTO_CMD12_MASK 0x00000100U /**< Auto CMD12 Error */
#define XSDPS_INTR_ERR_ADMA_MASK       0x00000200U /**< ADMA Error */
#define XSDPS_INTR_ERR_TR_MASK         0x00001000U /**< Tuning Error */
#define XSDPS_INTR_VEND_SPF_ERR_MASK   0x0000E000U /**< Vendor Specific Error */
#define XSDPS_ERROR_INTR_ALL_MASK      0x0000F3FFU /**< Mask for error bits */

#define XSDPS_BLK_SIZE_MASK       0x00000FFFU /**< Transfer Block Size */
#define XSDPS_SDMA_BUFF_SIZE_MASK 0x00007000U /**< Host SDMA Buffer Size */
#define XSDPS_BLK_SIZE_1024       0x400U
#define XSDPS_BLK_SIZE_2048       0x800U
#define XSDPS_BLK_CNT_MASK        0x0000FFFFU

#define XSDPS_TM_DMA_EN_MASK          0x00000001U /**< DMA Enable */
#define XSDPS_TM_BLK_CNT_EN_MASK      0x00000002U /**< Block Count Enable */
#define XSDPS_TM_AUTO_CMD12_EN_MASK   0x00000004U /**< Auto CMD12 Enable */
#define XSDPS_TM_DAT_DIR_SEL_MASK     0x00000010U /**< Data Transfer Direction Select */
#define XSDPS_TM_MUL_SIN_BLK_SEL_MASK 0x00000020U /**< Multi/Single Block Select */

/* Spec 3.0 */
#define XSDPS_CAPS_ASYNC_INTR_MASK 0x20000000U /**< Async Interrupt support */
#define XSDPS_CAPS_SLOT_TYPE_MASK  0xC0000000U /**< Slot Type */
#define XSDPS_CAPS_REM_CARD        0x00000000U /**< Removable Slot */
#define XSDPS_CAPS_EMB_SLOT        0x40000000U /**< Embedded Slot */
#define XSDPS_CAPS_SHR_BUS         0x80000000U /**< Shared Bus Slot */

#define XSDPS_ECAPS_SDR50_MASK         0x00000001U /**< SDR50 Mode support */
#define XSDPS_ECAPS_SDR104_MASK        0x00000002U /**< SDR104 Mode support */
#define XSDPS_ECAPS_DDR50_MASK         0x00000004U /**< DDR50 Mode support */
#define XSDPS_ECAPS_DRV_TYPE_A_MASK    0x00000010U /**< DriverType A support */
#define XSDPS_ECAPS_DRV_TYPE_C_MASK    0x00000020U /**< DriverType C support */
#define XSDPS_ECAPS_DRV_TYPE_D_MASK    0x00000040U /**< DriverType D support */
#define XSDPS_ECAPS_TMR_CNT_MASK       0x00000F00U /**< Timer Count for Re-tuning */
#define XSDPS_ECAPS_USE_TNG_SDR50_MASK 0x00002000U /**< SDR50 Mode needs tuning */
#define XSDPS_ECAPS_RE_TNG_MODES_MASK  0x0000C000U /**< Re-tuning modes support */
#define XSDPS_ECAPS_RE_TNG_MODE1_MASK  0x00000000U /**< Re-tuning mode 1 */
#define XSDPS_ECAPS_RE_TNG_MODE2_MASK  0x00004000U /**< Re-tuning mode 2 */
#define XSDPS_ECAPS_RE_TNG_MODE3_MASK  0x00008000U /**< Re-tuning mode 3 */
#define XSDPS_ECAPS_CLK_MULT_MASK      0x00FF0000U /**< Clock Multiplier Programmable clock mode*/
#define XSDPS_ECAPS_SPI_MODE_MASK      0x01000000U /**< SPI mode */
#define XSDPS_ECAPS_SPI_BLK_MODE_MASK  0x02000000U /**< SPI block mode */

#define ZYNQ_SDHC_HOST_ADMA_ERR_MASK 0x03U /**< ADMA Error Status Mask */

struct __packed zynq_sdhc_reg {
	volatile uint32_t sdma_sysaddr;           /**< SDMA System Address */
	volatile uint16_t block_size;             /**< Block Size */
	volatile uint16_t block_count;            /**< Block Count */
	volatile uint32_t argument;               /**< Argument */
	volatile uint16_t transfer_mode;          /**< Transfer Mode */
	volatile uint16_t cmd;                    /**< Command */
	volatile uint32_t resp_01;                /**< Response Register 0 & 1 */
	volatile uint16_t resp_2;                 /**< Response Register 2*/
	volatile uint16_t resp_3;                 /**< Response Register 3 */
	volatile uint16_t resp_4;                 /**< Response Register 4 */
	volatile uint16_t resp_5;                 /**< Response Register 5 */
	volatile uint16_t resp_6;                 /**< Response Register 6 */
	volatile uint16_t resp_7;                 /**< Response Register 7 */
	volatile uint32_t data_port;              /**< Buffer Data Port */
	volatile uint32_t present_state;          /**< Present State */
	volatile uint8_t host_ctrl1;              /**< Host Control 1 */
	volatile uint8_t power_ctrl;              /**< Power Control */
	volatile uint8_t block_gap_ctrl;          /**< Block Gap Control */
	volatile uint8_t wake_up_ctrl;            /**< Wakeup Control */
	volatile uint16_t clock_ctrl;             /**< Clock Control */
	volatile uint8_t timeout_ctrl;            /**< Timeout Control */
	volatile uint8_t sw_reset;                /**< Software Reset */
	volatile uint16_t normal_int_stat;        /**< Normal Interrupt Status */
	volatile uint16_t err_int_stat;           /**< Error Interrupt Status */
	volatile uint16_t normal_int_stat_en;     /**< Normal Interrupt Status Enable */
	volatile uint16_t err_int_stat_en;        /**< Error Interrupt Status Enable */
	volatile uint16_t normal_int_signal_en;   /**< Normal Interrupt Signal Enable */
	volatile uint16_t err_int_signal_en;      /**< Error Interrupt Signal Enable */
	volatile uint16_t auto_cmd_err_stat;      /**< Auto CMD Error Status */
	volatile uint16_t host_ctrl2;             /**< Host Control 2 */
	volatile uint64_t capabilities;           /**< Capabilities */
	volatile uint64_t max_current_cap;        /**< Max Current Capabilities */
	volatile uint16_t force_err_autocmd_stat; /**< Force Event for Auto CMD Error Status*/
	volatile uint16_t force_err_int_stat;     /**< Force Event for Error Interrupt Status */
	volatile uint8_t adma_err_stat;           /**< ADMA Error Status */
	volatile uint8_t reserved[3];
	volatile uint32_t adma_sys_addr1; /**< ADMA System Address1 */
	volatile uint32_t adma_sys_addr2; /**< ADMA System Address2 */
	volatile uint32_t preset_val_1;   /**< Preset Value 1 */
	volatile uint32_t preset_val_2;   /**< Preset Value 2 */
	volatile uint32_t preset_val_3;   /**< Preset Value 3 */
	volatile uint32_t preset_val_4;   /**< Preset Value 4 */
	volatile uint32_t boot_timeout;   /**< Boot Timeout */
	volatile uint16_t preset_val_8;   /**< Preset Value 8 */
	volatile uint16_t reserved3;
	volatile uint16_t vendor_reg; /**< Vendor Enhanced strobe */
	volatile uint16_t reserved4[57];
	volatile uint32_t reserved5[4];
	volatile uint16_t slot_intr_stat;     /**< Slot Interrupt Status */
	volatile uint16_t host_cntrl_version; /**< Host Controller Version */
} zynq_sdhc_reg_t;

typedef struct adma_attr {
	uint8_t valid: 1;
	uint8_t end: 1;
	uint8_t int_en: 1;
	uint8_t r0: 1;
	uint8_t act: 2;
	uint16_t r1: 10;
} adma_attr_t;
typedef union {
	uint16_t val;
	adma_attr_t attr_bits;
} adma_attr_u;

typedef struct adma_desc {
	adma_attr_u attr;
	uint16_t len;
#if defined(CONFIG_64BIT)
	uint64_t address;
#else
	uint32_t address;
#endif
} __packed adma_desc_t;

enum zynq_sdhc_swrst {
	ZYNQ_SDHC_HOST_SWRST_DATA_LINE = 0,
	ZYNQ_SDHC_HOST_SWRST_CMD_LINE,
	ZYNQ_SDHC_HOST_SWRST_ALL
};

enum zynq_sdhc_cmd_type {
	ZYNQ_SDHC_HOST_CMD_NORMAL = 0,
	ZYNQ_SDHC_HOST_CMD_SUSPEND,
	ZYNQ_SDHC_HOST_CMD_RESUME,
	ZYNQ_SDHC_HOST_CMD_ABORT
};

enum zynq_sdhc_resp_type {
	ZYNQ_SDHC_HOST_RESP_NONE = 0,
	ZYNQ_SDHC_HOST_RESP_LEN_136,
	ZYNQ_SDHC_HOST_RESP_LEN_48,
	ZYNQ_SDHC_HOST_RESP_LEN_48_BUSY,
	ZYNQ_SDHC_HOST_INVAL_HOST_RESP
};

enum zynq_sdhc_slot_type {
	ZYNQ_SDHC_SLOT_SD = 1,
	ZYNQ_SDHC_SLOT_MMC,
	ZYNQ_SDHC_SLOT_SDIO,
	ZYNQ_SDHC_SLOT_SD_COMBO,
	ZYNQ_SDHC_SLOT_EMMC
};

static ALWAYS_INLINE void zynq_sdhc_write8(const struct device *dev, uint32_t offset, uint8_t val)
{
	sys_write8(val, DEVICE_MMIO_GET(dev) + offset);
}
static ALWAYS_INLINE void zynq_sdhc_write16(const struct device *dev, uint32_t offset, uint16_t val)
{
	sys_write16(val, DEVICE_MMIO_GET(dev) + offset);
}
static ALWAYS_INLINE void zynq_sdhc_write32(const struct device *dev, uint32_t offset, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + offset);
}

static ALWAYS_INLINE uint8_t zynq_sdhc_read8(const struct device *dev, uint32_t offset)
{
	// assert offset is 32-bit aligned
	return sys_read8(DEVICE_MMIO_GET(dev) + offset);
}
static ALWAYS_INLINE uint16_t zynq_sdhc_read16(const struct device *dev, uint32_t offset)
{
	// assert offset is 32-bit aligned
	return sys_read16(DEVICE_MMIO_GET(dev) + offset);
}
static ALWAYS_INLINE uint32_t zynq_sdhc_read32(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offset);
}
static ALWAYS_INLINE uint64_t zynq_sdhc_read64(const struct device *dev, uint32_t offset)
{
	return sys_read64(DEVICE_MMIO_GET(dev) + offset);
}

#endif