.. Copyright 2026 Renato Mauro
..
.. SPDX-License-Identifier: Apache-2.0

.. _st_gh08172t_shield:

ST GH08172T SLCD Panel Shield
=============================

Overview
--------

The GH08172T is a 6-digit 14-segment LCD panel with 12 indicators.
Hardware Configuration:
- 6 Display Positions (digits and letters 1-6, left to right in datasheet)
- 14 Segments per Position (A, B, C, D, E, F, G, H, J, K, M, N, P, Q)
- 4 physical COM lines (COM0-COM3), 8 logical COM lines handled via 8 RAM
registers
- 24 physical SEG lines (SEG0-SEG23)
- 28 physical pins (Pin 1-28), no VDD and GND, the LCD can be mounted
upside-down, in that case the MCU-PIN to LCD-PIN mapping changes, but the
panel logic is the same
- 8 dot indicators (DP11-DP4, COL1-COL4): dot and double dot indicators
between digit positions 1-5, not available between positions 5 and 6
- 4 bar indicators at the right side of position 6

Supported characters

ASCII characters:
    - numbers:                0..9
    - uppercase letters:      A..Z
    - operators:              +  -  *  /
    - symbols:                Space (' ': 0x20)  _  (  )
    - indicators (pos 1..4):  . (dot)  : (double dot)

Not ASCII indicators:
    - pos 1..4:               Triple dot (both dot and double dot)
    - pos 6:                  4 independent bars

Custom characters (see sample auxdisplay_14seg):
    - unit prefixes:          d  c  m  u  n
    - other symbols:          Degree  Low ring  Full

GLASS LCD MAPPING
The LCD has six 14-segment digits with point/colon and 4 bars:::

      1       2       3       4       5       6
    -----   -----   -----   -----   -----   -----
    |\|/| o |\|/| o |\|/| o |\|/| o |\|/|   |\|/|   BAR3
    -- --   -- --   -- --   -- --   -- --   -- --   BAR2
    |/|\| o |/|\| o |/|\| o |/|\| o |/|\|   |/|\|   BAR1
    ----- * ----- * ----- * ----- * -----   -----   BAR0

LCD segment mapping:::

     Pos 1..6     Pos 1..4                 Pos 6

    -----A-----
    |\   |   /|        _                  _______
    F H  J  K B   COL |_|           BAR3 |_______|  (same as pos 5 COL)
    |  \ | /  |                           _______
    --G-- --M--        _            BAR2 |_______|  (some as pos 5 DP)
    |  / | \  |   COL |_|                 _______
    E Q  P  N C                     BAR1 |_______|  (same as pos 6 COL)
    |/   |   \|        _                  _______
    -----D-----   DP  |_|           BAR0 |_______|  (some as pos 6 DP)

Summary for all positions (LCD pin to LCD crystal, sorted by LCD pin)

====== ==== ====== ====== ====== ======
LCD    LCD  COM3   COM2   COM1   COM0
Pin    Pin
name
====== ==== ====== ====== ====== ======
SEG0   1    1N     1P     1D     1E
SEG1   2    1DP    1COL   1C     1M
SEG2   3    2N     2P     2D     2E
SEG3   4    2DP    2COL   2C     2M
SEG4   5    3N     3P     3D     3E
SEG5   6    3DP    3COL   3C     3M
SEG6   7    4N     4P     4D     4E
SEG7   8    4DP    4COL   4C     4M
SEG8   9    5N     5P     5D     5E
SEG9   10   BAR2   BAR3   5C     5M
SEG10  11   6N     6P     6D     6E
SEG11  12   BAR0   BAR1   6C     6M

COM3   13
COM2   14          COM2
COM1   15                 COM1
COM0   16                        COM0

SEG12  17   6J     6K     6A     6B
SEG13  18   6H     6Q     6F     6G
SEG14  19   5J     5K     5A     5B
SEG15  20   5H     5Q     5F     5G
SEG16  21   4J     4K     4A     4B
SEG17  22   4H     4Q     4F     4G
SEG18  23   3J     3K     3A     3B
SEG19  24   3H     3Q     3F     3G
SEG20  25   2J     2K     2A     2B
SEG21  26   2H     2Q     2F     2G
SEG22  27   1J     1K     1A     1B
SEG23  28   1H     1Q     1F     1G
====== ==== ====== ====== ====== ======

Truly it's more complicated, because each COM is able to drive
more than 32 bits; so, if bits 0-31 are driven by COM0, bits
32-63 (actually, in ST sheets, 38 for segments and 43 for shifts)
are driven by a second register, named COM0_1. This means that
a logical 63 bit set is handled via two 32 bit sets driven by
two registers. This is rapresented in the ST manual by MFU AF11
channel having a name whose number is greater than 31.
This happens for positions 4 and 5 only, for SEG 7, 8, 15 and 16
for channels 33, 35, 34 and 32.

    COM   0-31  32-63
     0      0     1
     1      2     3
     2      4     5
     3      6     7

===== ====== ======== ===== =========== ===== ========= =====
LCD   GPIO    LCD     LCD   MCU         Bit   RAM       Bit
Pin   port    Pin     Pin   AF11        posi  regi      posi
      pin     name    name  channel     tion  ster      tion
                      id                                % 32
===== ====== ======== ===== =========== ===== ========= =====
1     A07    SEG00    00    LCD_SEG4    4     even       4
2     C05    SEG01    01    LCD_SEG23   23    even       23
3     B01    SEG02    02    LCD_SEG6    6     even       6
4     B13    SEG03    03    LCD_SEG13   13    even       13
5     B15    SEG04    04    LCD_SEG15   15    even       15
6     D09    SEG05    05    LCD_SEG29   29    even       29
7     D11    SEG06    06    LCD_SEG31   31    even       31
8     D13    SEG07    07    LCD_SEG33   33    odd        1
9     D15    SEG08    08    LCD_SEG35   35    odd        3
10    C07    SEG09    09    LCD_SEG25   25    even       25
11    A15    SEG10    10    LCD_SEG17   17    even       17
12    B04    SEG11    11    LCD_SEG8    8     even       8

13    B09    COM03          LCD_COM3          Com line
14    A10    COM02          LCD_COM2          Com line
15    A09    COM01          LCD_COM1          Com line
16    A08    COM00          LCD_COM0          Com line

17    B05    SEG12    12    LCD_SEG9    9     even       9
18    C08    SEG13    13    LCD_SEG26   26    even       26
19    C06    SEG14    14    LCD_SEG24   24    even       24
20    D14    SEG15    15    LCD_SEG34   34    odd        2
21    D12    SEG16    16    LCD_SEG32   32    odd        0
22    D10    SEG17    17    LCD_SEG30   30    even       30
23    D08    SEG18    18    LCD_SEG28   28    even       28
24    B14    SEG19    19    LCD_SEG14   14    even       14
25    B12    SEG20    20    LCD_SEG12   12    even       12
26    B00    SEG21    21    LCD_SEG5    5     even       5
27    C04    SEG22    22    LCD_SEG22   22    even       22
28    A06    SEG23    23    LCD_SEG3    3     even       3
===== ====== ======== ===== =========== ===== ========= =====
