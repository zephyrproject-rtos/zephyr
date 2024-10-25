.. zephyr:code-sample:: settings
   :name: Settings API
   :relevant-api: settings settings_rt settings_name_proc

   Load and save configuration values using the settings API.

Overview
********

This is a simple application demonstrating use of the settings runtime
configuration module. In this application some configuration values are loaded
from persistent storage and exported to persistent storage using different
settings method. The example shows how to implement module handlers, how to
register them.

Requirements
************

* A board with settings support, for instance: nrf52840dk/nrf52840
* Or qemu_x86 target

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/settings` in
the Zephyr tree.

The sample can be built for several platforms, the following commands build the
application for the qemu_x86.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/settings
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

After running the image to the board the output on the console shows the
settings manipulation messages.

Sample Output
=============

.. code-block:: console

   ***** Booting Zephyr OS build v2.1.0-rc1-123-g41091eb1d5e0 *****

   *** Settings usage example ***

   settings subsys initialization: OK.
   subtree <alpha> handler registered: OK
   subtree <alpha/beta> has static handler

   ##############
   # iteration 0
   ##############

   =================================================
   basic load and save using registered handlers

   load all key-value pairs using registered handlers
   loading all settings under <beta> handler is done
   loading all settings under <alpha> handler is done

   save <alpha/beta/voltage> key directly: OK.

   load <alpha/beta> key-value pairs using registered handlers
   <alpha/beta/voltage> = -3025
   loading all settings under <beta> handler is done

   save all key-value pairs using registered handlers
   export keys under <beta> handler
   export keys under <alpha> handler

   load all key-value pairs using registered handlers
   export keys under <beta> handler
   export keys under <alpha> handler

   =================================================
   loading subtree to destination provided by the caller

   direct load: <alpha/length/2>
   direct load: <alpha/length/1>
   direct load: <alpha/length>
     direct.length = 100
     direct.length_1 = 41
     direct.length_2 = 59

   =================================================
   Delete a key-value pair

   immediate load: OK.
     <alpha/length> value exist in the storage
   delete <alpha/length>: OK.
     Can't to load the <alpha/length> value as expected

   =================================================
   Service a key-value pair without dedicated handlers

   <gamma> = 0 (default)
   save <gamma> key directly: OK.
   ...

Provisioning of Settings
************************

Settings will often have to be provisioned separately from the application
itself, e.g. because the settings are specific to a particular device or because
the settings are sensitive and should not be included in the application binary.

The settings subsystem supports this use case by allowing settings to be
provisioned from a separate image.

To demonstrate this feature, the sample can be built with the
``CONFIG_FLASH_SIMULATOR_PROVISION`` option on QEMU. This will simulate a flash
partition with a fixed memory layout and load the settings from a separate image
at runtime into QEMU memory.

To build and run the sample on QEMU, follow these steps:

1. Build the sample for the qemu_x86 board with the default settings from
   qemu_x86.conf (i.e. with the NVS settings backend enabled.) and this
   overlay:

   .. code-block:: shell

      $ west build -b qemu_x86 samples/subsys/settings -- -DOVERLAY_CONFIG=overlay-qemu.conf

2. Place the sample soc-nv-flash-image.hex file in the build directory:

   .. code-block:: shell

      $ cp samples/subsys/settings/soc-nv-flash-image.hex build/zephyr

3. Start the QEMU emulator with the following command:

   .. code-block:: shell

      $ west build -t run

To achieve the same with a real device, a separate flash image can be prepared
that conforms to the selected memory backend and is flashed separately to the
appropriate flash memory location.

Run your application in debug mode in terminal 1:

   .. code-block:: shell

      $ west build -t debugserver

Start a remote debug session in terminal 2:

   .. code-block:: shell

      $ gdb build/zephyr/zephyr.elf
      (gdb) target remote localhost:1234
      (gdb) continue

Now interact with your application to prepare the desired settings state. You
could also write a dummy application that writes the settings to flash.

To dump the state of the settings backend in Intel HEX format:

   .. code-block:: shell

      (gdb) Ctrl-C
      (gdb) dump ihex memory soc-nv-flash-image.hex <storage_partition addr> <storage_partition size>
      (gdb) exit

To inspect the image:

   .. code-block:: shell

      (gdb) dump binary soc-nv-flash-image.bin <storage_partition addr> <storage_partition size>
      (gdb) exit
      $ hexdump -C soc-nv-flash-image.bin

To convert a binary file to Intel HEX format with correct address offsets, use
the ``objcpy`` tool:

   .. code-block:: shell

      objcopy -I binary -O ihex --change-addresses=<storage_partition addr> \
          soc-nv-flash-image.bin build/zephyr/soc-nv-flash-image.hex

And then flash the hex file to the device without erasing existing flash content.
