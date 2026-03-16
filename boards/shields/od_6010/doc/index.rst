.. Copyright 2026 NXP
..
.. SPDX-License-Identifier: Apache-2.0

.. _od_6010_shield:

OD-6010 SLCD Panel Shield
==========================

Overview
--------

The OD-6010 is a 7-segment LCD display panel shield that provides a 6-position,
7-segment numeric display with 6 additional dot indicators. The panel is designed
to be used with the Segmented LCD controller.

**Display Characteristics:**

- **Display Type**: 7-segment numeric display
- **Positions**: 6 digit positions (digits 1-6, right to left per datasheet)
- **Segments**: 7 segments per position (A, B, C, D, E, F, G)
- **Indicators**: 6 dot indicators (P1-P6) placed at inter-digit gaps
- **Common Lines**: 4 COM lines (COM0, COM1, COM2, COM3) for multiplexing
- **Total Pins**: 12 front-plane segment pins (pins 0-11)

Hardware Pinout
---------------

The OD-6010 shield uses the following pin-to-signal mapping:

::

      |PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7|PIN8|PIN9|PIN10|PIN11
 -----+----+----+----+----+----+----+----+----+----+----+-----+-----
 COM0 | 1A | P6 | 2A | P3 | 3A | P2 | 4A | P1 | 5A | P4 | 6A  | P5
 COM1 | 1B | 1F | 2B | 2F | 3B | 3F | 4B | 4F | 5B | 5F | 6B  | 6F
 COM2 | 1C | 1G | 2C | 2G | 3C | 3G | 4C | 4G | 5C | 5G | 6C  | 6G
 COM3 | 1D | 1E | 2D | 2E | 3D | 3E | 4D | 4E | 5D | 5E | 6D  | 6E

7-Segment Display Mapping
--------------------------

Each of the 6 positions contains a 7-segment display with segments labeled A-G:

::

     AAA
    F   B
     GGG
    E   C
     DDD

The 7-segment display can represent:

- **Numbers**: 0-9

Display Layout
--------------

The 6 digit positions are arranged left to right.

::

   __   __  P1  __  P2  __  P3  __   __
  |__| |__|    |__|    |__|    |__| |__|
  |__| |__| P4 |__| P5 |__| P6 |__| |__|
  dig6 dig5    dig4    dig3    dig2 dig1

Dot Indicators
--------------

The shield includes 6 dot indicators (P1-P6) used as separator dots between digit
positions. All indicators are driven at COM0. P1-P3 are upper dots and P4-P6 are
lower dots at the same gaps:

- **P1**: Upper dot between dig5 and dig4 (gap 1), Pin 7, COM0
- **P2**: Upper dot between dig4 and dig3 (gap 2), Pin 5, COM0
- **P3**: Upper dot between dig3 and dig2 (gap 3), Pin 3, COM0
- **P4**: Lower dot between dig5 and dig4 (gap 1), Pin 9, COM0
- **P5**: Lower dot between dig4 and dig3 (gap 2), Pin 11, COM0
- **P6**: Lower dot between dig3 and dig2 (gap 3), Pin 1, COM0

Writing a ``:`` character at a digit position activates both the upper and lower dot
at that gap. Writing a ``.`` activates only the lower dot.
