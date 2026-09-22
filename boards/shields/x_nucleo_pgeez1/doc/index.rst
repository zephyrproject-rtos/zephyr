.. SPDX-FileCopyrightText: Copyright 2026 EXALT Technologies
.. SPDX-License-Identifier: Apache-2.0

.. _x_nucleo_pgeez1_shield:

X-NUCLEO-PGEEZ1 page EEPROM expansion board
###########################################

Overview
********

The X-NUCLEO-PGEEZ1 expansion board carries an ST M95P32 32-Mbit SPI
page EEPROM. The device supports single, dual, and quad output reads and
NOR-compatible array operations described by SFDP.

.. figure:: img/x_nucleo_pgeez1.webp
   :align: center
   :alt: X-NUCLEO-PGEEZ1

Zephyr exposes the M95P32 main memory array through the Flash API over MSPI.
The shield configuration uses a 1-1-4 quad read and a single-line page
program because the M95P32 does not provide a quad page-program command.
The M95P32 is compatible with the generic ``jedec,nor`` MSPI driver.

More information about the expansion board is available on the
`X-NUCLEO-PGEEZ1 product page`_.

Requirements
************

Until STM32-based boards are fully migrated to the MSPI API, the shield
requires a board that provides the ``st_zio_qspi`` connector. Currently, the
:zephyr:board:`nucleo_u575zi_q` is supported.

Programming
***********

Build the MSPI flash sample with the shield enabled:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mspi/mspi_flash
   :board: nucleo_u575zi_q
   :shield: x_nucleo_pgeez1
   :goals: build

On the :zephyr:board:`nucleo_u575zi_q`, the shield replaces the board's
internal secondary MCUboot slot with a partition on the M95P32 and selects
the UART used by MCUmgr. The board-specific shield configuration uses an
8-KiB page layout so the external layout matches the STM32U575 internal
flash layout. Swap using offset also requires
``SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET=y`` in the sysbuild configuration.

For example, build the MCUmgr SMP server with MCUboot as follows:

.. code-block:: console

   west build --sysbuild -b nucleo_u575zi_q \
     samples/subsys/mgmt/mcumgr/smp_svr --shield x_nucleo_pgeez1 -- \
     -DEXTRA_CONF_FILE=serial.conf \
     -DSB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET=y

.. _X-NUCLEO-PGEEZ1 product page:
   https://www.st.com/en/evaluation-tools/x-nucleo-pgeez1.html
