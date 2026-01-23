.. Copyright 2025 NXP
..
.. SPDX-License-Identifier: Apache-2.0

.. _od_6010_shield:

OD-6010 SLCD Panel Shield
==========================

Overview
--------

The OD-6010 is a 7-segment LCD display panel shield that provides a 6-position,
7-segment numeric display with 6 additional icon indicators. The panel is designed
to be used with the Segmented LCD controller.

**Display Characteristics:**

- **Display Type**: 7-segment alphanumeric display
- **Positions**: 6 digit positions (labeled 1-6 from left to right)
- **Segments**: 7 segments per position (A, B, C, D, E, F, G)
- **Icons**: 6 individual icons/indicators (P1, P2, P3, P4, P5, P6)
- **Common Lines**: 4 COM lines (COM0, COM1, COM2, COM3) for multiplexing
- **Total Pins**: 12 front-plane segment pins (pins 0-11)

Hardware Pinout
---------------

The OD-6010 shield uses the following pin-to-signal mapping:

**COM0 (Common Line 0)**

========  =============  ==========
Pin       Signal         Type
========  =============  ==========
0         1A             Segment
1         P6             Icon
2         2A             Segment
3         P3             Icon
4         3A             Segment
5         P2             Icon
6         4A             Segment
7         P1             Icon
8         5A             Segment
9         P4             Icon
10        6A             Segment
11        P5             Icon
========  =============  ==========

**COM1 (Common Line 1)**

========  =============  ==========
Pin       Signal         Type
========  =============  ==========
0         1B             Segment
1         1F             Segment
2         2B             Segment
3         2F             Segment
4         3B             Segment
5         3F             Segment
6         4B             Segment
7         4F             Segment
8         5B             Segment
9         5F             Segment
10        6B             Segment
11        6F             Segment
========  =============  ==========

**COM2 (Common Line 2)**

========  =============  ==========
Pin       Signal         Type
========  =============  ==========
0         1C             Segment
1         1G             Segment
2         2C             Segment
3         2G             Segment
4         3C             Segment
5         3G             Segment
6         4C             Segment
7         4G             Segment
8         5C             Segment
9         5G             Segment
10        6C             Segment
11        6G             Segment
========  =============  ==========

**COM3 (Common Line 3)**

========  =============  ==========
Pin       Signal         Type
========  =============  ==========
0         1D             Segment
1         1E             Segment
2         2D             Segment
3         2E             Segment
4         3D             Segment
5         3E             Segment
6         4D             Segment
7         4E             Segment
8         5D             Segment
9         5E             Segment
10        6D             Segment
11        6E             Segment
========  =============  ==========

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
- **Letters**: A, B, C, D, E, F, G, H, I, J, L, N, O, P, S, T, U, Y
- **Custom patterns**: Any combination of segments can be displayed

Icon Indicators
---------------

The shield includes 6 individual icon indicators (P1-P6) that can be controlled
independently for displaying status or special symbols:

- **P1**: Icon Indicator 1 (Pin 7, COM0)
- **P2**: Icon Indicator 2 (Pin 5, COM0)
- **P3**: Icon Indicator 3 (Pin 3, COM0)
- **P4**: Icon Indicator 4 (Pin 9, COM0)
- **P5**: Icon Indicator 5 (Pin 11, COM0)
- **P6**: Icon Indicator 6 (Pin 1, COM0)

Usage
-----

To use the OD-6010 shield, include it in your device tree configuration:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,slcd-panel = &od_6010;
       };
   };

API Usage Example
-----------------

Display a number at position 0:

.. code-block:: c

   #include <zephyr/drivers/slcd_panel.h>

   const struct device *panel = DEVICE_DT_GET(DT_NODELABEL(od_6010));

   /* Display digit '5' at position 0 */
   slcd_panel_show_number(panel, 0, 5, true);

   /* Clear position 0 */
   slcd_panel_show_number(panel, 0, 5, false);

Display a letter at position 2:

.. code-block:: c

   /* Display letter 'A' at position 2 */
   slcd_panel_show_letter(panel, 2, 'A', true);

Control an icon:

.. code-block:: c

   /* Enable icon P1 (index 0) */
   slcd_panel_show_icon(panel, 0, true);

   /* Disable icon P1 */
   slcd_panel_show_icon(panel, 0, false);

Device Tree Binding
-------------------

The OD-6010 shield uses the `slcd-panel` binding. For complete documentation,
see :ref:`slcd-panel` binding documentation.

**Key Properties:**

- ``compatible``: Must be "slcd-panel" or a specific OD-6010 compatible string
- ``segment-type``: Set to "7-segment"
- ``num-positions``: 6 (number of display positions)
- ``num-icons``: 6 (number of icon indicators)
- ``num-coms``: 4 (number of common lines)
- ``num-pins``: 12 (number of front-plane pins)
- ``segment-mux``: Array of 42 values (6 positions × 7 segments)
- ``icon-mux``: Array of 6 values (one per icon)

Configuration Options
---------------------

Enable the segment7 driver for 7-segment displays:

.. code-block:: kconfig

   CONFIG_SLCD_PANEL=y
   CONFIG_SLCD_PANEL_SEGMENT7=y

References
----------

- :ref:`slcd_interface` - SLCD Controller Interface
- :ref:`slcd_panel_interface` - SLCD Panel Interface
- NXP SLCD Hardware Documentation (TODO: Add specific reference)
