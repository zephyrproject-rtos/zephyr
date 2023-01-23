/**
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2023 Mr Beam Lasers GmbH.
 * Copyright (c) 2023 Amrith Venkat Kesavamoorthi <amrith@mr-beam.org> 
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * @see https://www.buydisplay.com/download/ic/GC9A01A.pdf
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_GC9A01A_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_GC9A01A_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

/* Command registers*/
// #define GC9A01A_SWRESET 0x01 ///< Software reset register
// #define GC9A01A 0x04   ///< Read display identification information
#define GC9A01A 0x09   ///< Read Display Status

#define GC9A01A_SLPIN 0x10  ///< Enter Sleep Mode
#define GC9A01A_SLPOUT 0x11 ///< Sleep Out
#define GC9A01A_PTLON 0x12  ///< Partial Mode ON
#define GC9A01A_NORON 0x13  ///< Normal Display Mode ON

#define GC9A01A_INVOFF 0x20   ///< Display Inversion OFF
#define GC9A01A_INVON 0x21    ///< Display Inversion ON
#define GC9A01A_DISPOFF 0x28  ///< Display OFF
#define GC9A01A_DISPON 0x29   ///< Display ON

#define GC9A01A_CASET 0x2A ///< Column Address Set
#define GC9A01A_PASET 0x2B ///< Page Address Set
#define GC9A01A_RAMWR 0x2C ///< Memory Write

#define GC9A01A_PTLAR 0x30    ///< Partial Area
#define GC9A01A_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define GC9A01A_TEOFF 0x34    ///< Tearing effect line off
#define GC9A01A_TEON 0x35     ///< Tearing effect line on
#define GC9A01A_MADCTL 0x36   ///< Memory Access Control
#define GC9A01A_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define GC9A01A_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define GC9A01A1_DFUNCTR 0xB6 ///< Display Function Control

#define GC9A01A_VREG1A 0xC3 ///< Vreg1a voltage control
#define GC9A01A_VREG1B 0xC4 ///< Vreg1b voltage control
#define GC9A01A_VREG2A 0xC9 ///< Vreg2a voltage control

#define GC9A01A_RDID1 0xDA ///< Read ID 1
#define GC9A01A_RDID2 0xDB ///< Read ID 2
#define GC9A01A_RDID3 0xDC ///< Read ID 3

#define GC9A01A_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define GC9A01A_GMCTRN1 0xE1 ///< Negative Gamma Correction
#define GC9A01A_FRAMERATE 0xE8 ///< Frame rate control

#define GC9A01A_INREGEN2 0xEF ///< Inter register enable 2
#define GC9A01A_GAMMA1 0xF0 ///< Set gamma 1
#define GC9A01A_GAMMA2 0xF1 ///< Set gamma 2
#define GC9A01A_GAMMA3 0xF2 ///< Set gamma 3
#define GC9A01A_GAMMA4 0xF3 ///< Set gamma 4
#define GC9A01A_INREGEN1 0xFE ///< Inter register enable 1

// Color definitions
#define GC9A01A_BLACK 0x0000       ///<   0,   0,   0
#define GC9A01A_NAVY 0x000F        ///<   0,   0, 123
#define GC9A01A_DARKGREEN 0x03E0   ///<   0, 125,   0
#define GC9A01A_DARKCYAN 0x03EF    ///<   0, 125, 123
#define GC9A01A_MAROON 0x7800      ///< 123,   0,   0
#define GC9A01A_PURPLE 0x780F      ///< 123,   0, 123
#define GC9A01A_OLIVE 0x7BE0       ///< 123, 125,   0
#define GC9A01A_LIGHTGREY 0xC618   ///< 198, 195, 198
#define GC9A01A_DARKGREY 0x7BEF    ///< 123, 125, 123
#define GC9A01A_BLUE 0x001F        ///<   0,   0, 255
#define GC9A01A_GREEN 0x07E0       ///<   0, 255,   0
#define GC9A01A_CYAN 0x07FF        ///<   0, 255, 255
#define GC9A01A_RED 0xF800         ///< 255,   0,   0
#define GC9A01A_MAGENTA 0xF81F     ///< 255,   0, 255
#define GC9A01A_YELLOW 0xFFE0      ///< 255, 255,   0
#define GC9A01A_WHITE 0xFFFF       ///< 255, 255, 255
#define GC9A01A_ORANGE 0xFD20      ///< 255, 165,   0
#define GC9A01A_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define GC9A01A_PINK 0xFC18        ///< 255, 130, 198


/* MADCTL register fields. */
#define GC9A01A_MADCTL_MY BIT(7U)
#define GC9A01A_MADCTL_MX BIT(6U)
#define GC9A01A_MADCTL_MV BIT(5U)
#define GC9A01A_MADCTL_ML BIT(4U)
#define GC9A01A_MADCTL_BGR BIT(3U)
#define GC9A01A_MADCTL_MH BIT(2U)

/* PIXSET register fields. */
#define GC9A01A_PIXSET_RGB_18_BIT 0x60
#define GC9A01A_PIXSET_RGB_16_BIT 0x50
#define GC9A01A_PIXSET_MCU_18_BIT 0x06
#define GC9A01A_PIXSET_MCU_16_BIT 0x05

/* pg 103 section 6.4.2 of datasheet */
#define GC9A01A_SLEEP_OUT_TIME 120

/** Command/data GPIO level for commands. */
#define GC9A01A_CMD 0U
/** Command/data GPIO level for data. */
#define GC9A01A_DATA 1U

/*Configuration data struct.*/
struct gc9a01a_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec cmd_data;
    struct gpio_dt_spec reset;
    struct pwm_dt_spec backlight;
    uint8_t pixel_format;
	  uint16_t rotation;
	  uint16_t x_resolution;
	  uint16_t y_resolution;
	  bool inversion;
    const void *regs;
    int (*regs_init_fn)(const struct device *dev);
};

/* GC9A01A registers to be intitialized*/
struct gc9a01a_regs {
    uint8_t reg_arr[222];
};

#define GC9A01A_REGS_INIT(n) \
  static const struct gc9a01a_regs gc9a01a_regs_##n = { \
    .reg_arr={\
    GC9A01A_INREGEN1, 0,\
    GC9A01A_INREGEN2, 0,\
    0xEB, 1, 0x14,\
    0x84, 1, 0x40,\
    0x85, 1, 0xFF,\
    0x86, 1, 0xFF,\
    0x87, 1, 0xFF,\
    0x88, 1, 0x0A,\
    0x89, 1, 0x21,\
    0x8A, 1, 0x00,\
    0x8B, 1, 0x80,\
    0x8C, 1, 0x01,\
    0x8D, 1, 0x01,\
    0x8E, 1, 0xFF,\
    0x8F, 1, 0xFF,\
    0xB6, 2, 0x00,0x20,\
    0x90, 4, 0x08, 0x08, 0x08, 0x08,\
    0xBD, 1, 0x06,\
    0xBC, 1, 0x00,\
    0xFF, 3, 0x60, 0x01, 0x04,\
    GC9A01A_VREG1A,1, 0x13,\
    GC9A01A_VREG1B,1, 0x13,\
    GC9A01A_VREG2A,1, 0x22,\
    0xBE, 1, 0x11,\
    GC9A01A_GMCTRN1, 2, 0x10, 0x0E,\
    0xDF, 3, 0x21, 0x0c, 0x02,\
    GC9A01A_GAMMA1, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,\
    GC9A01A_GAMMA2, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,\
    GC9A01A_GAMMA3, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,\
    GC9A01A_GAMMA4, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,\
    0xED, 2, 0x1B, 0x0B,\
    0xAE, 1, 0x77,\
    0xCD, 1, 0x63,\
    0x70, 9, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03,\
    GC9A01A_FRAMERATE, 1, 0x34,\
    0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,\
    0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,\
    0x64, 7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,\
    0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,\
    0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,\
    0x74, 7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,\
    0x98, 2, 0x3e, 0x07,\
    GC9A01A_TEON, 0,\
    GC9A01A_SLPOUT, 0x80,\
    GC9A01A_DISPON, 0x80,\
    0x00 }\
  };
  
int gc9a01a_regs_init(const struct device *dev);

int gc9a01a_transmit(const struct device *dev, uint8_t cmd,
		     const void *tx_data, size_t tx_len);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_GC9A01A_H_ */
