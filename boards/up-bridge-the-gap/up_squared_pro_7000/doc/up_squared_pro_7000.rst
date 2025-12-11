.. zephyr:board:: up_squared_pro_7000

Overview
********

UP Squared Pro 7000 is the 3rd generation of palm-sized developer board of
UP Boards series. UP Squared Pro 7000 is powered by Intel Alder Lake N
(Intel N-series Platform).

For more information about Intel N-series Platform please refer to
:ref:`intel_adl_n`.

This board configuration enables kernel support for the UP Squared Pro 7000 boards.

Hardware
********

General information about the board can be found at the `UP Squared Pro 7000`_ website.

Connections and IOs
===================

Refer to the `UP Squared Pro 7000`_ website for more information.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use the following procedures for booting an image for an UP Squared Pro 7000 board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for UP Squared Pro 7000 board:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: up_squared_pro_7000
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Connect Serial Console
======================

Current board configuration assumes that serial console is connected to
connector ``CN14 USB 2.0/UART 1x10P Wafer``. Refer to `User Manual`_ for
description of the connector and location on the board.

Refer to `UP Serial Console`_ for additional information about serial
connection setup.

Booting the UP Squared Pro 7000 Board using UEFI
================================================

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

Booting the UP Squared Pro 7000 Board over network
==================================================

.. include:: ../../../intel/common/net_boot.rst
   :start-after: start_include_here

References
**********

.. target-notes::

.. _UP Squared Pro 7000: https://up-board.org/up-squared-pro-7000/
.. _User Manual: https://downloads.up-community.org/download/up-squared-pro-7000-user-manual/
.. _UP Serial Console: https://github.com/up-board/up-community/wiki/Serial-Console
