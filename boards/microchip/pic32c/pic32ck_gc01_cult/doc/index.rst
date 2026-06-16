.. zephyr:board:: pic32ck_gc01_cult

Overview
********

The PIC32CK GC01 Curiosity Ultra evaluation kit is a hardware platform
to evaluate the Microchip PIC32CK GC microcontrollers, and the
evaluation kit part number is EV44P93A. This kit provides a comprehensive
set of features that allow users to explore the PIC32CK GC peripherals
and gain insight into integrating the device into their own designs.

Hardware
********

- 144-pin TQFP PIC32CK2051GC01144 microcontroller
- Arm® Cortex®-M33 Microcontroller
- 32.768 kHz crystal oscillator
- 12 MHz crystal oscillator
- 2048 KiB flash memory and 512 KiB of RAM
- Two user LEDs (Red and Green)
- One green board power LED
- Two mechanical user push buttons
- One reset button
- On-Board Temperature Sensor
- Two USB interfaces (Type-C and Micro-B)
- Virtual COM port (VCOM)
- Programming and debugging of on-board PIC32CK GC through Serial Wire Debug (SWD) and on-board PKoB4 debugger
- Arduino Uno connector
- MikroBUS™ socket
- Xplained Pro extension header
- Graphics interface
- 10/100 MB Ethernet interface
- Two CAN FD interface
- SD card slot
- QSPI Flash and Serial EEPROM
- One Motor control Position Decoder (PDEC) interface

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CK GC01 Curiosity Ultra User Guide`_ has detailed information about board connections.

Programming & Debugging
***********************

.. zephyr:board-supported-runners::

Flash Using J-Link
==================

To flash the board using the J-Link debugger, follow the steps below:

1. Install J-Link Software

   - Download and install the `J-Link software`_ tools from Segger.
   - Make sure the installed J-Link executables (e.g., ``JLink``, ``JLinkGDBServer``)
     are available in your system's PATH.

2. Connect the Board

   - Connect the `J32 Debug Probe`_ to the board's **CORTEX DEBUG** header.
   - Connect the other end of the J32 Debug Probe to your **host machine (PC)** via USB.
   - Connect the DEBUG USB port on the board to your host machine to **power up the board**.

3. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32ck_gc01_cult -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32ck_gc01_cult`` board.

4. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

   This uses the default ``jlink`` runner to flash the application to the board.

5. Observe the Result

   After flashing, **LED0** on the board should start **blinking**, indicating that the
   application is running successfully.


Debug Using J-Link: GDB Command-Line
====================================

To debug the board using the J-Link debugger, follow the steps below:

1. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32ck_gc01_cult -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32ck_gc01_cult`` board.

2. Debug the Application

   To debug the application, run the following command from your Zephyr workspace:

   .. code-block:: console

      west debug

   .. note::
      ``Debugging with J-Link GDB Server: Halting at `main` and Setting Breakpoints``

      .. code-block:: sh

         (gdb) break main
         (gdb) break main.c:42         # Optional: set breakpoint at line 42 in main.c
         (gdb) continue                # Start execution, halts at first breakpoint
         (gdb) info breakpoints        # List all breakpoints
         (gdb) delete 1                # Delete breakpoint number 1
         (gdb) delete                  # Delete all breakpoints
         (gdb) clear main.c:42         # Clear breakpoint at specific location
         (gdb) continue                # Resume execution

Debug Using J-Link: VSCode
==========================

To debug using VSCode, create or edit the **.vscode/launch.json** file in your project directory with the following configuration:

.. code-block:: json

   {
      "version": "0.2.0",
      "configurations": [
         {
               "name": "Debug (J-Link, PIC32CK2051GC01144)",
               "type": "cortex-debug",
               "request": "launch",
               "servertype": "jlink",
               "device": "PIC32CK2051GC01144",
               "interface": "swd",
               "runToEntryPoint": "main",
               "showDevDebugOutput": "none",
               "cwd": "${workspaceFolder}",
               "executable": "${workspaceFolder}/zephyr/build/zephyr/zephyr.elf",

               //J-Link settings
               "serverpath": "C:/Program Files/SEGGER/JLink_Vxxx/JLinkGDBServerCL.exe",
               "serverArgs": [
                  "-singlerun",
                  "-if", "swd",
                  "-speed", "4000",
                  "-device", "PIC32CK2051GC",
                  "-port", "50000"
               ],

               //Zephyr SDK toolchain paths
               "gdbPath": "***/zephyr-sdk-x.xx.x/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb.exe",
               "objdumpPath": "***/zephyr-sdk-x.xx.x/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe",
         }
      ]
   }

.. note::

   - **serverpath**: Specify the full path to the J-Link GDB Server executable if it is not in your system's PATH.

   - **gdbPath**: Specify the full path to the GDB executable if it is not in your system's PATH.

   - **objdumpPath**: Specify the full path to the objdump executable if it is not in your system's PATH.

Debugging Steps in VSCode
-------------------------

1. Start the J-Link GDB Server as shown above.
2. Open the samples/basic/blinky/src/main.c in VSCode.
3. Set breakpoints in your source files by clicking next to the line numbers.
4. Press **F5** or click the green "Start Debugging" button in VSCode.
5. The debugger will connect, load your program, and halt at **main**.
6. Use the VSCode debug controls to continue, step, or inspect variables.

References
**********

PIC32CK GC Product Page:
   https://www.microchip.com/en-us/product/pic32ck2051gc01144

PIC32CK SG/GC Product Family Page:
   https://www.microchip.com/en-us/products/microcontrollers/32-bit-mcus/pic32-sam/pic32ck-sg-gc

.. _PIC32CK GC01 Curiosity Ultra evaluation kit Page:
   https://www.microchip.com/en-us/development-tool/EV44P93A

.. _PIC32CK GC01 Curiosity Ultra User Guide:
   https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/PIC32CK-SG01-GC01-Curiosity-Ultra-User-Guide-DS70005529.pdf

.. _J-Link software:
   https://www.segger.com/downloads/jlink

.. _J32 Debug Probe:
    https://www.microchip.com/en-us/development-tool/dv164232
