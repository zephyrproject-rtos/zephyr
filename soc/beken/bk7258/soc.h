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
 * All addresses carry SOC_ADDR_OFFSET. The reference configuration builds with
 * SPE=1 (TrustZone secure state), so SOC_ADDR_OFFSET is 0 and the
 * values below are the final addresses.
 */

#define SOC_ITCM_DATA_BASE  0x00000000U /* 16 KB (ITCM_SIZE=0x4000) */
#define SOC_DTCM_DATA_BASE  0x20000000U /* 16 KB (DTCM_SIZE=0x4000) */
#define SOC_FLASH_DATA_BASE 0x02000000U /* XIP code window (alias address) */
#define SOC_ROM_DATA_BASE   0x06000000U

#define SOC_SRAM0_DATA_BASE 0x28000000U /* 64 KB */
#define SOC_SRAM1_DATA_BASE 0x28010000U /* 64 KB */
#define SOC_SRAM2_DATA_BASE 0x28020000U /* 128 KB */
#define SOC_SRAM3_DATA_BASE 0x28040000U /* 128 KB */
#define SOC_SRAM4_DATA_BASE 0x28060000U /* 128 KB */
#define SOC_SRAM5_DATA_BASE 0x28080000U /* 128 KB */
/* 640 KB of SRAM in total (0xA0000), matching ram_regions.csv */

#define SOC_PSRAM_DATA_BASE 0x60000000U /* 8 MB */
#define SOC_QSPI0_DATA_BASE 0x64000000U
#define SOC_QSPI1_DATA_BASE 0x68000000U

/*
 * Alias views (from the comments in bk7258_ap_bsp.ld):
 * 0x08000000 instruction region <-> 0x28000000 data region
 * 0x18000000 instruction region <-> 0x38000000 data region
 * that is, instruction alias = data address - 0x20000000.
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
 * level 1: pin configuration word bit 6, gpio_2_func_en, the master switch
 * that hands the pin to a peripheral
 * level 2: seven words in the SYSTEM domain, each covering 8 pins with a
 * 4-bit function number per pin, at
 * GPIO_LL_SYSTEM_REG_BASE = SOC_SYS_REG_BASE + 0x30*4 = 0x440100C0
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
 * Interrupt type encoding (hal_gpio_types.h: gpio_int_type_t)
 * 0 = low level, 1 = high level, 2 = rising edge, 3 = falling edge
 *
 * Only 2 bits are available, so the hardware has no both-edges mode and the
 * driver has to answer GPIO_INT_TRIG_BOTH with -ENOTSUP.
 */
#define SOC_GPIO_INT_TYPE_LOW_LEVEL    0U
#define SOC_GPIO_INT_TYPE_HIGH_LEVEL   1U
#define SOC_GPIO_INT_TYPE_RISING_EDGE  2U
#define SOC_GPIO_INT_TYPE_FALLING_EDGE 3U

/*
 * Interrupt status registers (gpio_struct.h: gpio_hw_t)
 *
 * The pin configuration words occupy REG_0x00 to REG_0x37 (56 pins) followed
 * by 8 reserved words, so the status registers start at REG_0x40, i.e. byte
 * offset 0x40*4 = 0x100.
 *
 * Write 1 to clear: gpio_ll_clear_interrupt_status() simply writes the value
 * it just read back into the register.
 */
#define SOC_GPIO_INT_ST_LO_ADDR (SOC_AON_GPIO_REG_BASE + (0x40U << 2)) /* GPIO0..31 */
#define SOC_GPIO_INT_ST_HI_ADDR (SOC_AON_GPIO_REG_BASE + (0x41U << 2)) /* GPIO32..55 */

/*
 * The second-level interrupt enable of this core. Without it a GPIO
 * interrupt never reaches the NVIC at all.
 *
 * Interrupts on the BK7258 are two-level:
 * level 1: the NVIC (IRQ numbers in icu_map.h)
 * level 2: one pair of "IRQ 0..31 / 32..63" enable bits per CPU in the
 * SYSTEM domain, set by the vendor sys_drv_int_group2_enable().
 * The interrupt is only reported to the CPU when this bit is set
 * too.
 *
 * Bit numbering (verified): IRQ N maps to
 * N < 32 -> cpu1_int_0_31_en bit N
 * N >= 32 -> cpu1_int_32_63_en bit (N - 32)
 * For example GPIO, IRQ 55, is bit 23, matching
 * SYS_CPU0_INT_32_63_EN_CPU0_GPIO_INT_EN_POS(23) in sys_types.h.
 *
 * Source: ap/middleware/soc/bk7258_ap/hal/sys_ll.h
 * sys_ll_set_cpu1_int_0_31_en_value() -> SOC_SYS_REG_BASE + (0x22 << 2)
 * sys_ll_set_cpu1_int_32_63_en_value() -> SOC_SYS_REG_BASE + (0x23 << 2)
 *
 * The SYSTEM domain is shared with the system controller, so any update has to be a read-modify-
 * write: a plain sys_write32 would clear the enable bits of other interrupts.
 */
#define SOC_SYS_CPU1_INT_0_31_EN_ADDR  (SOC_SYS_REG_BASE + (0x22U << 2)) /* 0x44010088 */
#define SOC_SYS_CPU1_INT_32_63_EN_ADDR (SOC_SYS_REG_BASE + (0x23U << 2)) /* 0x4401008C */

/* UART0 pins (macros at the end of gpio_map.h) */
#define SOC_GPIO_UART0_TX_PIN   11U
#define SOC_GPIO_UART0_RX_PIN   10U
#define SOC_GPIO_UART0_CTS_PIN  13U
#define SOC_GPIO_UART0_RTS_PIN  12U
/* UART0 is entry 0 in the function table of both GPIO10 and GPIO11 */
#define SOC_GPIO_UART0_FUNC_NUM 0U

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

#define SOC_UART0_HW ((soc_uart_hw_t *)SOC_UART0_REG_BASE)

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
 * --- clocks: display ---
 *
 * The display clock is one of the few in this port whose value the hardware it
 * drives depends on directly: it becomes the pixel clock on the RGB bus, so the
 * frame rate the panel sees is this clock divided by the frame's total pixel
 * count. The vendor's own table for the panel on this board asks for 26 MHz,
 * which is the source and the two divider fields below.
 */
#define SOC_SYS_CPU_CLK_DIV_MODE2_ADDR (SOC_SYS_REG_BASE + (0x9U << 2)) /* 0x44010024 */
#define SOC_SYS_CLKDIV_DISP_L_BIT      BIT(31) /* in mode1: low bit of the divider */
#define SOC_SYS_CLKDIV_DISP_H_SHIFT    0U      /* in mode2: [2:0] */
#define SOC_SYS_CLKSEL_DISP_BIT        BIT(3)  /* in mode2: 0 selects the 320 MHz source */

#define SOC_SYS_CPU_DEVICE_CLK_ENABLE_ADDR (SOC_SYS_REG_BASE + (0xCU << 2)) /* 0x44010030 */
#define SOC_SYS_DEVICE_CKEN_DISP_BIT       BIT(29)

/*
 * --- LCD controller: RGB parallel output ---
 *
 * Offsets and field positions follow lcd_disp_ll_macro_def.h in the vendor's
 * tree (base 0x48060000); the field names are theirs. Only what this port
 * programs is named here.
 *
 * The controller reads one whole frame from memory and scans it out
 * continuously, which is why the frame has to live somewhere the controller can
 * be pointed at and left alone.
 */
/*
 * The display module is powered down by default: two bits in the SYSTEM block,
 * its shutdown and deep-sleep lines, come up set, and while they are set the
 * controller's whole register block reads back as zero. That is worth knowing
 * before it is worth debugging -- a controller whose registers read zero is not
 * a controller that was configured wrong, it is one that is not there yet.
 */
/*
 * The video/display power domain. One bit, and it comes up set: the whole VIDP
 * domain -- the display controller among it -- starts powered down, which is
 * why every register in the controller reads back as zero until it is cleared.
 * The system controller's power management owns this bit in the vendor flow and the AP asks
 * it over the mailbox; this port writes it directly, which is the smaller thing
 * to do until the port grows a power story of its own.
 */
#define SOC_SYS_CPU_POWER_SLEEP_WAKEUP_ADDR (SOC_SYS_REG_BASE + (0x10U << 2)) /* 0x44010040 */
#define SOC_SYS_PWD_VIDP_BIT                BIT(7)                            /* 1 = powered down */

#define SOC_SYS_DEVICE_MEM_CTRL1_ADDR (SOC_SYS_REG_BASE + (0xEU << 2)) /* 0x44010038 */
#define SOC_SYS_DEVICE_MEM_CTRL2_ADDR (SOC_SYS_REG_BASE + (0xFU << 2)) /* 0x4401003C */
#define SOC_SYS_MEM_CTRL_DISP_SD_BIT  BIT(18)
#define SOC_SYS_MEM_CTRL_DISP_DS_BIT  BIT(18)

#define SOC_LCD_DISP_REG_BASE 0x48060000U

#define SOC_LCD_MODULE_CONTROL_ADDR   (SOC_LCD_DISP_REG_BASE + 0x08U)
#define SOC_LCD_MODULE_SOFT_RESET_BIT BIT(0)
#define SOC_LCD_MODULE_CLK_GATE_BIT   BIT(1)

#define SOC_LCD_DISPLAY_INT_ADDR (SOC_LCD_DISP_REG_BASE + 0x10U)
#define SOC_LCD_INT_DISCONTI_BIT BIT(28) /* stop the clock between frames */

#define SOC_LCD_RGB_CFG_ADDR            (SOC_LCD_DISP_REG_BASE + 0x14U)
#define SOC_LCD_RGB_CFG_X_PIXEL_SHIFT   0U /* [10:0] */
#define SOC_LCD_RGB_CFG_X_PIXEL_MASK    0x7FFU
#define SOC_LCD_RGB_CFG_DCLK_REV_BIT    BIT(11) /* 1 = data changes on the falling edge */
#define SOC_LCD_RGB_CFG_Y_PIXEL_SHIFT   12U     /* [22:12] */
#define SOC_LCD_RGB_CFG_Y_PIXEL_MASK    0x7FFU
#define SOC_LCD_RGB_CFG_OUT_666_LSB_BIT BIT(23)
#define SOC_LCD_RGB_CFG_DISP_ON_BIT     BIT(24) /* the panel's DISP line */
#define SOC_LCD_RGB_CFG_RGB_ON_BIT      BIT(25) /* RGB module output enable */
#define SOC_LCD_RGB_CFG_LCD_ON_BIT      BIT(26) /* controller output enable */
#define SOC_LCD_RGB_CFG_OUT_FMT_SHIFT   27U     /* [29:27]: bits per pixel to the panel */
#define SOC_LCD_RGB_CFG_OUT_FMT_MASK    0x7U
#define SOC_LCD_RGB_CFG_SRC_FMT_BIT     BIT(31) /* 0 = RGB565 in memory, 1 = RGB888 */

#define SOC_LCD_SYNC_CFG_ADDR          (SOC_LCD_DISP_REG_BASE + 0x1CU)
#define SOC_LCD_SYNC_HSYNC_BACK_SHIFT  0U  /* [7:0] */
#define SOC_LCD_SYNC_HSYNC_FRONT_SHIFT 8U  /* [14:8] */
#define SOC_LCD_SYNC_VSYNC_BACK_SHIFT  15U /* [19:15] */
#define SOC_LCD_SYNC_VSYNC_FRONT_SHIFT 20U /* [26:20] */

#define SOC_LCD_RGB_SYNC_LOW_ADDR    (SOC_LCD_DISP_REG_BASE + 0x34U)
#define SOC_LCD_SYNC_LOW_HSYNC_SHIFT 0U /* [5:0] pulse width in clocks */
#define SOC_LCD_SYNC_LOW_VSYNC_SHIFT 8U /* [13:8] pulse width in lines */

#define SOC_LCD_DAT_FIFO_THRD_ADDR (SOC_LCD_DISP_REG_BASE + 0x40U)
#define SOC_LCD_FIFO_WR_THRD_SHIFT 8U  /* [17:8] */
#define SOC_LCD_FIFO_RD_THRD_SHIFT 20U /* [29:20] */

/* Where the controller says what the fetch is doing: bit 23 the display FIFO
 * is empty, bit 24 it is nearly full, bits 0..5 the frame counter.
 */
#define SOC_LCD_DISP_STATUS_ADDR (SOC_LCD_DISP_REG_BASE + 0x30U)
#define SOC_LCD_FRAME_ADDR_ADDR  (SOC_LCD_DISP_REG_BASE + 0x44U)

/*
 * Horizontal and vertical blanking. The panel that lights on this board uses
 * back/front porches of 46/48 and 24/24 mclk symbols, read out of the vendor's
 * working firmware; the controller has to emit the same, or the panel's sampler
 * never sits inside a valid window.
 */
#define SOC_LCD_H_PORCH_ADDR (SOC_LCD_DISP_REG_BASE + 0x50U)
#define SOC_LCD_V_PORCH_ADDR (SOC_LCD_DISP_REG_BASE + 0x54U)

/*
 * 5. Interrupt map (straight to the NVIC, 64 lines)
 *
 * Peripheral interrupts go directly to the NVIC, with no cascaded controller
 * in between. The second column of
 * icu_map.h::ICU_DEV_MAP is the NVIC IRQ number.
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
 * Measured configuration: XTAL_FREQ=26000000,
 * CPU_FREQ_HZ=120000000.
 * UART baud rate: clk_div = UART_CLOCK / baud - 1
 * 115200 -> 26000000 / 115200 - 1 = 224
 */

#define SOC_XTAL_FREQ_HZ        26000000U
#define SOC_APLL_FREQ_HZ        120000000U
#define SOC_DEFAULT_CPU_FREQ_HZ 120000000U

/* Equivalent of the vendor uart_hal_set_baud_rate() computation */
#define SOC_UART_CLK_DIV_FROM_XTAL(baud) ((SOC_XTAL_FREQ_HZ / (uint32_t)(baud)) - 1U)
#define SOC_UART_CLK_DIV_FROM_APLL(baud) ((SOC_APLL_FREQ_HZ / (uint32_t)(baud)) - 1U)

/*
 * 7. CPUs
 */

/*
 * 8. Clock self-check
 */

/**
 * @brief Read the raw value of SYS_CPU_CLK_DIV_MODE1 (0x44010020).
 *
 * Printed at boot to confirm the CPU clock configuration left behind by BL2.
 *
 * Bit fields (sys_struct.h::sys_cpu_clk_div_mode1_t):
 * [ 3: 0] clkdiv_core core divider
 * [ 5: 4] cksel_core core clock source; 2 = 320 MHz and 3 = 480 MHz are
 * known (PM_CLKSEL_CORE_320M/_480M in sys_hal.c),
 * 0 and 1 are unconfirmed. The reference build has
 * CPU_FREQ_HZ=120000000, so one of them is
 * presumably the 120 MHz DPLL setting.
 * [ 6: 6] clkdiv_bus
 * [ 9: 8] clkdiv_uart0
 * [10:10] clksel_uart0 0 = XTAL 26M (default), 1 = APLL
 *
 * Defined in soc.c.
 */
uint32_t soc_bk7258_read_clk_div_mode1(void);

/**
 * @brief Decode clksel_uart0 / clkdiv_uart0 out of clk_div_mode1 for printing.
 */
void soc_bk7258_decode_uart0_clk(uint32_t reg, bool *use_apll, uint32_t *uart0_div);

/*
 * 9. Inter-core mailbox mbox0 (how this core tells the system controller "I am still alive")
 *
 * This link is a hard requirement for this core to stay up, not an
 * optimisation:
 * - After the system controller releases this core it waits 2 s for POWER_UP and
 * then prints "IPC retry to start core1".
 * - From then on it waits 6 s (MB_IPC_HEARTBEAT_TIME * 3) between
 * heartbeats before hitting BK_ASSERT(false), which resets the whole chip.
 * The visible symptom is a board that keeps rebooting with no this core's log at
 * all.
 * Source: ap/middleware/driver/mailbox/mb_ipc_heartbeat.c:191-236 (master).
 *
 * The mailbox is FIFO based, not a set of four parameter registers:
 * - Each channel N owns a register group (0x10 registers per group,
 * register number * 4 = byte offset).
 * - A send pushes the *address* of a 16-byte command block in shared memory
 * into the FIFO; the system controller fetches the block from that address.
 * Source: ap/middleware/soc/bk7258_ap/soc/mbox0_struct.h
 *
 * Channel semantics (easy to get backwards):
 * - Channel N belongs to CPU N, so this core sends through channel 1 and
 * receives through channel 1: sending writes that channel's tdata0/tdata1
 * plus mail_tid = destination, and the hardware then delivers the message
 * into the FIFO of the *destination* channel; receiving reads its
 * mail_sid/rdata.
 * - In short: send through your own channel's tdata, receive through your
 * own channel's rdata.
 * Source: ap/middleware/soc/common/hal/mbox0_hal.c:69-81 (chn1_send) and
 * :97-110 (chn0_recv), ap/middleware/driver/mailbox/mbox0_drv.c:37,95
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
#define SOC_MBOX0_CHN_MAIL_TID  4U /* send: write = destination CPU, also "go" */
#define SOC_MBOX0_CHN_MAIL_SID  5U /* recv: read = source CPU, also "go" */
#define SOC_MBOX0_CHN_RDATA0    6U /* recv: data word 0 */
#define SOC_MBOX0_CHN_RDATA1    7U /* recv: data word 1 */
#define SOC_MBOX0_CHN_FIFO_STAT 8U /* [0]=full [1]=empt [7:2]=count */

#define SOC_MBOX0_CHN_ADDR(n, off) SOC_MBOX0_REG_ADDR(SOC_MBOX0_CHN_REG(n) + (off))

#define SOC_MBOX0_FIFO_STAT_FULL_BIT BIT(0)
#define SOC_MBOX0_FIFO_STAT_EMPT_BIT BIT(1)

/* --- Logical channels and commands (ap/include/driver/mailbox_channel.h:78-80)
 * MB_CHNL_HW_CTRL = CP0_MB_LOG_CHNL_START = CPX_LOG_CHNL_START(CPU1, CPU0)
 * = (0<<6) | (1<<4) = 0x10
 *
 * The this side ipc_init() uses MB_CHNL_HW_CTRL, not the better-named
 * CP0_MB_CHNL_IPC (0x11): do not be misled by the name.
 */
#define SOC_MB_LOG_CHNL_HW_CTRL  0x10U
#define SOC_MB_IPC_CMD_POWER_UP  1U  /* IPC_CPU1_POWER_UP_INDICATION */
#define SOC_MB_IPC_CMD_HEARTBEAT 2U  /* IPC_CPU1_HEART_BEAT_INDICATION */
#define SOC_MB_IPC_MSG_LEN       16U /* sizeof(mb_chnl_cmd_t) */

/* Mailbox numbering: this core is 1, the system controller is 0 (see SOC_MB_CHNL_*) */
#define SOC_MB_SELF_CPU 1U
#define SOC_MB_PEER_CPU 0U

/* --- Physical command block header (mailbox_channel.c:64-84 phy_chnnl_hdr_t)
 * [ 7: 0] cmd
 * [11: 8] state
 * [15:12] ctrl
 * [23:16] tx_seq
 * [31:24] logical_chnl
 */
#define SOC_MB_HDR(cmd, seq)                                                                       \
	(((uint32_t)(cmd) & 0xFFU) | (((uint32_t)(seq) & 0xFFU) << 16) |                           \
	 ((uint32_t)SOC_MB_LOG_CHNL_HW_CTRL << 24))

/* Generic header with an explicit logical channel (HW_CTRL is only one of them) */
#define SOC_MB_HDR_CH(chnl, cmd, seq)                                                              \
	(((uint32_t)(cmd) & 0xFFU) | (((uint32_t)(seq) & 0xFFU) << 16) | ((uint32_t)(chnl) << 24))

/* --- Logical channels in the outbound direction (this core -> system controller, 0x1x)
 * Source: ap/include/driver/mailbox_channel.h:78-96.
 *
 * The receiver renumbers them: when the system controller receives logical channel i it
 * computes CPX_LOG_CHNL_START(CPU0, CPU1) + i = 0x50 + i. So the 0x10 we send
 * lands on the system controller's own MB_CHNL_HW_CTRL index. Each side numbers by its own
 * outgoing direction, while the index i is the same.
 */
#define SOC_MB_CHNL_HW_CTRL   0x10U /* IPC: POWER_UP / HEART_BEAT / TEST_CMD ... */
#define SOC_MB_CHNL_IPC       0x11U
#define SOC_MB_CHNL_PWC       0x12U
#define SOC_MB_CHNL_BT_CMD    0x13U /* the wireless stack runs behind this core */
#define SOC_MB_CHNL_WIFI_CMD  0x14U
#define SOC_MB_CHNL_WIFI_DATA 0x15U
#define SOC_MB_CHNL_FLASH     0x1BU
#define SOC_MB_CHNL_LOG       0x1EU

/* --- Logical channels in the inbound direction (system controller -> this core, 0x4x)
 * A receiver sees exactly the number the sender filled in, so anything the system controller
 * starts carries a 0x4x channel, while the ACK it returns echoes the 0x1x sent
 * from here. Note that the system controller's replies to an mb_ipc request also come back on its
 * own channel (0x41 for the router channel), not on 0x1x.
 */
#define SOC_MB_CHNL_CP_BASE      0x40U
#define SOC_MB_CHNL_CP_IPC       0x41U /* mb_ipc commands pushed to this core */
#define SOC_MB_CHNL_AP_IPC       0x11U /* mb_ipc commands this core sends */
#define SOC_MB_CHNL_CP_BT_CMD    0x43U
#define SOC_MB_CHNL_CP_WIFI_CMD  0x44U /* replies and events of the command channel */
#define SOC_MB_CHNL_CP_WIFI_DATA 0x45U
#define SOC_MB_CHNL_CP_FLASH     0x4BU /* the system controller flash operation notice, see below */
#define SOC_MB_CHNL_CP_LOG       0x4EU
#define SOC_MB_CHNL_CP_SARADC    0x4CU /* SARADC server push, not ours */

/* Commands of the shell protocol carried on the LOG channel
 * (ap/components/bk_cli/shell_drv.h:119-125).
 *
 * MB_CMD_LOG_OUT, the outbound direction: print this string on UART0.
 * Its payload is log_cmd_t { hdr; u8 *buf; u16 len; u16 tag }
 * so word1 is the string pointer and word2 low 16 is the
 * length (see soc_bk7258_mbox_log_write()).
 *
 * MB_CMD_USER_INPUT the inbound direction: the line the system controller's own CLI received
 * after `ap_cmd ` is forwarded here. Same shape with one
 * field fewer, user_cmd_t { hdr; u8 *buf; u16 len; }.
 */
#define SOC_MB_CMD_LOG_OUT    1U
#define SOC_MB_CMD_USER_INPUT 3U

/*
 * Longest string this core hands to the system controller in a single LOG message.
 *
 * The mailbox is metered per message, so console output is accumulated into
 * lines and sent a line at a time; a line longer than this is split. The system controller
 * prints what it receives verbatim, so the string must be NUL-free.
 */
#define SOC_MB_LOG_LINE_LEN 128U

/* the system controller flash operation notices (cp/middleware/driver/flash/flash_notify.c:47-57)
 * hdr.cmd : 0 = IPC_FLASH_OP_START (the system controller is about to erase or program),
 * 1 = IPC_FLASH_OP_END
 * param1 : IPC_FLASH_OP_REQ (1) on the way in; the ACK must answer with
 * IPC_FLASH_OP_ACK (2)
 *
 * Implementing this is mandatory, not a refinement: the system controller's send_flash_op_state()
 * waits up to 5 ms for the ACK before carrying on. If we never answer, the system controller
 * burns 5 ms on every erase or program and believes this core has stopped
 * touching flash when it has not -- two cores driving the same device is
 * exactly the whole-chip reset described in .
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
 * What happens when we do not answer (measured): after sending its
 * mb_ipc reply the system controller waits for our ACK before clearing STATE_RX_IN_PROCESS in
 * ipc_socket_tx_cmpl_handler(). Without it the server socket stays RX_BUSY for
 * ever and every later SEND is rejected with route_status=5 /
 * api_impl_status=2. The symptom is "CONNECT succeeded but not a single flash
 * access works".
 */
#define SOC_MB_HDR_IS_ACK(hdr)     ((((uint32_t)(hdr)) >> 12) & SOC_MB_CTRL_ACK_BOX)
#define SOC_MB_HDR_IS_SYNC_TX(hdr) ((((uint32_t)(hdr)) >> 12) & SOC_MB_CTRL_SYNC_TX)

/* IPC command numbers (cp/middleware/driver/mailbox/mb_ipc_cmd.h:22-56) */
/* loopback: the system controller adds 1 to the 4-byte payload */
#define SOC_MB_IPC_CMD_TEST     0U
#define SOC_MB_IPC_RSP_CMD_FLAG 0x80U

/* ack_state of mb_chnl_ack_t (mailbox_channel.h:129-134) */
#define SOC_MB_ACK_STATE_PENDING  0x01U
#define SOC_MB_ACK_STATE_COMPLETE 0x02U
#define SOC_MB_ACK_STATE_FAIL     0x03U

#ifndef _ASMLANGUAGE

/**
 * @brief Report to the system controller that this core is powered up
 * (once; the system controller waits 2 s).
 */
void soc_bk7258_ipc_power_up(void);

/**
 * @brief Send one heartbeat to the system controller (then every 2 s; 6 s without one resets us).
 *
 * @return 0 delivered; -EBUSY the system controller FIFO is full (drop this tick, retry).
 */
int soc_bk7258_ipc_heartbeat(void);

/*
 * : turn "heartbeat only" into a usable bidirectional inter-core transport.
 *
 * The semantics mirror the vendor mb_chnl_write() one for one: the mailbox
 * FIFO carries the *address* of a 16-byte command block in shared memory plus
 * its length, not the data itself.
 *
 * word0 = header (cmd[7:0] / state[11:8] / tx_seq[23:16] / channel[31:24])
 * word1 = param1 (for IPC this is cmd_buff, the payload pointer)
 * word2 = param2 (for IPC this is cmd_data_len, low 16 bits)
 * word3 = param3 (for an ACK this is ack_state)
 */

/**
 * @brief Callback invoked when an inter-core message arrives.
 *
 * @param hdr command block word0
 * @param w1 command block word1 (payload pointer)
 * @param w2 command block word2 (payload length)
 * @param w3 command block word3 (ack_state of an ACK)
 * @param nbytes length declared by the system controller (the length word in the FIFO)
 */
typedef void (*soc_bk7258_mbox_rx_cb_t)(uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3,
					uint32_t nbytes);

/**
 * @brief Register another receive callback; every subscriber sees every message.
 *
 * This adds a subscriber rather than replacing one: the flash proxy and a
 * wireless driver both need the mailbox, and neither may silently displace the
 * other. Each subscriber is handed the whole block and filters by logical
 * channel itself. Registering the same callback twice is a no-op.
 *
 * Staying alive does not require a callback: ACKs to the heartbeat are simply
 * drained and dropped and the system controller does not block on us.
 * But seeing its replies (the +1 result of IPC_TEST_CMD, for instance)
 * does require one.
 */
void soc_bk7258_mbox_set_rx_cb(soc_bk7258_mbox_rx_cb_t cb);

/**
 * @brief Send a command on a given logical channel.
 *
 * @param chnl logical channel number (SOC_MB_CHNL_*)
 * @param cmd command number
 * @param p1 word1: payload pointer, must be stable shared memory, never a
 * stack variable
 * @param p2 word2: payload length, low 16 bits
 * @return 0 delivered; -EBUSY the system controller FIFO is full
 */
int soc_bk7258_mbox_send(uint32_t chnl, uint32_t cmd, uint32_t p1, uint32_t p2);

/**
 * @brief Drain the receive FIFO (messages are dropped when no callback is set).
 *
 * Called by the heartbeat timer every 2 s; latency-sensitive applications can
 * call it more often from their own loop.
 */
void soc_bk7258_mbox_pump(void);

/**
 * @brief Number of times the mailbox receive interrupt ran.
 *
 * Zero means the interrupt never fired and the polling path in
 * soc_bk7258_mbox_pump() is doing all the work.
 */
uint32_t soc_bk7258_mbox_irq_count(void);

/* Echo latency of the mailbox path, measured in the receiving interrupt:
 * count, average and worst case in microseconds.
 */
void soc_bk7258_mbox_echo_stats(uint32_t *n, uint32_t *avg_us, uint32_t *max_us);

/**
 * @brief Largest number of messages drained in a single interrupt pass.
 */
uint32_t soc_bk7258_mbox_batch_max(void);

/**
 * @brief Number of interrupt passes that drained more than one message.
 *
 * Non-zero means the system controller queued several messages behind one interrupt, which
 * is the case where echoing a per-message tx_seq falls behind.
 */
uint32_t soc_bk7258_mbox_batch_multi(void);

/**
 * @brief Number of SARADC server pushes left unanswered.
 */
uint32_t soc_bk7258_mbox_saradc_skipped(void);

/**
 * @brief Number of acknowledgments that could not be written.
 *
 * A full system-controller FIFO makes the write fail and nothing retries it, so every one of
 * these becomes a timeout on the system controller's side.
 */
uint32_t soc_bk7258_mbox_ack_tx_fail(void);

/**
 * @brief Number of ipc level responses sent from the receive interrupt.
 */
uint32_t soc_bk7258_mbox_ipc_rsp_sent(void);

/**
 * @brief Send an ACK back to the system controller (echo the command block, changing only
 * what has to change).
 *
 * @param addr address of the command block we received (logging / sanity only;
 * the ACK uses a buffer of our own)
 * @param hdr the word0 we received (cmd / tx_seq / logical_chnl are preserved;
 * the system controller's physical layer checks the latter two, its router layer
 * then checks cmd plus param1)
 * @param w1 ACK ack_data1 (IPC_FLASH_OP_ACK for flash notices)
 * @param w2 ACK ack_data2 (normally passed straight through)
 * @param w3 ACK ack_state (ACK_STATE_COMPLETE and friends)
 * @return 0 delivered; -EBUSY the system controller FIFO is full
 */
int soc_bk7258_mbox_ack(uint32_t addr, uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3);

/**
 * @brief Number of ACKs we have sent to the system controller, including the automatic
 * logical-channel ACKs issued from the pump.
 *
 * Hardware check: this counter must keep climbing while an mb_ipc exchange
 * runs. If it does not, the system controller's replies are never confirmed and the server socket
 * stays stuck at RX_BUSY.
 */
uint32_t soc_bk7258_mbox_txack_count(void);

/**
 * @brief Post an already built command block (caller owns the header and the
 * buffer).
 *
 * @param cmd address of the 16-byte command block (stable shared memory)
 * @param len length of the block (normally SOC_MB_IPC_MSG_LEN = 16)
 * @return 0 delivered; -EBUSY the system controller FIFO is full
 *
 * Difference from soc_bk7258_mbox_send(): that one reuses the single command
 * block owned by soc_ipc.c, which suits a fire-and-forget heartbeat. Console
 * output and IPC need several messages in flight at once, so they must bring
 * their own buffer and rotate it, or the next line overwrites the previous one
 * before the system controller has read it.
 */
int soc_bk7258_mbox_post(const uint32_t *cmd, uint32_t len);

/**
 * @brief Send a command block whose four words the caller supplies in full.
 *
 * @param chnl logical channel number (SOC_MB_CHNL_*)
 * @param cmd first word of the block header (see SOC_MB_HDR_CH())
 * @param p1 second word, @param p2 third, @param p3 fourth
 *
 * The Wi-Fi command channel needs this: its server reads the block as a
 * descriptor of a buffer chain rather than as (pointer, length), so all four
 * words carry meaning and none of them can be left zero.
 */
int soc_bk7258_mbox_post_cmd(uint32_t chnl, uint32_t cmd, uint32_t p1, uint32_t p2, uint32_t p3);

/*
 * Give the mailbox a reader thread of its own. Images whose driver does not
 * already pump the mailbox need this before waiting on anything that arrives
 * through it.
 */
void soc_bk7258_mbox_reader_start(void);

/**
 * @brief Read the statistics counters (any pointer may be NULL).
 */
void soc_bk7258_mbox_stats(uint32_t *rx, uint32_t *ack, uint32_t *flash_notify);

/**
 * @brief Print a string on the system controller's console, over the outbound LOG channel.
 *
 * This is the one and only way this core can produce visible output: it has no
 * UART of its own (UART0 belongs to the system controller), so the string is sent as a LOG
 * message and the system controller prints it verbatim.
 *
 * @param str the string to print (need not be NUL terminated, no NUL may be
 * part of it)
 * @param len number of bytes, at most SOC_MB_LOG_LINE_LEN
 * @return 0 delivered; -EBUSY the system controller FIFO is full (the caller drops it)
 */
int soc_bk7258_mbox_log_write(const char *str, uint32_t len);

/**
 * @brief Take one byte of console input forwarded by the system controller, or -1 when empty.
 *
 * The system controller's own CLI owns the terminal, so input enters this core through it: a
 * line typed as `ap_cmd <text>` on the system controller's console is forwarded to this core
 * on the LOG channel as MB_CMD_USER_INPUT and lands here.
 *
 * Safe to call from any context; it only takes the interrupt lock.
 */
int soc_bk7258_mbox_console_getc(void);

/**
 * @brief Statistics of the console input path (any pointer may be NULL).
 *
 * @param msgs MB_CMD_USER_INPUT messages received
 * @param dropped whole lines thrown away because the queue was full
 * @param bad_ptr messages whose payload pointer was outside the shared SRAM
 *
 * A non-zero bad_ptr means the system controller put the string somewhere this core cannot read,
 * which is the one failure mode that would silently look like "the system controller forwards
 * nothing".
 */
void soc_bk7258_mbox_console_stats(uint32_t *msgs, uint32_t *dropped, uint32_t *bad_ptr);

#endif /* _ASMLANGUAGE */

/*
 */
#endif /* _ASMLANGUAGE */

#endif /* _SOC_BK7258_SOC_H_ */
