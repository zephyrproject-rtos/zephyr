/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 SoC definitions.
 *
 * Register layouts and bit fields are taken from the vendor SoC headers
 * (ap/include/soc/bk7258/reg_base.h, .../{uart,gpio,sys}_struct.h,
 * .../{gpio,sys}_reg.h, .../icu_map.h), not guessed.
 */

#ifndef _SOC_BK7258_SOC_H_
#define _SOC_BK7258_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

/*
 * CMSIS device macros. cmsis_core_m.h checks them right after including
 * <soc.h>, so they are this file's responsibility, and this SoC has no CMSIS
 * device header of its own to take them from. The include stays inside the
 * _ASMLANGUAGE guard so the assembly stage does not pull it in.
 */
#include <cmsis_core_m_defaults.h>

#endif /* _ASMLANGUAGE */

/*
 * 1. Memory map
 *
 * SOC_ADDR_OFFSET is 0 in the secure state this image runs in, so the values
 * below are the final addresses.
 */

#define SOC_ITCM_DATA_BASE  0x00000000U /* 16 KB */
#define SOC_DTCM_DATA_BASE  0x20000000U /* 16 KB */
#define SOC_FLASH_DATA_BASE 0x02000000U /* XIP code window (alias address) */
#define SOC_ROM_DATA_BASE   0x06000000U

#define SOC_SRAM0_DATA_BASE 0x28000000U /* 64 KB */
#define SOC_SRAM1_DATA_BASE 0x28010000U /* 64 KB */
#define SOC_SRAM2_DATA_BASE 0x28020000U /* 128 KB */
#define SOC_SRAM3_DATA_BASE 0x28040000U /* 128 KB */
#define SOC_SRAM4_DATA_BASE 0x28060000U /* 128 KB */
#define SOC_SRAM5_DATA_BASE 0x28080000U /* 128 KB */
/* 640 KB of SRAM in total */

#define SOC_PSRAM_DATA_BASE 0x60000000U /* 8 MB */
#define SOC_QSPI0_DATA_BASE 0x64000000U
#define SOC_QSPI1_DATA_BASE 0x68000000U

/*
 * Alias views of the SRAM regions, where the instruction alias of a data
 * address is data address - 0x20000000:
 *   0x08000000 instruction region <-> 0x28000000 data region
 *   0x18000000 instruction region <-> 0x38000000 data region
 */
#define SOC_IRAM_ALIAS_OFFSET 0x20000000U

/*
 * 2. Peripheral register base addresses (SOC_ADDR_OFFSET = 0)
 */

#define SOC_AON_PMU_REG_BASE  0x44000000U
#define SOC_AON_GPIO_REG_BASE 0x44000400U /* one word per pin */
#define SOC_AON_RTC_REG_BASE  0x44000200U
#define SOC_AON_WDT_REG_BASE  0x44000600U
#define SOC_SYS_REG_BASE      0x44010000U /* SYSTEM; clocks at 0x44010020 */
#define SOC_FLASH_REG_BASE    0x44030000U

#define SOC_MBOX0_REG_BASE 0x41000000U
#define SOC_MBOX1_REG_BASE 0x41020000U

#define SOC_TIMER0_REG_BASE 0x44810000U
#define SOC_WDT_REG_BASE    0x44800000U
#define SOC_EFUSE_REG_BASE  0x44880000U
#define SOC_SPI0_REG_BASE   0x44870000U
#define SOC_CKMN_REG_BASE   0x448A0000U

#define SOC_UART0_REG_BASE 0x44820000U /* the console port */
#define SOC_UART1_REG_BASE 0x45830000U
#define SOC_UART2_REG_BASE 0x45840000U

#define SOC_TIMER1_REG_BASE 0x45800000U
#define SOC_I2C0_REG_BASE   0x45850000U
#define SOC_I2C1_REG_BASE   0x45860000U
#define SOC_SPI1_REG_BASE   0x45880000U
#define SOC_SARADC_REG_BASE 0x45890000U
#define SOC_PWM_REG_BASE    0x458A0000U
#define SOC_TRNG_REG_BASE   0x458C0000U
#define SOC_SDIO_REG_BASE   0x458D0000U

#define SOC_USB_REG_BASE   0x46000000U
#define SOC_QSPI0_REG_BASE 0x46040000U
#define SOC_QSPI1_REG_BASE 0x46060000U
#define SOC_ETH_REG_BASE   0x460A0000U

/*
 * 3. GPIO
 *
 * SOC_GPIO_NUM pins, one 32-bit configuration word each, based at
 * SOC_AON_GPIO_REG_BASE.
 *
 * Pin muxing is a two-level mechanism:
 *   level 1: pin configuration word bit 6, gpio_2_func_en, the master switch
 *            that hands the pin to a peripheral
 *   level 2: seven words in the SYSTEM domain, each covering 8 pins with a
 *            4-bit function number per pin, at
 *            GPIO_LL_SYSTEM_REG_BASE = SOC_SYS_REG_BASE + 0x30*4 = 0x440100C0
 */

#define SOC_GPIO_NUM              56U
#define SOC_GPIO_PERI_FUNC_NUM    8U
#define SOC_GPIO_SYSTEM_GROUP_NUM 7U

#define SOC_GPIO_FUNC_MODE_REG_BASE (SOC_SYS_REG_BASE + 0x30U * 4U) /* 0x440100C0 */

/* Configuration word address of pin N */
#define SOC_GPIO_PIN_CFG_ADDR(n) (SOC_AON_GPIO_REG_BASE + 4U * (uint32_t)(n))

/* Address of the function-number register holding pin N */
#define SOC_GPIO_FUNC_MODE_ADDR(n) (SOC_GPIO_FUNC_MODE_REG_BASE + 4U * ((uint32_t)(n) / 8U))

/* Shift of pin N's function number inside that register */
#define SOC_GPIO_FUNC_MODE_SHIFT(n) (4U * ((uint32_t)(n) % 8U))

/* Pin configuration word bits (gpio_struct.h: gpio_hw_t::gpio_num[].cfg) */
#define SOC_GPIO_CFG_INPUT_BIT         BIT(0) /* read-only: current level */
#define SOC_GPIO_CFG_OUTPUT_BIT        BIT(1) /* output level */
#define SOC_GPIO_CFG_INPUT_EN_BIT      BIT(2)
#define SOC_GPIO_CFG_OUTPUT_EN_BIT     BIT(3)
#define SOC_GPIO_CFG_PULL_MODE_BIT     BIT(4) /* 1 = pull-up, 0 = pull-down */
#define SOC_GPIO_CFG_PULL_MODE_EN_BIT  BIT(5) /* defaults to 1 */
#define SOC_GPIO_CFG_2_FUNC_EN_BIT     BIT(6) /* master mux switch */
#define SOC_GPIO_CFG_INPUT_MONITOR_BIT BIT(7)
#define SOC_GPIO_CFG_CAPACITY_SHIFT    8U  /* [9:8] drive strength */
#define SOC_GPIO_CFG_INT_TYPE_SHIFT    10U /* [11:10] */
#define SOC_GPIO_CFG_INT_EN_BIT        BIT(12)
#define SOC_GPIO_CFG_INT_CLEAR_BIT     BIT(13) /* write 1 to clear */

/*
 * Interrupt type encoding (hal_gpio_types.h: gpio_int_type_t), 2 bits:
 *   0 = low level, 1 = high level, 2 = rising edge, 3 = falling edge
 */
#define SOC_GPIO_INT_TYPE_LOW_LEVEL    0U
#define SOC_GPIO_INT_TYPE_HIGH_LEVEL   1U
#define SOC_GPIO_INT_TYPE_RISING_EDGE  2U
#define SOC_GPIO_INT_TYPE_FALLING_EDGE 3U

/* Interrupt status registers, and write 1 to clear (gpio_struct.h: gpio_hw_t) */
#define SOC_GPIO_INT_ST_LO_ADDR (SOC_AON_GPIO_REG_BASE + (0x40U << 2)) /* GPIO0..31  */
#define SOC_GPIO_INT_ST_HI_ADDR (SOC_AON_GPIO_REG_BASE + (0x41U << 2)) /* GPIO32..55 */

/*
 * 4. UART
 *
 * uart_struct.h::uart_hw_t: registers step by 4 bytes, offset = index * 4.
 */

#ifndef _ASMLANGUAGE

typedef struct {
	volatile uint32_t dev_id;           /* 0x00 read-only */
	volatile uint32_t dev_version;      /* 0x04 read-only */
	volatile uint32_t global_ctrl;      /* 0x08 */
	volatile uint32_t dev_status;       /* 0x0C */
	volatile uint32_t config;           /* 0x10 */
	volatile uint32_t fifo_config;      /* 0x14 */
	volatile uint32_t fifo_status;      /* 0x18 */
	volatile uint32_t fifo_port;        /* 0x1C */
	volatile uint32_t int_enable;       /* 0x20 */
	volatile uint32_t int_status;       /* 0x24 */
	volatile uint32_t flow_ctrl_config; /* 0x28 */
	volatile uint32_t wake_config;      /* 0x2C */
} soc_uart_hw_t;

/* --- global_ctrl --- */
#define SOC_UART_GLOBAL_SOFT_RESET_BIT      BIT(0)
#define SOC_UART_GLOBAL_CLK_GATE_BYPASS_BIT BIT(1) /* bypass clock gating: set to 1 */

/* --- config --- */
#define SOC_UART_CFG_TX_ENABLE_BIT   BIT(0)
#define SOC_UART_CFG_RX_ENABLE_BIT   BIT(1)
#define SOC_UART_CFG_DATA_BITS_SHIFT 3U /* [4:3], 3 = 8 bits */
#define SOC_UART_CFG_DATA_BITS_8     3U
#define SOC_UART_CFG_PARITY_EN_BIT   BIT(5)
#define SOC_UART_CFG_PARITY_ODD_BIT  BIT(6) /* 0 = even, 1 = odd */
#define SOC_UART_CFG_STOP_BITS_2_BIT BIT(7) /* 0 = 1 bit, 1 = 2 bits */
#define SOC_UART_CFG_CLK_DIV_SHIFT   8U     /* [23:8] clk_div = uart_clk/baud */

/* --- fifo_config --- */
#define SOC_UART_FIFO_TX_THRESH_MASK     0xFFU
#define SOC_UART_FIFO_RX_THRESH_SHIFT    8U
#define SOC_UART_FIFO_RX_STOP_TIME_SHIFT 16U /* 0=32 1=64 2=128 3=256 */

/* --- fifo_status --- */
#define SOC_UART_FIFO_STATUS_TX_FULL_BIT  BIT(16)
#define SOC_UART_FIFO_STATUS_TX_EMPTY_BIT BIT(17)
#define SOC_UART_FIFO_STATUS_RX_FULL_BIT  BIT(18)
#define SOC_UART_FIFO_STATUS_RX_EMPTY_BIT BIT(19)
#define SOC_UART_FIFO_STATUS_WR_READY_BIT BIT(20) /* room for one more byte */
#define SOC_UART_FIFO_STATUS_RD_READY_BIT BIT(21) /* a byte is waiting */

/* --- fifo_port --- */
#define SOC_UART_FIFO_PORT_TX_DATA_MASK  0xFFU
#define SOC_UART_FIFO_PORT_RX_DATA_SHIFT 8U

/* --- int_enable / int_status (same bit layout) --- */
#define SOC_UART_INT_TX_NEED_WRITE_BIT BIT(0)
#define SOC_UART_INT_RX_NEED_READ_BIT  BIT(1)
#define SOC_UART_INT_RX_OVERFLOW_BIT   BIT(2)
#define SOC_UART_INT_RX_PARITY_ERR_BIT BIT(3)
#define SOC_UART_INT_RX_STOP_ERR_BIT   BIT(4)
#define SOC_UART_INT_TX_FINISH_BIT     BIT(5)
#define SOC_UART_INT_RX_FINISH_BIT     BIT(6)
#define SOC_UART_INT_RXD_WAKEUP_BIT    BIT(7)
#define SOC_UART_INT_ALL_MASK          0xFFU

/* --- clocks: SYS_CPU_CLK_DIV_MODE1 (sys_reg.h) --- */
#define SOC_SYS_CPU_CLK_DIV_MODE1_ADDR (SOC_SYS_REG_BASE + (0x8U << 2)) /* 0x44010020 */
#define SOC_SYS_CLKSEL_UART0_BIT       BIT(10) /* 0 = XTAL 26M (default), 1 = APLL */
#define SOC_SYS_CLKDIV_UART0_SHIFT     8U      /* [9:8] */

/*
 * Core clock: clkdiv_core[3:0] divides the source by (value + 1) and
 * cksel_core[5:4] picks it: 0 = XTAL 26 MHz, 3 = PLL 480 MHz.
 */
#define SOC_SYS_CLKDIV_CORE_MASK 0xFU
#define SOC_SYS_CLKSEL_CORE_MASK (0x3U << 4)
#define SOC_SYS_CLKSEL_CORE_PLL  (0x3U << 4)
#define SOC_SYS_CLKDIV_CORE_120M 3U /* 480 MHz / 4 */

/* --- clocks: SYS_CPU_CLK_DIV_MODE2 (sys_reg.h) --- */
#define SOC_SYS_CPU_CLK_DIV_MODE2_ADDR (SOC_SYS_REG_BASE + (0x9U << 2)) /* 0x44010024 */

/*
 * Flash controller clock: cksel_flash[25:24] picks the source (0 = XTAL,
 * 1 = PLL 480 MHz) and ckdiv_flash[27:26] divides it, where 1 is 480/6.
 */
#define SOC_SYS_CKSEL_FLASH_MASK (0x3U << 24)
#define SOC_SYS_CKSEL_FLASH_PLL  (0x1U << 24)
#define SOC_SYS_CKDIV_FLASH_MASK (0x3U << 26)
#define SOC_SYS_CKDIV_FLASH_80M  (0x1U << 26)

/* --- clocks: SYS_ANA_REG0 (sys_struct.h sys_ana_reg0_t) --- */
#define SOC_SYS_ANA_REG0_ADDR          (SOC_SYS_REG_BASE + (0x40U << 2)) /* 0x44010100 */
#define SOC_SYS_ANA_REG0_DPLL_TSTEN    BIT(0)                            /* PLL test mode */
#define SOC_SYS_ANA_REG0_SPIDETEN      BIT(4)  /* calibration detect enable */
#define SOC_SYS_ANA_REG0_SPITRIG       BIT(19) /* calibration trigger */
#define SOC_SYS_ANA_REG0_BAND1         BIT(21) /* VCO band bit 1 */
#define SOC_SYS_ANA_REG0_BANDMANUAL    BIT(25) /* manual VCO band select */
#define SOC_SYS_ANA_REG0_DSPTRIG       BIT(26) /* detector trigger */
#define SOC_SYS_ANA_REG0_VCTRL_DPLLLDO BIT(31) /* VCO LDO enable */

/* --- clocks: SYS_ANA_REG5 (sys_struct.h sys_ana_reg5_t) --- */
#define SOC_SYS_ANA_REG5_ADDR    (SOC_SYS_REG_BASE + (0x45U << 2)) /* 0x44010114 */
#define SOC_SYS_ANA_REG5_EN_DPLL BIT(5)                            /* PLL enable */

/*
 * Peripheral clocks of this image. This is where the console port is clocked
 * when the image owns the chip; the firmware the device ships with turns that
 * gate on already.
 */
#define SOC_SYS_CPU_DEVICE_CLK_ENABLE_ADDR (SOC_SYS_REG_BASE + (0xCU << 2)) /* 0x44010030 */
#define SOC_SYS_UART0_CKEN_BIT             BIT(2)

/*
 * 5. Interrupt map (straight to the NVIC, 64 lines)
 *
 * Peripheral interrupts go directly to the NVIC with no cascaded controller in
 * between. The second column of icu_map.h::ICU_DEV_MAP is the NVIC IRQ number.
 */

#define SOC_NUM_IRQS 64U

#define SOC_IRQ_DMA0     0
#define SOC_IRQ_TIMER0   3
#define SOC_IRQ_UART0    4 /* console */
#define SOC_IRQ_PWM0     5
#define SOC_IRQ_I2C0     6
#define SOC_IRQ_SPI0     7
#define SOC_IRQ_SDIO     10
#define SOC_IRQ_TIMER1   13
#define SOC_IRQ_I2C1     14
#define SOC_IRQ_UART1    15
#define SOC_IRQ_UART2    16
#define SOC_IRQ_SPI1     17
#define SOC_IRQ_CAN      18
#define SOC_IRQ_USB      19
#define SOC_IRQ_QSPI0    20
#define SOC_IRQ_AUDIO    23
#define SOC_IRQ_I2S0     24
#define SOC_IRQ_JPEG_ENC 25
#define SOC_IRQ_JPEG_DEC 26
#define SOC_IRQ_LCD      27
#define SOC_IRQ_DMA2D    28
#define SOC_IRQ_BLE      40
#define SOC_IRQ_QSPI1    42
#define SOC_IRQ_ETH      48
#define SOC_IRQ_RTC      54
#define SOC_IRQ_GPIO     55 /* GROUP1 */
#define SOC_IRQ_MAILBOX  63

/*
 * 6. Clocks
 *
 * 26 MHz crystal, which is what the core runs on out of reset. The first stage
 * leaves it there; soc.c switches the core to 120 MHz once it is up. The
 * console port stays on the crystal, so its divider does not depend on that:
 * clk_div = UART_CLOCK / baud - 1, so 115200 needs 26000000 / 115200 - 1 = 224.
 */

#define SOC_XTAL_FREQ_HZ        26000000U
#define SOC_APLL_FREQ_HZ        120000000U
#define SOC_DEFAULT_CPU_FREQ_HZ 120000000U

/*
 * 7. Mailbox mbox0
 *
 * When the image runs alongside the firmware the device ships with, it has to
 * report "powered up" here and then keep a heartbeat going, or that firmware
 * resets the whole chip a few seconds into the boot. See soc_ipc.c.
 *
 * The mailbox is FIFO based rather than a set of parameter registers: a send
 * pushes the *address* of a 16-byte command block in shared memory into the
 * FIFO, and the peer fetches the block from there. There is one channel per
 * participant, each owning a register group of 0x10 registers at a byte offset
 * of register number * 4.
 *
 * Channel ownership is easy to get backwards: a send goes out through the
 * sender's own tdata registers with mail_tid set to the destination, while a
 * receive reads the receiver's own mail_sid / rdata.
 *
 * Register layout: ap/middleware/soc/bk7258_ap/soc/mbox0_struct.h
 * Send/receive sequence: ap/middleware/soc/common/hal/mbox0_hal.c:69-110
 */

/* mbox0 registers: register number * 4 = byte offset (mbox0_ll.h reaches them
 * through struct member order instead)
 */
#define SOC_MBOX0_REG_ADDR(regno) (SOC_MBOX0_REG_BASE + ((uint32_t)(regno) << 2))

/* First register number of channel N's group: chn0=0x10, chn1=0x20, chn2=0x30 */
#define SOC_MBOX0_CHN_REG(n) (0x10U + ((uint32_t)(n) * 0x10U))

/* Offsets inside a group (increments of the register number) */
#define SOC_MBOX0_CHN_FIFO_CFG  0U /* fifo_start[5:0] / int_en[8] */
#define SOC_MBOX0_CHN_ENABLE    1U /* chn_enable[0] / fifo_length[6:1] */
#define SOC_MBOX0_CHN_TDATA0    2U /* send: data word 0 */
#define SOC_MBOX0_CHN_TDATA1    3U /* send: data word 1 */
#define SOC_MBOX0_CHN_MAIL_TID  4U /* send: destination channel, also "go" */
#define SOC_MBOX0_CHN_MAIL_SID  5U /* recv: source channel, also "go" */
#define SOC_MBOX0_CHN_RDATA0    6U /* recv: data word 0 */
#define SOC_MBOX0_CHN_RDATA1    7U /* recv: data word 1 */
#define SOC_MBOX0_CHN_FIFO_STAT 8U /* [0]=full [1]=empt [7:2]=count */

#define SOC_MBOX0_CHN_ADDR(n, off) SOC_MBOX0_REG_ADDR(SOC_MBOX0_CHN_REG(n) + (off))

#define SOC_MBOX0_FIFO_STAT_FULL_BIT BIT(0)
#define SOC_MBOX0_FIFO_STAT_EMPT_BIT BIT(1)

/* --- Logical channels and commands (ap/include/driver/mailbox_channel.h:78-80)
 * The IPC commands go out on MB_CHNL_HW_CTRL = (0 << 6) | (1 << 4) = 0x10, not
 * on the similarly named MB_CHNL_IPC (0x11).
 */
#define SOC_MB_LOG_CHNL_HW_CTRL  0x10U
#define SOC_MB_IPC_CMD_POWER_UP  1U
#define SOC_MB_IPC_CMD_HEARTBEAT 2U
#define SOC_MB_IPC_MSG_LEN       16U /* sizeof(mb_chnl_cmd_t) */

/* The mailbox channel this image uses, and the one the peer owns */
#define SOC_MB_SELF_CHN 1U
#define SOC_MB_PEER_CHN 0U

/* --- Physical command block header (mailbox_channel.c:64-84 phy_chnnl_hdr_t)
 *   [ 7: 0] cmd
 *   [11: 8] state
 *   [15:12] ctrl
 *   [23:16] tx_seq
 *   [31:24] logical_chnl
 */
#define SOC_MB_HDR(cmd, seq)                                                                       \
	(((uint32_t)(cmd) & 0xFFU) | (((uint32_t)(seq) & 0xFFU) << 16) |                           \
	 ((uint32_t)SOC_MB_LOG_CHNL_HW_CTRL << 24))

/* Generic header with an explicit logical channel (HW_CTRL is only one of them) */
#define SOC_MB_HDR_CH(chnl, cmd, seq)                                                              \
	(((uint32_t)(cmd) & 0xFFU) | (((uint32_t)(seq) & 0xFFU) << 16) | ((uint32_t)(chnl) << 24))

/* --- Logical channels in the outgoing direction (0x1x)
 * Source: ap/include/driver/mailbox_channel.h:78-96.
 *
 * The receiver renumbers these by its own outgoing direction, so what both
 * sides agree on is the index, not the number: the 0x10 sent from here lands on
 * the peer's own MB_CHNL_HW_CTRL.
 */
#define SOC_MB_CHNL_HW_CTRL 0x10U /* IPC: POWER_UP / HEART_BEAT / TEST_CMD ... */
#define SOC_MB_CHNL_IPC     0x11U
#define SOC_MB_CHNL_PWC     0x12U
#define SOC_MB_CHNL_FLASH   0x1BU
#define SOC_MB_CHNL_LOG     0x1EU

/* --- Logical channels the peer uses for messages of its own (0x4x)
 * A receiver sees the number the sender filled in, so anything the peer starts
 * carries a 0x4x channel, while the ACK it returns echoes the 0x1x sent from
 * here. That echo is why tx_seq plus logical_chnl is enough to match an ACK to
 * its request.
 */
#define SOC_MB_CHNL_PEER_BASE  0x40U
#define SOC_MB_CHNL_PEER_FLASH 0x4BU /* flash operation notice, see below */
#define SOC_MB_CHNL_PEER_LOG   0x4EU

/* Flash operation notices from the peer (flash_notify.c:47-57): hdr.cmd is
 * IPC_FLASH_OP_START or IPC_FLASH_OP_END, and param1 is IPC_FLASH_OP_REQ on the
 * way in, which the ACK has to answer with IPC_FLASH_OP_ACK. The ACK matters:
 * the peer waits up to 5 ms for it on every erase or program.
 */
#define SOC_MB_FLASH_OP_START 0U
#define SOC_MB_FLASH_OP_END   1U
#define SOC_MB_FLASH_OP_REQ   1U
#define SOC_MB_FLASH_OP_ACK   2U

/* ctrl bits of the physical command block header (mailbox_channel.c:59-61,
 * occupying hdr[15:12])
 */
#define SOC_MB_CTRL_ACK_BOX 0x01U
#define SOC_MB_CTRL_SYNC_TX 0x02U

/*
 * Decide whether a *received* message needs an ACK.
 *
 * The rule in the vendor mailbox_channel.c:433-435 is that the receiver always
 * answers with an ACK once it has handled the command -- echoing the command
 * block back with CHNL_CTRL_ACK_BOX set -- unless the sender set
 * CHNL_CTRL_SYNC_TX in hdr.ctrl. An ACK itself (ctrl carrying ACK_BOX) is
 * never ACKed in turn.
 *
 * Not answering has a concrete cost: the peer waits for this ACK before it
 * clears STATE_RX_IN_PROCESS in ipc_socket_tx_cmpl_handler(), so the server
 * socket stays busy for ever and rejects every later request.
 */
#define SOC_MB_HDR_IS_ACK(hdr)     ((((uint32_t)(hdr)) >> 12) & SOC_MB_CTRL_ACK_BOX)
#define SOC_MB_HDR_IS_SYNC_TX(hdr) ((((uint32_t)(hdr)) >> 12) & SOC_MB_CTRL_SYNC_TX)

/* IPC command numbers (cp/middleware/driver/mailbox/mb_ipc_cmd.h:22-56) */
#define SOC_MB_IPC_CMD_TEST     0U /* loopback: peer adds 1 to the 4-byte payload */
#define SOC_MB_IPC_RSP_CMD_FLAG 0x80U

/* ack_state of mb_chnl_ack_t (mailbox_channel.h:129-134) */
#define SOC_MB_ACK_STATE_PENDING  0x01U
#define SOC_MB_ACK_STATE_COMPLETE 0x02U
#define SOC_MB_ACK_STATE_FAIL     0x03U

#ifndef _ASMLANGUAGE

/**
 * @brief Report that this image is powered up (once; the peer waits 2 s).
 */
void soc_bk7258_ipc_power_up(void);

#endif /* _ASMLANGUAGE */

/*
 * Both guards have to be closed here: the outer _SOC_BK7258_SOC_H_ and the
 * inner _ASMLANGUAGE. Leaving the outer one open swallows everything after
 * this file is included, which shows up as a cascade of unrelated CMSIS errors
 * rather than as a missing #endif.
 */
#endif /* _ASMLANGUAGE */

#endif /* _SOC_BK7258_SOC_H_ */
