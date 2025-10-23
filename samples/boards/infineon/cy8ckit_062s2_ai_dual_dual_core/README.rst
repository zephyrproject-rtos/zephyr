Dual Core Sample for CY8CKIT-062S2-AI-DUAL
==========================================

Overview
--------

This sample demonstrates dual-core operation on the CY8CKIT-062S2-AI-DUAL board
using Zephyr's sysbuild system. The application runs on both Cortex-M0+ and
Cortex-M4 cores simultaneously.

Features
--------

- **CM0+ Core (Primary)**:
  - Uses GPIO interrupts for button press detection
  - Toggles LED0 when button is pressed (interrupt-driven)
  - Starts the CM4 core after initialization
  - Provides system control

- **CM4 Core (Secondary)**:
  - Blinks LED1 continuously
  - Runs independently from CM0+

Hardware Requirements
--------------------

- CY8CKIT-062S2-AI-DUAL board
- LED0 and LED1 (defined in device tree)
- Button (SW0) connected to interrupt-capable GPIO

Technical Implementation
------------------------

- **Interrupt-based Button Handling**: The CM0+ core uses GPIO interrupts
  to detect button presses, providing immediate response without polling
- **Dual-core Coordination**: CM0+ starts CM4 core using `Cy_SysEnableCM4()`
  function from Infineon PDL
- **Independent Operation**: Both cores run their own Zephyr instances with
  separate memory spaces and device trees

Building and Running
--------------------

Build the sample using sysbuild:

.. code-block:: bash

   west build -b cy8ckit_062s2_ai_dual/cy8c624abzi_s2d44/m0 samples/boards/infineon/cy8ckit_062s2_ai_dual_dual_core --sysbuild

Flash the sample:

.. code-block:: bash

   west flash

Expected Behavior
-----------------

1. **CM0+ starts first**:
   - LED0 blinks 5 times to indicate CM0+ is running
   - Button press toggles LED0 state (interrupt-driven)
   - After 3 seconds, CM4 core is started

2. **CM4 starts after CM0+**:
   - LED1 begins blinking continuously (1 second intervals)
   - Both cores run independently

3. **Dual-core operation**:
   - CM0+ handles button/LED0 interactions
   - CM4 handles LED1 blinking
   - Both cores can run simultaneously

Console Output
-------------

The sample will output messages to the console showing:
- Core identification (CM0+ or CM4)
- LED state changes
- Button press events
- Core startup sequence

Troubleshooting
--------------

- Ensure the board is properly connected
- Check that LED0, LED1, and button are defined in the device tree
- Verify that both cores are flashed correctly
- Monitor console output for error messages
