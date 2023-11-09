:orphan:

.. _up_squared_pro_7000_board:

UP SQUARED PRO 7000 board
#########################

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

General information about the board can be found at the `UP_SQUARED_PRO_7000`_ website.

Connections and IOs
===================

Refer to the `UP_SQUARED_PRO_7000`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an UP SQUARED PRO 7000 board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for UP SQUARED PRO 7000 board:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: up_squared_pro_7000
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Booting the UP Squared Pro 7000 Board using UEFI
================================================

.. include:: ../../common/efi_boot.rst
   :start-after: start_include_here

Booting the UP Squared Pro 7000 Board over network
==================================================

.. include:: ../../common/net_boot.rst
   :start-after: start_include_here

.. _UP_SQUARED_PRO_7000: https://up-board.org/up-squared-pro-7000/
