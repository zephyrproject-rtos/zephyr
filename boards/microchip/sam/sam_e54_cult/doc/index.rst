.. zephyr:board:: sam_e54_cult

Overview
********

The SAM E54 Curiosity Ultra evaluation kit is ideal for evaluation and
prototyping with the SAM E54 Cortex®-M4F processor-based
microcontrollers. The kit includes Microchip's Embedded Debugger (EDBG),
which provides a full debug interface without the need for additional
hardware.

Hardware
********

- ATSAME54P20A, 120 MHz, 1MB Flash, 256 KB RAM
- On-Board Debugger (EDBG)
- Virtual COM port (VCOM)
- Data Gateway Interface (DGI)
- One mikroBUS interfaces
- One X32 audio interfaces supporting Bluetooth and audio
- Ethernet interface
- Graphics interface
- Xplained Pro extension compatible interface
- CAN interface
- User buttons
- User LEDs
- 8-MB QSPI memory
- Arduino Uno R3 compatible interface

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAM E54 Curiosity Ultra User Guide`_ has detailed information about board connections.

Programming and Debugging
*************************

The SAM E54 Curiosity Ultra features an on-board Microchip Embedded Debugger (EDBG),
which provides both a standard debug interface and a virtual serial port used as the Zephyr console.
SERCOM1 of the ATSAME54P20A MCU is configured as a USART for console output.

#. Connect the board to your host machine using the debug USB port.

#. Open a terminal and start a serial console on the corresponding port.
   On Linux, this is typically ``/dev/ttyACM0``. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Build and flash the Zephyr ``hello_world`` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: sam_e54_cult
      :goals: flash
      :compact:

#. Observe output on the terminal. If everything is set up correctly, you should see:

   .. code-block:: console

      Hello World! sam_e54_cult

References
**********

SAM E54 Product Page:
    https://www.microchip.com/en-us/product/ATSAME54P20A

SAM E54 Curiosity Ultra evaluation kit Page:
    https://www.microchip.com/en-us/development-tool/EV66Z56A

.. _SAM E54 Curiosity Ultra User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/UserGuides/SAM-E54-Curiosity-Ultra-User%E2%80%99s-Guide-DS70005405.pdf
