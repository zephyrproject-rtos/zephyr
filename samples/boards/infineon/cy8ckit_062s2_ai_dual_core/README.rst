Dual Core Sample for CY8CKIT-062S2-AI
==========================================

Overview
--------

This sample demonstrates dual-core operation on the CY8CKIT-062S2-AI board
using Zephyr's sysbuild system. The application runs on both Cortex-M0+ and
Cortex-M4 cores simultaneously.

Features
--------

- **CM0+ Core (Primary)**:
  - Blinks LED0 five times to indicate CM0+ is running
  - Starts the CM4 core after a 3-second delay
  - Provides system control and initialization

- **CM4 Core (Secondary)**:
  - Blinks LED1 continuously (1 second intervals)
  - Runs independently from CM0+

Hardware Requirements
--------------------

- CY8CKIT-062S2-AI board
- LED0 and LED1 (defined in device tree)

Technical Implementation
------------------------

- **Dual-core Coordination**: CM0+ starts CM4 core using ``Cy_SysEnableCM4()``
  function from Infineon PDL with the M4 flash base address (0x10080000)
- **Independent Operation**: Both cores run their own Zephyr instances with
  separate memory spaces and device trees
- **Memory Partitioning**: M0 uses first 512KB of flash and 128KB of SRAM,
  M4 uses remaining 1.5MB of flash and 892KB of SRAM, with 4KB shared SRAM
  for inter-core communication

Building and Running
--------------------

Build the sample using sysbuild:

.. code-block:: bash

   west build -b cy8ckit_062s2_ai/cy8c624abzi_s2d44/m0 samples/boards/infineon/cy8ckit_062s2_ai_dual_core --sysbuild

Flash the sample:

.. code-block:: bash

   west flash

Expected Behavior
-----------------

1. **CM0+ starts first**:
   - LED0 blinks 5 times (500ms intervals) to indicate CM0+ is running
   - Then CM4 core is started
   - CM0+ then enters sleep mode

2. **CM4 starts after CM0+**:
   - LED1 begins blinking continuously (1 second intervals)
   - Both cores run independently

3. **Dual-core operation**:
   - CM0+ remains in sleep mode after starting CM4
   - CM4 handles LED1 blinking continuously
   - Both cores can run simultaneously

Console Output
-------------

The sample will output messages to the console showing:
- Core identification (CM0+ or CM4)
- CM0+ initialization and CM4 startup sequence
- CM4 LED1 state changes (ON/OFF) during continuous blinking

Troubleshooting
--------------

- Ensure the board is properly connected
- Check that LED0 and LED1 are defined in the device tree
- Verify that both cores are flashed correctly using sysbuild
- Monitor console output for error messages
- Ensure the M4 flash base address (0x10080000) matches your board configuration
