.. zephyr:board:: crd40l50

Overview
********

The CRD40L50-POC-Q "Blackstar" demonstration board is a hardware platform
used to evaluate Cirrus Logic CS40L50 haptic drivers. The board provides
a set of features that enables programming the on-board STM32F401xD
microcontroller with custom sample applications, including the CS40L5x
sample provided in Zephyr's samples directory.

Hardware
********

- STM32F401xD microcontroller
- AT25 SPI flash controller with 256 KiB flash memory
- 32.768 kHz crystal oscillator
- Two pass/fail LEDs, one green and one red
- Three yellow user LEDs
- One user push button
- Power source selector switch
- Actuator output for LRAs
- One Pico-Clasp connector for GPIOs
- One USB-C port for Cirrus Logic ETHapBridge connectivity
- Programming and debugging of on-board STM32F401xD through Serial Wire Debug (SWD)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- Molex 1053131302 connector (J4) for haptic output
- JST S2B-PH-SM4-TB(LF)(SN) connector (J5) for external power sources
- SWD header (J7) for J-Link debug probes
- Molex 203559-0607 connector (J8) for GPIOs
- USB-C port (J10) for ETHapBridge connectivity

Programming & Debugging
***********************

.. zephyr:board-supported-runners::

Flash Using J-Link
==================

To flash the board using the J-Link debugger, follow the steps below:

1. Install J-Link Software

   - Download and install the `J-Link software`_ tools from Segger.
   - Make sure the installed J-Link executables (e.g., ``JLink``, ``JLinkRTTViewer``)
     are available in your system's PATH.

2. Connect the Board

   - Connect the `J-Link Debug Probe`_ to the board's SWD header (J7).
   - Connect the other end of the J-Link Debug Probe to your **host machine** via USB.
   - Connect the VBAT connector (J5) to an external power source to **power up the board**.

3. Build the Application

   You can build a sample Zephyr application, such as **Blinky** or **cs40l5x**, using
   the ``west`` tool. Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b crd40l50 -p always samples/drivers/haptics/cs40l5x

   This will build the CS40L5x sample application for the ``crd40l50`` board.

4. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash --runner jlink

   This uses the ``jlink`` runner to flash the application to the board.

5. Observe the Result

   After flashing, interact with the Shell interface via ``JLinkRTTViewer`` to demo
   the on-board CS40L50 haptics driver.

References
**********

.. _Cirrus Logic:
    https://www.cirrus.com/support

.. _J-Link software:
    https://www.segger.com/downloads/jlink

.. _J-Link Debug Probe:
    https://www.segger.com/products/debug-probes/j-link/
