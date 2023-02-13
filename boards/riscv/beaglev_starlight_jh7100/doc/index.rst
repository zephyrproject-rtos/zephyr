.. _beaglev_starlight_jh7100:

BeagleV Starlight JH7100
########################

Overview
********

The BeagleV Starlight is an 64-bit open-source RISC-V development board with
a StarFive JH7100 SoC.

Programming and debugging
*************************

Building
========

Applications for the ``beaglev_starlight_jh7100`` board configuration can be built
as usual (see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: beaglev_starlight_jh7100
   :goals: build

The bootloader expects size information at the start of the binary file,
so the bin file needs to be processed first to include that information.
Download the helper script from starfive-tech github repo `here
<https://github.com/starfive-tech/freelight-u-sdk/blob/starfive/fsz.sh>`_

.. code-block:: console

   ./fsz.sh build/zephyr/zephyr.bin

This will create a new file build/zephyr/zephyr.bin.out that can be flashed.

Flashing
========

.. note::
   The following steps use minicom for serial communication, feel free to use
   any other serial terminal that supports xmodem based file transfers.

#. BeagleV Starlight uses uart for flashing. Refer to `BeagleV Getting Started
   <https://web.archive.org/web/20210625011721/https://wiki.seeedstudio.com/BeagleV-Getting-Started/>`_
   to connect your serial-to-usb converter. Now power on the board and using
   minicom access board's serial.

   .. code-block:: console

      minicom -D /dev/ttyUSB0 -b 115200

#. Press any key to stop the boot sequence. This will output a menu

   .. code-block:: console

      ***************************************************
      *************** FLASH PROGRAMMING *****************
      ***************************************************

      0:update uboot
      1:quit
      select the function:

#. Select 0 to flash a new image.
#. Press Ctrl+A and then press s to enter upload mode
#. Select xmodem and press Enter
#. Select Goto from the bottom tab menu and press Enter
#. Enter the directory path and press Enter
#. Select zephyr.bin.out by navigating using arrow keys, press Space and press Enter
#. Once uploaded hit any key to continue and reset the board to boot the zephyr binary
