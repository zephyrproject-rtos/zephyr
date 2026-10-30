.. zephyr:board:: pic32wm_bz6204_curiosity

Overview
*********

The **PIC32-BZ6 Curiosity Board (EV31U42A)** is a versatile development platform designed to facilitate rapid prototyping and evaluation of the **PIC32WM-BZ6204UE** module.
This module supports **Bluetooth® Low Energy (BLE)** and **IEEE® 802.15.4** wireless communications.

The board is targeted at **IoT**, **Home Automation**, and **Industrial Automation** applications such as smart door locks, alarm sensors, wall switches, and thermostats.

.. note::

   This is the initial Zephyr port for the board. Only the GPIO driver is
   currently supported, which is sufficient to run the :zephyr:code-sample:`blinky`
   sample. Support for the remaining on-chip peripherals (UART, DMA, RTC, PWM,
   BLE, IEEE 802.15.4, and others) is planned for follow-up contributions.

**Key Features:**

- On-board PIC32WM-BZ6204UE wireless module (BLE and 802.15.4)
- Integrated PKOB4 programmer/debugger
- Power options: USB Type-C, External 5V, or Li-ion/Li-Po battery
- USB-UART converter for serial communication
- Two mikroBUS™ sockets for expansion
- XPRO and RMII headers for Ethernet and touch interface
- User interface: RGB LED, user LED, two push buttons, and a reset button
- Temperature sensor and external QSPI Flash memory
- External antenna connector (2.4 GHz, 2 dBi)



Hardware
********

Power Supply
------------

The board supports multiple power sources:

1. **Debug USB (J100)** – via Type-C cable from PC
2. **Target USB (J103)** – via Type-C cable
3. **External 5 V (J201)** – through 1×2 header
4. **Li-ion/Li-Po battery (J204)** – 4.2 V via JST PH 2-pin connector (recommended 400 mAh minimum)

**Power Selection Jumper (J202):**

- Pins 1–2 → External 5 V (J201)
- Pins 3–4 → Debug USB (J100)
- Pins 5–6 → Target USB (J103)

**Voltage Regulation:**

- 3.3 V buck regulator (U203, MIC33153) for PIC32WM-BZ6204UE
- LDO (U204, MCP1727) optional for 3.3 V
- LDO (U201) powers USB hub, PKOB4, and UART converter

**External Supply Header (J203):**

Allows direct 1.9–3.6 V input to the PIC32WM-BZ6204UE (requires changing resistor jumpers R213/R217).

USB and Connectivity
--------------------

- Integrated **USB 2.0 hub controller (USB2512B)**
- **USB Device/Host support** via Target USB Type-C connector (J103)
- Host mode requires jumper configuration:
  - J106 → CC pin
  - J102 → Target USB VBUS select
  - J104 → VBUSON control (hard/soft enable)

Expansion and Interfaces
------------------------

- **Two mikroBUS™ sockets (J903, J905)** for Click™ add-on boards
  Provide SPI, UART, I²C, PWM, Analog, Reset, Interrupt, and Power pins.
- **PTA headers (J901, J902)** for co-existence control (e.g., Wi-Fi/Bluetooth)
- **XPRO header (J900)** compatible with Microchip QT7/T9 Xplained Pro kits
- **GPIO headers (J701, J702)** to access module I/O pins
- **Graphics connector (J910)** for external GUI/Display adapter
- **RMII headers (J906–J909)** for external Ethernet PHY

On-board Peripherals
--------------------

- **Temperature sensor (U800, MCP9700A)** → PD4/AN10
- **QSPI Flash (U801, SST26VF064B)** → 64 Mbit
- **User LED (D801, Blue)** → PB7 (active low)
- **RGB LED (D800)**:
  - Red: RPC7
  - Green: RPC10
  - Blue: RPE0
- **Two user buttons:**
  - SW801 (BTN1) → RPB9
  - SW800 (BTN2) → RPE3
- **Reset switch (SW802)** → NMCLR


**Design Files:**
`PIC32-BZ6 Curiosity Board Hardware Design Documentation <https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/BoardDesignFiles/PIC32-BZ6-EA-Kit-Hardware-Design-Documentation.zip>`_


Supported Features
==================

.. zephyr:board-supported-hw::

For complete list of EV96B94A see `PIC32-BZ6 Curiosity Board User’s Guide <https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/UserGuides/PIC32-BZ6-Curiosity-Board-User-Guide-DS00006006.pdf>`_


Connections and IOs
===================

GPIO Headers
------------

**J701 (GPIO Header 1):**

::

   1  - MIKRO1_RST / RPA15
   2  - AN0 / MIKRO1_AN / RPB10
   3  - GND
   4  - MIKRO2_AN / BAT_MON / RPB5
   5  - GREEN_LED / MIKRO2_RST / RPC10
   6  - GND
   7  - GMAC_GTX0 / RPC0
   8  - GMAC_GTX1 / RPE1
   9  - GND
   10 - GMAC_GREFCLKOUT / RPC1
   11 - COEX_RF_ACTIVE / QSPI_DATA1 / RPB12
   12 - QSPI_DATA3 / CAN1_RX / RPA2
   13 - QSPI_DATA2 / COEX_WLAN_ACT / RPB11
   14 - BAT_MON
   15 - QSPI_CS / RPB13
   16 - GND
   17 - GND
   18 - MIKRO2_INT / BTN2 / RPE3
   19 - RED_LED / RMII_INT / RPC7
   20 - RMII_EN / MIKRO2_PWM / RPC11
   21 - GND
   22 - GND
   23 - UART_CTS / RPB4
   24 - RPB6
   25 - UART_RTS / RPA3
   26 - CAN1_TX / MIKRO1_UART_TX / RPB2
   27 - MIKRO1_PWM / RPD1
   28 - MIKRO1_INT / RPE4
   29 - BLUE_LED / RPE0
   30 - RMII_RST / RPD0
   31 - GND
   32 - GND

**J702 (GPIO Header 2):**

::

   1  - GMAC_GCRS_DV / RPE2
   2  - GMAC_GTXEN / RPC9
   3  - GMAC_GMDIO / RPD6
   4  - GMAC_GRXER / RPC8
   5  - GMAC_GRX1 / RPA13
   6  - GMAC_GMDC / RPD7
   7  - UART_TX / RPA5
   8  - GMAC_GRX0 / RPA14
   9  - GND
   10 - UART_RX / RPA6


USB-UART Serial Converter (U101)
--------------------------------

- **Device:** MCP2200 USB-UART converter
- **Connections:**
  - TX → PA6 (UART RX)
  - RX → PA5 (UART TX)
  - RTS → PB4 (UART CTS) — requires R602 populated
  - CTS → PA3 (UART RTS) — requires R601 populated

mikroBUS™ Sockets (J903, J905)
------------------------------

- Two mikroBUS™ sockets for Click™ boards or expansion modules
- Provide SPI, UART, I²C, PWM, Analog, Reset, INT, 3.3 V/5 V, and GND
- Shared/multiplexed pins require attention to resource conflicts

LEDs and Buttons
----------------

- **User LED (D801):** Blue, active LOW, connected to PB7
- **RGB LED (D800):**
  - Red → RPC7
  - Green → RPC10
  - Blue → RPE0
- **Buttons:**
  - SW801 → RPB9 (BTN1)
  - SW800 → RPE3 (BTN2)
- **Reset Button:** SW802 → NMCLR


Temperature Sensor
------------------

- **U800 (MCP9700A)** connected to PD4 (AN10)
- Enabled via jumper **J801**
- Shared with XPRO CVD input

QSPI Flash
----------

- **U801 (SST26VF064B, 64 Mbit)**
- To enable QSPI:
  - Populate resistors R739, R742, R744, R746, R749, R752
- Shares pins with other peripherals


For detailed schematics, pin assignments, and signal descriptions, go to the reference document section below

Programming and Debugging
*************************

This section describes how to flash and debug applications on the Microchip Wireless PIC32WM_BZ6 Curiosity Board Curiosity board using Zephyr.

**Supported Debuggers**

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 20 20

   * -
     - Flash
     - Debug
     - Debug Server
     - Debug Tool
   * - Segger
     - ✓
     - ✓
     - ✓
     - J-Link
   * - OpenOCD
     - ✓
     - ✓
     - ✓
     - PKOB4, PICkit Basic

Flashing
--------

Follow the steps below to build and flash your application:

1. Open a terminal and change to the Zephyr workspace directory:

   .. code-block:: console

      cd zephyr

2. Build the application using the following command:

   .. code-block:: console

      west build -p always -b pic32wm_bz6204_curiosity .\samples\basic\blinky\

3. After a successful build, connect the PIC32WM_BZ6 device to your machine.

4. Flash the device using the ``west flash`` command:

   .. code-block:: console

      west flash --hex-file build/zephyr/zephyr_signed.hex

5. Ensure the flash process completes successfully. You should see confirmation messages in the terminal.

Debugging
---------

To debug the PIC32WM_BZ6 application using Visual Studio Code:

1. Ensure the application is built for the PIC32WM_BZ6 board.
2. Install the ``cortex-debug`` extension in Visual Studio Code.
3. Open the workspace and click the **Run and Debug** icon on the left sidebar.
4. If ``launch.json`` and ``tasks.json`` files are already present, VS Code will automatically start the debug session.
5. If prompted to create a new ``launch.json``, select the **Cortex Debug** debugger option.
6. Replace the contents of ``launch.json`` with:

   .. code-block:: json

      {
        "version": "2.0.0",
        "configurations": [
          {
            "name": "Debug PIC32WM_BZ6",
            "type": "cortex-debug",
            "request": "attach",
            "servertype": "openocd",
            "cwd": "C:\\developers\\zephyr\\",
            "executable": "<path to zephyr project>/build/zephyr/zephyr.elf",
            "device": "PIC32WM",
            "configFiles": [
              "interface/cmsis-dap.cfg",
              "target/pic32wm.cfg"
            ],
            "gdbPath": "<path to zephyr sdk>/arm-zephyr-eabi/bin/arm-zephyr-eabigdb.exe",
            "preLaunchTask": "flash_pic32wm_hex",
            "postRestartCommands": [
              "symbol-file <path to zephyr project>/build/zephyr/zephyr.elf",
              "monitor reset halt",
              "break main"
            ],
            "showDevDebugOutput": "none"
          }
        ]
      }

7. Create ``tasks.json`` inside ``.vscode`` with:

   .. code-block:: json

      {
        "version": "2.0.0",
        "tasks": [
          {
            "label": "flash_pic32wm_hex",
            "type": "shell",
            "command": "openocd",
            "args": [
              "-f", "interface/cmsis-dap.cfg",
              "-f", "target/pic32wm.cfg",
              "-c", "init",
              "-c", "reset halt",
              "-c", "program <path to zephyr project>/build/zephyr/zephyr_signed.hex reset exit"
            ],
            "problemMatcher": [],
            "group": {
              "kind": "build",
              "isDefault": true
            }
          }
        ]
      }

8. Connect the PIC32WM_BZ6 Curiosity board.
9. Click the **Run and Debug** icon again and select the Debug PIC32WM_BZ6 debug option.
10. Confirm that the debugger hits the breakpoint in ``main.c``. Press **Continue** to proceed.

References
----------

- `PIC32-BZ6 Curiosity Board Product Page <https://www.microchip.com/en-us/development-tool/ea81w68a>`_
- `PIC32-BZ6 Curiosity Board User Guide <https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/UserGuides/PIC32-BZ6-Curiosity-Board-User-Guide-DS00006006.pdf>`_
- `Hardware Design Files <https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/BoardDesignFiles/PIC32-BZ6-EA-Kit-Hardware-Design-Documentation.zip>`_
- `PIC32CX-BZ6 and PIC32WM-BZ6 Family Data Sheet <https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/DataSheets/PIC32CX-BZ6-and-PIC32WM-BZ6-Family-Data-Sheet-00005998.pdf>`_
- `mikroBUS Click Boards <https://www.mikroe.com/click>`_
- `Microchip Support Portal <http://support.microchip.com/>`_
- `Microchip Direct <https://www.microchipdirect.com/?srsltid=AfmBOop0KWt1byQZUafcD8wwzrgQX_iuCJLi6AmzTIzhI6Ez-D2IZr_M>`_
- `Zephyr® for Microchip <https://www.microchip.com/en-us/tools-resources/develop/zephyr>`_
