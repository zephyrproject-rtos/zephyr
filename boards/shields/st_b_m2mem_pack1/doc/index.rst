.. Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>
.. SPDX-License-Identifier: Apache-2.0

.. _st_b_m2mem_pack1_shield:

ST B-M2MEM-PACK1 M.2 serial memory pack
#######################################

Overview
********

The ST B-M2MEM-PACK1 is a family of M.2 serial memory add-on boards for
Nucleo-144 development boards that expose the ST M.2 Key A serial memory
connector.

In Zephyr, each populated memory module is modeled as a separate shield variant.
All variants share a common identification EEPROM on the M.2 I2C bus and expose
one serial NOR flash device on the M.2 XSPI bus.

Supported variants
******************

.. list-table::
   :header-rows: 1

   * - Shield
     - Module
     - Serial NOR
     - Bus mode
     - Capacity
     - IO rail
   * - ``b_m2mem_pack1_mb1927_33ba``
     - MB1927-33BA
     - MX25LM51245GXDI00
     - Octal STR
     - 512 Mbit
     - 3.3 V
   * - ``b_m2mem_pack1_mb1927_18ba``
     - MB1927-18BA
     - MX25UW25645GXDI00
     - Octal STR
     - 256 Mbit
     - 1.8 V
   * - ``b_m2mem_pack1_mb1928_33la``
     - MB1928-33LA
     - IS25LP032DJNLE-TR
     - Quad STR
     - 32 Mbit
     - 3.3 V
   * - ``b_m2mem_pack1_mb1928_33lb``
     - MB1928-33LB
     - W25Q16JVSNIQ
     - Quad STR
     - 16 Mbit
     - 3.3 V

Requirements
************

This shield family can be used with any board that provides the ST M.2 serial
memory connector and exports the board-side Devicetree labels used by the
shields:

- ``m2mem_i2c`` for the identification EEPROM bus
- ``m2mem_xspi`` for the serial NOR interface
- ``m2mem_ldo_en`` for module power-cycle/reset control

The current in-tree host-board implementation is ``nucleo_u3c5zi_q``.

Programming
***********

Select the wanted shield variant when invoking ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u3c5zi_q
   :shield: b_m2mem_pack1_mb1928_33lb
   :goals: build
   :compact:

To build one of the other modules, replace the shield name with one of:

- ``b_m2mem_pack1_mb1927_33ba``
- ``b_m2mem_pack1_mb1927_18ba``
- ``b_m2mem_pack1_mb1928_33la``
- ``b_m2mem_pack1_mb1928_33lb``

Regardless of the selected variant, the shield exposes its serial NOR device
under the stable ``m2mem_flash`` Devicetree label, so an application or sample
can target it consistently (for example through a ``flash0`` alias). The device
can be explored with the :zephyr:code-sample:`flash-shell` sample. The common
identification EEPROM is described in Devicetree and can be accessed by
applications through the :ref:`EEPROM driver API <eeprom_api>`.

References
**********

ST documentation for the pack and connector includes:

- B-M2MEM-PACK1 data brief
- `UM3611`_, M.2 serial memory pack for Nucleo-144 boards
- `TN1618`_, M.2 serial memory interface specification

.. target-notes::

.. _UM3611:
  https://www.st.com/resource/en/user_manual/um3611-m2-serial-memory-pack-for-nucleo144-boards-stmicroelectronics.pdf

.. _TN1618:
  https://www.st.com/resource/en/technical_note/tn1618-m2serialmemory-interface-specification-stmicroelectronics.pdf
