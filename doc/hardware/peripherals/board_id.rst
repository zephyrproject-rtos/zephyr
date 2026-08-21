.. Copyright (c) 2026 The Zephyr Project Contributors
.. Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

.. _board_id_api:

Board Identity
##############

Overview
********

The board-id API reads a raw identity value that a board encodes in
hardware - strap resistors on a few GPIO pins, a resistor divider, an
EEPROM word - so a single firmware image can recognize which hardware
revision it is running on at runtime.

This is a different problem from Zephyr's build-time
:ref:`board revisions <porting_board_revisions>`: that mechanism binds one
image to one revision at build time, while board-id lets one already-built
image distributed by FOTA identify the revision it landed on.

The API returns the **raw** value as the board encodes it, not a semantic
meaning ("v3.2 with the radio populated"). Mapping the raw value to
something meaningful is application or board-integrator policy and stays
out of this driver class on purpose.

A driver may read its backend once at init and always return that cached
value; :c:func:`board_id_read` documents this per call, and the GPIO backend
below does exactly that, since strap pins do not change while the board is
powered.

Configuration Options
**********************

Related configuration options:

* :kconfig:option:`CONFIG_BOARD_ID`
* :kconfig:option:`CONFIG_BOARD_ID_GPIO`
* :kconfig:option:`CONFIG_BOARD_ID_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: board_id_interface
