.. zephyr:board:: dspic33a_curiosity

Overview
********

The Curiosity Platform Development Board (EV74H48A) is a
full-featured development and demonstration platform enabling
customers to explore the capabilities of the dsPIC33A Digital
Signal Controller (DSC) family and also the PIC32A MCU family.
This board requires a DIM (Daughter Interface Module):
dsPIC33AK128MC106 or dsPIC33AK512MPS512

Hardware
********

  - Baseboard
     - dsPIC33A Curiosity Board
  - Supported DIM Modules
     - dsPIC33AK128MC106
     - dsPIC33AK512MPS512

dsPIC33AK128MC106
  - High-Performance dsPIC33A DSP CPU
  - 16-bit dsPIC33 core compatible
  - Non-paged linear Data/Flash 24-bit addressing space
  - 16-bit/32-bit instructions for optimized code size and performance
  - 32-bit Working Registers
  - Program flash: 128 KB
  - RAM: 16KB
  - 3 Instances of UARTs
  - One Dedicated 32-bit Timer/Counter
  - Four Single Output Capture/Compare/PWM/Timer (SCCP) Modules
  - The POSC and Internal FRC oscillators can use the on-chip PLL to achieve higher operating speeds.

dsPIC33AK512MPS512
  - High-Performance dsPIC33A DSP CPU
  - 16-bit dsPIC33 core compatible
  - Linear 24-bit program/data memory addressing space (no paging required)
  - 16-bit/32-bit instructions for optimized code size and performance
  - 32-bit Working Registers
  - Program flash: 512 KB
  - RAM: 64KB
  - 3 Instances of UARTs
  - Three Dedicated 32-Bit Timers/Counters
  - Single (SCCP) and Multiple (MCCP) Output Capture/Compare/PWM/Timer Modules
  - Two 1.6 GHz PLLs for Peripheral which can be clocked from the FRC or a Crystal Oscillator

For more information about the Baseboard please see `Curiosity Platform Development Board User's Guide`_
For more information about the dsPIC33AK128MC106 please see `dsPIC33AK128MC106 Family Data Sheet`_
For more information about the dsPIC33AK512MPS512 please see `dsPIC33AK512MPS512 Family Data Sheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================
The dsPIC33A Curiosity board requires one of the following: dsPIC33AK128MC106 or dsPIC33AK512MPS512

  - Insert the appropriate DIM board into the 120-pin socket, ensuring correct orientation.
  - Connect the Curiosity board to the host PC via USB.
  - Power and programming are handled through the on-board PKOB debugger.
  - Remove the jumper ``J28`` to enable the debugger.
  - ``J25`` is a 3-pin jumper used to choose the power source for the board.
    Place the jumper on pins 1-2 to use USB power (5V_USB).

System Clock
============
  - The dsPIC33AK128MC106/dsPIC33AK512MPS512 uses its internal 8 MHz FRC oscillator as the default clock source.
  - They also support multiple clock sources and an on-chip PLL, allowing the system and peripherals to operate at higher performance levels.
  - Peripheral clocks (Fast, Standard, Slow) are derived via fixed divisors.
  - See Processor clock control register in chapter ``oscillator module`` of the data sheet.

Serial Port
===========
  - The dsPIC33A Curiosity board provides a UART interface for console output and communication with a PC.
  - The serial interface can be accessed through the baseboard's DIM connector:
  - UART1 is configured for serial logs.
  - Pin 102 on base board USB to UART Converter TX
  - Pin 100 on base board USB to UART Converter RX

For more information about the DIM pins please see `dsPIC33AK128MC106 DIM schematics`_ `dsPIC33AK512MPS512 DIM schematics`_

ToolChain Setup
===============
  - Download and install the latest ``MPLABXIDE`` where you get the ``XC-DSC`` compiler from Microchip
  - Go to the official Microchip MPLAB X IDE please see `MPLABxIDE Installation`_
  - The XC-DSC toolchain is not part of the Zephyr SDK by default,
    so the path to the compiler must be added manually.

Setting the Toolchain Variant
=============================
  - Zephyr uses the variable :makevar:`ZEPHYR_TOOLCHAIN_VARIANT` to select which
    toolchain to use. For this board, the variant name is ``xcdsc``.

  - To specify it on the command line:

    .. code-block:: console

       west build -b dspic33a_curiosity/<qualifier> samples/hello_world -- -DZEPHYR_TOOLCHAIN_VARIANT=xcdsc

Setting the Toolchain Path
==========================
#. The XC-DSC compiler must be installed separately from Zephyr. The installation
   directory should be provided using the :makevar:`ZEPHYR_TOOLCHAIN_PATH` variable.

#. Typical installation paths are:

- **Windows**: ``C:\Program Files\Microchip\xc-dsc\<version>``
- **Linux**: ``/opt/microchip/xc16/<version>``

#. To specify it on the command line:

   .. code-block:: console

      west build -p always -b dspic33a_curiosity/<qualifier> samples/hello_world/ -- -DZEPHYR_TOOLCHAIN_VARIANT=xcdsc -DXCDSC_TOOLCHAIN_PATH=/opt/microchip/xc-dsc/<version>

Building
========

#. Build :zephyr:code-sample:`hello_world` application as you normally do.

#. After a successful build, the following files are produced:
   build/zephyr/zephyr.elf this will be used to flash

#. To build sample on the command line:

   .. code-block:: console

      west build -p always -b dspic33a_curiosity/<qualifier> samples/hello_world/ -- -DZEPHYR_TOOLCHAIN_VARIANT=xcdsc -DXCDSC_TOOLCHAIN_PATH=/opt/microchip/xc-dsc/<version>

Flashing
========
#. Run your favorite terminal program to listen for output.
   Under Linux the terminal should be :code:`/dev/ttyACM1`.

   For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM1 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Flash your board using ``west`` from the second terminal window.
   Split first and second terminal windows to view both of them.

   .. code-block:: console

      $ west flash

#. You should see ``"Hello World! <board name>"`` in the first terminal window.
   If you don't see this message, press the Reset button and the message should appear.

Debugging
=========
#. This board supports application flashing and debugging
   using the Microchip Debugger (MDB) command-line tool, which is part of the
   MPLAB X installation.

#. MDB can be used to program and debug Zephyr applications through a simple
   command's executed in a terminal.

   .. code-block:: console

      $ mdb
       - Device dsPIC33AK128MC106
       - Hwtool pkob4
       - Program build/zephyr/zephyr.elf
       - run
       - quit

#. The commands performs the following actions

  - Connects to the target device (dsPIC33AK128MC106).
  - Loads the hardware debugger (PKOB4).
  - Programs the compiled Zephyr ELF binary.
  - Starts program execution on the target.
  - Exits the debugger session.

For more information about MDB please see `Microchip Debugger (MDB) User's Guide`_

References
**********
.. target-notes::

..  _Curiosity Platform Development Board User's Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/UserGuides/Curiosity-Platform-Development-Board-Users-Guide-DS70005562.pdf
..  _dsPIC33AK128MC106 Family Data Sheet:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/DataSheets/dsPIC33AK128MC106-Family-Data-Sheet-DS70005539.pdf
..  _dsPIC33AK128MC106 DIM schematics:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/InformationSheet/dsPIC33AK128MC106-General-Purpose-DIM-Info-Sheet-DS70005556.pdf
..  _dsPIC33AK512MPS512 Family Data Sheet:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/DataSheets/dsPIC33AK512MPS512-Family-Data-Sheet-DS70005591.pdf
..  _dsPIC33AK512MPS512 DIM schematics:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/InformationSheet/dsPIC33AK512MPS512-General-Purpose-DIM-Info-Sheet-DS70005563.pdf
..  _MPLABxIDE Installation :
    https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide
..  _Microchip Debugger (MDB) User's Guide:
    https://www.microchip.com/content/dam/mchp/documents/DEV/ProductDocuments/UserGuides/50002102G.pdf
