.. zephyr:code-sample:: rpi_pico_shell_bootloader
   :name: Shell Bootloader (RP2040/RP2350)
   :relevant-api: shell_api

   Reboot RP2040/RP2350 into BOOTSEL mode via shell command or USB trigger.

Overview
********

This sample provides a Zephyr shell with two methods to reboot an
RP2040/RP2350 board into BOOTSEL (USB mass storage) mode without
pressing the physical BOOTSEL button:

- **Shell command**: ``reboot bootloader`` uses the Zephyr retention boot
  mode API (``bootmode_set(BOOT_MODE_TYPE_BOOTLOADER)`` + ``sys_reboot()``).
  On next boot, the SoC hook in ``rom_bootloader.c`` detects the flag and
  enters BOOTSEL.
- **USB 1200-baud trigger**: A background thread monitors the CDC ACM baud
  rate. When the host sets 1200 baud via a USB ``SET_LINE_CODING`` control
  transfer, the firmware enters BOOTSEL mode. A ``bootsel.py`` host script
  is provided.

Requirements
************

- A board with an RP2040 or RP2350 SoC (e.g., Raspberry Pi Pico, Pico 2).
- USB connection for CDC ACM console (use the ``cdc-acm-console`` snippet).
- The ``rp2-boot-mode-retention`` snippet for retained RAM boot mode flag.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/raspberrypi/rpi_pico/shell_bootloader
   :board: rpi_pico
   :goals: build flash
   :gen-args: -S cdc-acm-console -S rp2-boot-mode-retention

After flashing, connect to the USB serial console:

.. code-block:: console

   minicom -D /dev/ttyACM0 -b 115200

Shell Command
=============

.. code-block:: console

   uart:~$ reboot bootloader
   Rebooting into BOOTSEL...

The board reboots into BOOTSEL mode and mounts as USB mass storage.

USB Trigger (bootsel.py)
========================

The ``bootsel.py`` script triggers BOOTSEL from the host without opening
a serial console. It requires ``pyusb`` (``pip install pyusb``).

.. code-block:: console

   # Reboot all connected RP2 boards into BOOTSEL
   python3 samples/boards/raspberrypi/rpi_pico/shell_bootloader/bootsel.py

   # Reboot a specific board by USB bus and address
   python3 bootsel.py --bus 1 --addr 10
