..
   SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
   SPDX-License-Identifier: Apache-2.0

.. stm32wba_default_zephyr_bootloader_start

.. stm32_default_zephyr_bootloader_start

STM32 devices support secure boot with Zephyr using the default
MCUboot-based workflows.
The :zephyr:code-sample:`smp-svr` application is one example that can be used
with MCUboot image-management workflows.

Use an MCUmgr-capable client (`tools and libraries <https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html#tools-libraries>`_).
The following procedure uses `newtmgr <https://mynewt.apache.org/latest/newtmgr/index.html>`_.

Depending on your board capabilities, the ``smp_svr`` application can be used
to update images over a serial UART connection.

Build the sample with MCUboot and the serial MCUmgr transport:

.. parsed-literal::

   west build -b |st_board_target| --sysbuild samples/subsys/mgmt/mcumgr/smp_svr -- \\
     -DEXTRA_CONF_FILE=serial.conf

UART MCUmgr requires the ``zephyr,uart-mcumgr`` chosen node. If the board does
not define it, add an application board overlay under
``samples/subsys/mgmt/mcumgr/smp_svr/boards/``. For example:

.. code-block:: devicetree

   / {
     chosen {
       zephyr,uart-mcumgr = &usart1;
     };
   };

Replace ``&usart1`` with the UART connected to the board serial interface.

Once ``smp_svr`` is flashed, you can use ``newtmgr`` over serial. The examples
below assume the serial device is ``/dev/ttyACM0``:

.. code-block:: console

  # Create the serial connection for your board, here on /dev/ttyACM0
  newtmgr conn add serial type=serial connstring="dev=/dev/ttyACM0,baud=115200"
  # Upload a new signed image to the download slot
  newtmgr -c serial image upload build/smp_svr/zephyr/zephyr.signed.bin
  # List the images and retrieve the hash of the image the download slot
  newtmgr -c serial image list
  # Mark this image as a test image
  newtmgr -c serial image test <new_image_hash>
  # Reset the device for the bootloader to install the image into the primary slot
  newtmgr -c serial reset
  # Once the image is validated, confirm it to make it permanent (skip this step and reset to revert)
  newtmgr -c serial image confirm <new_image_hash>

.. stm32_default_zephyr_bootloader_end

.. stm32wba_default_zephyr_fota_start

The |st_board_name| board also supports secure FOTA over BLE with Zephyr
using the :zephyr:code-sample:`smp-svr` application.

Once ``smp_svr`` is flashed, you can use ``newtmgr`` for FOTA over BLE.
The examples below assume the HCI interface is ``hci0``. Note that internal
HCI devices can sometimes be unreliable.

.. code-block:: console

  # Upload a new signed image to the download slot
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image upload build/smp_svr/zephyr/zephyr.signed.bin
  # List the images and retrieve the hash of the image in the download slot
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image list
  # Mark this image as a test image
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image test <new_image_hash>
  # Reset the device for the bootloader to install the image into the primary slot
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr reset
  # Once the image is validated, confirm it to make it permanent (skip this step and reset to revert)
  newtmgr -i 0 --conntype ble --connstring peer_name=Zephyr image confirm <new_image_hash>

.. stm32wba_default_zephyr_fota_end

.. stm32wba_default_zephyr_bootloader_end

.. stm32wba_low_power_start

|stm32wba_lp_board_name| supports Zephyr power management to enter low-power states
when the application is idle.

To achieve the lowest power consumption, Zephyr ``suspend-to-ram`` leverages
the STM32 Standby mode on this platform.

Power management must be enabled in your application by activating
``CONFIG_PM``:

.. code-block:: none

  CONFIG_PM=y

For suspend-to-ram on STM32WBA, the reference implementation is
:zephyr:code-sample:`ble_peripheral_hr`, used together with
:zephyr_file:`samples/bluetooth/peripheral_hr/boards/stm32wba_pwr_optim.overlay`.

In this overlay, ``sram0`` can be reduced to the minimal usable size to
lower consumption, while ``pm_s2ram`` should remain located at the end of
the usable RAM.

Example build command:

.. parsed-literal::

   west build -b |stm32wba_lp_board_target| samples/bluetooth/peripheral_hr -- \\
     -DDTC_OVERLAY_FILE=boards/stm32wba_stdby.overlay \\
     -DCONFIG_PM=y

.. stm32wba_low_power_end
