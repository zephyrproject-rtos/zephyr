/*
 * Copyright (c) 2026 Jasper Jonker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_US2066_H_
#define ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_US2066_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * US2066 commands for auxdisplay_custom_command().
 *
 * The command is selected with auxdisplay_custom_data.options. The data payload
 * is one byte for scroll enable, scroll quantity, shift enable, and display
 * shift; two bytes for fade/blink and double-height; and empty for clear
 * double-height.
 */
enum auxdisplay_us2066_custom_command {
	AUXDISPLAY_US2066_CUSTOM_SET_SCROLL_ENABLE = 0,
	AUXDISPLAY_US2066_CUSTOM_SET_SCROLL_QUANTITY = 1,
	AUXDISPLAY_US2066_CUSTOM_SET_SHIFT_ENABLE = 2,
	AUXDISPLAY_US2066_CUSTOM_DISPLAY_SHIFT = 3,
	AUXDISPLAY_US2066_CUSTOM_SET_FADE_BLINK = 4,
	AUXDISPLAY_US2066_CUSTOM_SET_DOUBLE_HEIGHT = 5,
	AUXDISPLAY_US2066_CUSTOM_CLEAR_DOUBLE_HEIGHT = 6,
};

#define AUXDISPLAY_US2066_LINE_1                 BIT(0)
#define AUXDISPLAY_US2066_LINE_2                 BIT(1)
#define AUXDISPLAY_US2066_LINE_3                 BIT(2)
#define AUXDISPLAY_US2066_LINE_4                 BIT(3)

#define AUXDISPLAY_US2066_SCROLL_QUANTITY_MAX    48

#define AUXDISPLAY_US2066_DISPLAY_SHIFT_LEFT     0
#define AUXDISPLAY_US2066_DISPLAY_SHIFT_RIGHT    1

#define AUXDISPLAY_US2066_DOUBLE_HEIGHT_LOWER    BIT(2)
#define AUXDISPLAY_US2066_DOUBLE_HEIGHT_UPPER    BIT(3)

#define AUXDISPLAY_US2066_FADE_BLINK_DISABLE     0x00
#define AUXDISPLAY_US2066_FADE_OUT_ENABLE        0x20
#define AUXDISPLAY_US2066_BLINK_ENABLE           0x30

#define AUXDISPLAY_US2066_FADE_INTERVAL(n)       ((n) & 0x0F)
#define AUXDISPLAY_US2066_FADE_INTERVAL_MAX      15

#define AUXDISPLAY_US2066_FADE_8_FRAMES          0x00
#define AUXDISPLAY_US2066_FADE_64_FRAMES         0x07
#define AUXDISPLAY_US2066_FADE_128_FRAMES        0x0F

/* --- Function Set to enable/disable RE and IS bits + extra --- */
#define US2066_CMD_FUNCTION_SET          0x20   /* Base command */
#define US2066_FUNC_IS_BASIC             0x00   /* Only when RE=0. IS=0: Fundamental command set */
#define US2066_FUNC_IS_EXTENDED          BIT(0) /* Only when RE=0. IS=1: Extended command set */
#define US2066_FUNC_RE_BASIC             0x00   /* Set RE=0: Basic instruction set */
#define US2066_FUNC_RE_EXTENDED          BIT(1) /* Set RE=1: Extended instruction set */
#define US2066_FUNC_1_3_LINES            0x00   /* Both RE=0 and RE=1 */
#define US2066_FUNC_2_4_LINES            BIT(3) /* Both RE=0 and RE=1 */
#define US2066_FUNC_DOUBLE_HEIGHT_ENABLE BIT(2) /* Only when RE=0 */
#define US2066_FUNC_CGRAM_BLINK_ENABLE   BIT(2) /* Only when RE=1 */
#define US2066_FUNC_REVERSE_DISPLAY      BIT(0) /* Only when RE=1 */

/* --- Extended Function Set (RE=1 Required) --- */
#define US2066_CMD_FUNCTION_SET_EXTENDED BIT(3) /* Base command */
#define US2066_FUNC_EXT_FONT_WIDTH_6     BIT(2) /* 6-dot font width */
#define US2066_FUNC_EXT_FONT_WIDTH_5     0x00   /* 5-dot font width */
#define US2066_FUNC_EXT_INVERT_ENABLE    BIT(1) /* black/white inverting of cursor enable */
#define US2066_FUNC_EXT_INVERT_DISABLE   0x00   /* black/white inverting of cursor disable */
#define US2066_FUNC_EXT_LINES_3_4        BIT(0) /* 3/4 lines Display */
#define US2066_FUNC_EXT_LINES_1_2        0x00   /* 1/2 lines Display */

/* --- Function Selection A (RE=1) --- */
#define US2066_CMD_FUNC_SELECTION_A      0x71 /* Command */
#define US2066_FSA_VDD_5V                0x5C /* Enable internal regulator (5V I/O) */
#define US2066_FSA_VDD_3V3               0x00 /* Disable internal VDD regulator (3V3 I/O) */

/* --- Function Selection B (RE=1) --- */
#define US2066_CMD_FUNC_SELECTION_B      0x72 /* Command */
#define US2066_FSB_CGRAM_8_A             0x00 /* 240 CGROM, 8 CGRAM */
#define US2066_FSB_CGRAM_8_B             0x01 /* 248 CGROM, 8 CGRAM */
#define US2066_FSB_CGRAM_6               0x02 /* 250 CGROM, 6 CGRAM */
#define US2066_FSB_CGRAM_0               0x03 /* 256 CGROM, 0 CGRAM (all ROM) */
#define US2066_FSB_CGRAM_MASK            0x03

#define US2066_FSB_ROM_A                 0x00 << 2 /* ROM A - Standard ASCII character set */
#define US2066_FSB_ROM_B                 0x01 << 2 /* ROM B - Alternate character set */
#define US2066_FSB_ROM_C                 0x02 << 2 /* ROM C - Alternate character set */
#define US2066_FSB_ROM_MASK              0x03 << 2

/* --- Function Selection C (RE=1) --- */
#define US2066_OLED_FUNC_SELECTION_C     0xDC
#define US2066_FSC_VSL_INTERNAL          0x00   /* Internal VSL (POR) */
#define US2066_FSC_VSL_EXTERNAL          BIT(7) /* External VSL */

#define US2066_FSC_GPIO_HIZ_INPUT_OFF    0x00 /* High-Z, input disabled (reads low) (POR) */
#define US2066_FSC_GPIO_HIZ_INPUT_ON     0x01 /* High-Z, input enabled */
#define US2066_FSC_GPIO_OUTPUT_LOW       0x02 /* GPIO outputs LOW */
#define US2066_FSC_GPIO_OUTPUT_HIGH      0x03 /* GPIO outputs HIGH */

/* --- Clear Display --- */
#define US2066_CMD_CLEAR_DISPLAY         BIT(0)

/* --- Return Home (RE=0) --- */
#define US2066_CMD_RETURN_HOME           BIT(1)

/* --- Entry Mode Set --- */
#define US2066_CMD_ENTRY_MODE_SET        0x04 /* Base command */

/* Entry Mode Flags (RE=0, Fundamental Command Set) - Controls cursor/text flow */
#define US2066_ENTRY_INC                 BIT(1) /* Cursor moves right, DDRAM address increments */
#define US2066_ENTRY_DEC                 0x00   /* Cursor moves left, DDRAM address decrements */
#define US2066_ENTRY_SHIFT               BIT(0) /* Display shifts on character write */
#define US2066_ENTRY_NO_SHIFT            0x00   /* Display stays still */

/* Entry Mode Flags (RE=1, Extended Command Set) - Controls physical display orientation */
#define US2066_ENTRY_BDC                 BIT(1) /* BDC=1: COM0->COM31, BDC=0: COM31->COM0 */
#define US2066_ENTRY_BDS                 BIT(0) /* BDS=1: SEG0->SEG99, BDS=0: SEG99->SEG0 */

/* --- Display ON/OFF Control (RE=0) --- */
#define US2066_CMD_DISPLAY_CTRL          BIT(3) /* Base command */
#define US2066_DISPLAY_BLINK_ON          BIT(0)
#define US2066_DISPLAY_CURSOR_ON         BIT(1)
#define US2066_DISPLAY_ON                BIT(2)

/* --- Cursor or Display Shift (RE=0, IS=0) --- */
#define US2066_CMD_CURSOR_SHIFT          BIT(4) /* Base command */
#define US2066_SHIFT_DISPLAY             BIT(3) /* S/C=1: shift display */
#define US2066_SHIFT_CURSOR              0x00   /* S/C=0: shift cursor */
#define US2066_SHIFT_RIGHT               BIT(2) /* R/L=1: shift right */
#define US2066_SHIFT_LEFT                0x00   /* R/L=0: shift left */

/* --- Double Height (4- Line)/ Display-dot Shift (RE=1) --- */
#define US2066_CMD_DH_DOT_SHIFT          BIT(4) /* Base command */
#define US2066_DH_UD2                    BIT(3) /* UD2=1: upper part double height */
#define US2066_DH_UD1                    BIT(2) /* UD1=1: lower part double height */
#define US2066_DH_DISPLAY_SHIFT          BIT(0) /* DH’=1: display shift enable */
#define US2066_DH_DOT_SHIFT              0x00   /* DH’=0: dot scroll enable (POR) */

/* --- Shift Enable when DH'=1 (IS=1, RE=1) --- */
#define US2066_CMD_SHIFT_ENABLE          BIT(4) /* Base command */
#define US2066_SHIFT_DS4                 BIT(3) /* DS4=1 4th line display shift enable. */
#define US2066_SHIFT_DS3                 BIT(2) /* DS3=1 3rd line display shift enable. */
#define US2066_SHIFT_DS2                 BIT(1) /* DS2=1 2nd line display shift enable. */
#define US2066_SHIFT_DS1                 BIT(0) /* DS1=1 1st line display shift enable. */

/* --- Scroll Enable when DH'=0 (IS=1, RE=1) --- */
#define US2066_CMD_SCROLL_ENABLE         BIT(5) /* Base command */
#define US2066_SCROLL_HS4                BIT(3) /* HS4=1 4th line horizontal scroll enable. */
#define US2066_SCROLL_HS3                BIT(2) /* HS3=1 3rd line horizontal scroll enable. */
#define US2066_SCROLL_HS2                BIT(1) /* HS2=1 2nd line horizontal scroll enable. */
#define US2066_SCROLL_HS1                BIT(0) /* HS1=1 1st line horizontal scroll enable. */
#define US2066_SCROLL_QUANTITY_MAX       48     /* Max scroll quantity in pixels */

/* --- Set CGRAM Address (IS=0, RE=0) --- */
#define US2066_CMD_SET_CGRAM_ADDR        BIT(6)

/* --- Set DDRAM Address (IS=0, RE=0) --- */
#define US2066_CMD_SET_DDRAM_ADDR        BIT(7)

/* --- Set Scroll Quantity (RE=1) --- */
#define US2066_CMD_SET_SCROLL_QTY        BIT(7) /* Base command */

/* --- OLED Characterization (RE=1) --- */
#define US2066_CMD_OLED_DISABLE          0x78 /* SD=0 */
#define US2066_CMD_OLED_ENABLE           0x79 /* SD=1 */

/* --- Set Contrast Control (RE=1) --- */
/*
 * Double byte command to select 1 out of 256 contrast steps.
 * Contrast increases as the value increases.  (POR = 7Fh )
 */
#define US2066_OLED_SET_CONTRAST         0x81

/* --- Set Display Clock Divide Ratio / Oscillator Frequency (RE=1) --- */
#define US2066_OLED_SET_DISPLAY_CLK      0xD5
#define US2066_CLK_DIVIDER(div)          (((div) - 1) & 0x0F)   /* 1-16 as 0-15 */
#define US2066_CLK_FREQ(freq)            (((freq) & 0x0F) << 4) /* 0-15 (POR=0111b) */

/* --- Set Phase Length (RE=1) --- */
#define US2066_OLED_SET_PHASE_LENGTH     0xD9
#define US2066_PHASE1(val)               ((val) & 0x0F) /* Phase 1: 0-15 */
#define US2066_PHASE2(val)               (((val) & 0x0F) << 4) /* Phase 2: 1-15 DCLKs (0 invalid) */

/* --- Set SEG Pins Hardware Configuration (RE=1) --- */
#define US2066_OLED_SET_SEG_PINS         0xDA

/* SEG Pins Configuration Data Byte */
#define US2066_SEG_PINS_SEQUENTIAL       0x00   /* Sequential COM pin configuration */
#define US2066_SEG_PINS_ALTERNATIVE      BIT(4) /* Alternative COM pin configuration (0x10) */
#define US2066_SEG_PINS_REMAP_OFF        0x00   /* Disable COM Left/Right remap (POR) */
#define US2066_SEG_PINS_REMAP_ON         BIT(5) /* Enable COM Left/Right remap (0x20) */

/* --- Set VCOMH Deselect Level (RE=1) --- */
#define US2066_OLED_SET_VCOMH            0xDB
#define US2066_VCOMH_0_65_VCC            0x00
#define US2066_VCOMH_0_71_VCC            0x10
#define US2066_VCOMH_0_77_VCC            0x20
#define US2066_VCOMH_0_83_VCC            0x30
#define US2066_VCOMH_1_00_VCC            0x40

/* --- Set Fade Out and Blinking (RE=1) --- */
#define US2066_OLED_SET_FADE_BLINK       0x23

/* Fade/Blink Mode */
#define US2066_FADE_BLINK_DISABLE        0x00 /* Normal display (POR) */
#define US2066_FADE_OUT_ENABLE           0x20 /* Fade out to black */
#define US2066_BLINK_ENABLE              0x30 /* Fade out then in, loop */

/* Fade/Blink Interval (bits 3-0) - Frames per fade step */
#define US2066_FADE_INTERVAL(n)          ((n) & 0x0F) /* n=0-15: (n+1)*8 frames */
#define US2066_FADE_INTERVAL_MAX         15

/* Common intervals */
#define US2066_FADE_8_FRAMES             0x00 /* Fastest: 8 frames/step */
#define US2066_FADE_64_FRAMES            0x07 /* Medium: 64 frames/step */
#define US2066_FADE_128_FRAMES           0x0F /* Slowest: 128 frames/step */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_US2066_H_ */
