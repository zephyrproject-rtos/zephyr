.. zephyr:code-sample:: debug-ulp
   :name: Debug ULP

   Debug the LP Core in ESP32C6.

Overview
********

This example demonstrates how to build, flash and debug a simple application on the LP core.

This sample also provides a custom ``gdbinit`` file to be loaded by GDB to set up the debugging environment.
It connects with the remote OpenOCD server, loads the symbols from the LP Core application
and sets an initial breakpoint.

Limitations
***********
1. Currently debugging is not supported when either HP or LP core enters any sleep mode. So this limits debugging scenarios.
2. OS support is disabled when debugging the LP core, so you won't be able to see tasks running in the system. Instead there will be two threads representing HP ('esp32c6.cpu0') and LP ('esp32c6.cpu1') cores:

.. code-block:: console

   (gdb) info thread
     Id   Target Id                                                                Frame
     1    Thread 1 "esp32c6.hp.cpu0" (Name: esp32c6.hp.cpu0, state: debug-request) arch_irq_unlock (key=8) at zephyr/include/zephyr/arch/riscv/arch.h:259
   * 2    Thread 2 "esp32c6.lp.cpu" (Name: esp32c6.lp.cpu, state: breakpoint)      do_things (max=1000000000) at zephyr/samples/boards/espressif/ulp/lp_core/debug_ulp/remote/src/main.c:28

3. When setting HW breakpoint in GDB it is set on both cores, so the number of available HW breakpoints is limited to the number of them supported by LP core (2 for ESP32-C6).

Sysbuild
********

Sysbuild is in charge of building the application located in the ``remote`` folder for the LP Core target.
The build and flashing orders are as follows:

Build:

- MCUboot bootloader
- LP Core application
- HP Core application

Flash:

- MCUboot bootloader
- HP Core application
- LP Core application

Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/debug_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/debug_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: flash
   :compact:

Debugging
*********

Connect the USB cable to the USB port of the ESP32-C6 Devkitc board.

The ESP32-C6 modules require patches to OpenOCD that are not upstreamed yet.
Espressif maintains their own fork of the project.
The custom OpenOCD can be obtained at `OpenOCD ESP32`_.

In one terminal instance run the Espressif's OpenOCD:

.. code-block:: shell

   <path/to/openocd-esp32>/bin/openocd -f board/esp32c6-lpcore-builtin.cfg

Wait for the following output to show up in the console:

.. code-block:: console

   Info : Listening on port 3333 for gdb connections

On another terminal instance, run the GDB:

.. code-block:: shell

   <path/to/zephyr/sdk>/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb -x samples/boards/espressif/ulp/lp_core/debug_ulp/gdbinit build/debug_ulp/zephyr/zephyr.elf

The following output should be displayed:

.. code-block:: console

   [esp32c6.hp.cpu0] Reset cause (24) - (JTAG CPU reset)
   add symbol table from file "build/debug_ulp_lpcore/zephyr/zephyr.elf"
   Hardware assisted breakpoint 1 at 0x500000ec: file zephyr/samples/boards/espressif/ulp/lp_core/debug_ulp/remote/src/main.c, line 28.
   (gdb)

From now on you can use the GDB commands to debug the application. By using the command ``continue`` the LP Core application stop at the assigned breakpoint:

.. code-block:: console

   (gdb) continue
   Continuing.
   [Switching to Thread 2]

   Thread 2 "esp32c6.lp.cpu" hit Temporary breakpoint 1, do_things (max=1000000000) at zephyr/samples/boards/espressif/ulp/lp_core/debug_ulp/remote/src/main.c:28
   28                      for (int i = 0; i < max; i++) {
   (gdb)

When the application reaches the ``abort()`` function it will automatically break.

References
**********

.. target-notes::

.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
