.. _hl78xx_auto_baudrate:

HL78xx Auto Baud Rate Switching
===============================

Overview
--------

This feature enables automatic baud rate detection and switching for the
Sierra Wireless HL78xx modem driver. The driver can automatically detect
the modem's current baud rate and switch to a configured target baud rate
using the ``AT+IPR`` command.

Features
--------

- **Auto-detection**: Automatically detects the modem's current baud rate
  by trying a list of common baud rates
- **Dynamic switching**: Changes the modem's baud rate to the configured
  target using the ``AT+IPR`` command
- **Configurable**: Supports multiple baud rates from 9600 to 921600 bps
- **Retry mechanism**: Configurable retry count for robust detection
- **State machine integration**: Seamlessly integrated into the modem
  initialization sequence

Configuration
-------------

Enable the feature in your project's ``prj.conf``:

.. code-block:: ini

   # Enable auto baud rate detection and switching
   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y

   # Set target baud rate (default is 115200)
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y

   # Configure detection baud rates (comma-separated list)
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,9600,57600,38400,19200"

   # Set timeout for each detection attempt (in seconds)
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=4

   # Set number of retries
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3

   # Save baud rate change to modem NVMEM (default: y, recommended)
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y

   # Try target baud rate first before detection list (default: y, faster)
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y

   # Only perform auto-baud if initial communication fails (default: y, efficient)
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

   # Perform auto-baud immediately at boot, skip KSUP wait (default: n)
   CONFIG_MODEM_HL78XX_AUTOBAUD_AT_BOOT=n

Supported Baud Rates
--------------------

The following baud rates are supported:

- 9600 bps
- 19200 bps
- 38400 bps
- 57600 bps
- 115200 bps (default)
- 230400 bps
- 460800 bps
- 921600 bps
- 3000000 bps (HL7812 only)

How It Works
------------

Detection Phase (``MODEM_HL78XX_STATE_SET_BAUDRATE``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the modem enters the ``SET_BAUDRATE`` state after power-on:

1. The driver tries each baud rate from the detection list
2. For each rate, it:

   - Configures the UART to that baud rate
   - Sends an ``AT`` command
   - Waits for an ``OK`` response

3. Once a response is received, the current baud rate is identified

Switching Phase
~~~~~~~~~~~~~~~

If the detected baud rate differs from the target:

1. Send ``AT+IPR=<target_baudrate>`` at the current baud rate
2. Wait for ``OK`` response
3. Wait 2.5 seconds for the modem to apply the change
4. Reconfigure the host UART to the new baud rate
5. Wait 50 ms for UART stabilization
6. Verify communication by sending an ``AT`` test command
7. Send ``AT&W`` to save configuration to NVMEM

.. note::

   After baud rate switching completes, the state machine runs the
   post-restart script to wait for the KUSP URC message before proceeding
   to initialization.

State Transitions
~~~~~~~~~~~~~~~~~

::

   AWAIT_POWER_ON --> SET_BAUDRATE --> RUN_INIT_SCRIPT
                           |
                           +--> [On Failure] --> RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT

Use Cases
---------

Use Case 1: Unknown Modem Baud Rate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,9600,57600,38400,19200"

Use Case 2: High-Speed Communication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y

Use Case 3: Fast Boot with Known Baud Rate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=2

**Result:** Boot time ~50–100 ms if modem is already at target rate.

Use Case 4: Production / Field Deployment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,38400,19200,9600"
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=8
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=5
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

Use Case 5: Temporary Baud Rate Change
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=n

.. note::

   The modem will revert to the previous baud rate after a power cycle
   or ``AT+CFUN`` reset.

AT Commands Used
----------------

Query Current Baud Rate
~~~~~~~~~~~~~~~~~~~~~~~

::

   AT+IPR?

Response::

   +IPR: <rate>
   +IPR: 0

Set Fixed Baud Rate
~~~~~~~~~~~~~~~~~~~

::

   AT+IPR=<rate>

Where ``<rate>`` can be:

- 9600
- 19200
- 38400
- 57600
- 115200
- 230400
- 460800
- 921600
- 3000000 (HL7812 only)

.. important::

   The new baud rate takes effect after ~2 seconds. Use ``AT&W`` to
   persist the configuration across power cycles.

Troubleshooting
---------------

Detection Fails
~~~~~~~~~~~~~~~

**Symptoms:** Modem enters diagnostic state after multiple retries

**Solutions:**

1. Verify detection list includes the modem's current baud rate
2. Increase timeout
3. Check UART wiring and signal quality
4. Verify modem is powered and responsive

Switching Fails
~~~~~~~~~~~~~~~

**Symptoms:** Detection succeeds but switching fails

**Solutions:**

1. Verify target baud rate is supported by modem and UART
2. Check UART clock limitations
3. Try a lower baud rate
4. Verify firmware supports ``AT+IPR``

Communication Lost After Switch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Root Cause:** Host UART not reconfigured before sending ``AT&W``

**Solutions:**

1. Reconfigure host UART **before** ``AT&W``
2. Wait 2.5 seconds after ``AT+IPR``
3. Wait for KUSP URC
4. Check UART buffering and flow control

Disabling Auto-Baud Causes Failure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Root Cause:** Modem remains at saved baud rate while UART defaults differ.

**Solutions:**

- Keep auto-baud enabled (recommended)
- Update device tree UART speed:

.. code-block:: dts

   &uart_modem {
       current-speed = <921600>;
   };

- Manually reset modem baud rate before disabling auto-baud

Implementation Details
----------------------

Key Functions
~~~~~~~~~~~~~

.. code-block:: c

   static int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate);
   static int hl78xx_detect_current_baudrate(struct hl78xx_data *data);
   static int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate);

State Handler
~~~~~~~~~~~~~

.. code-block:: c

   static int hl78xx_on_set_baudrate_state_enter(struct hl78xx_data *data);
   static void hl78xx_set_baudrate_event_handler(struct hl78xx_data *data,
                                                 enum hl78xx_event evt);

Data Structure
~~~~~~~~~~~~~~

.. code-block:: c

   struct uart_status {
   #ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
       uint32_t current_baudrate;
       uint32_t target_baudrate;
       uint8_t baudrate_detection_retry;
   #endif
   };

License
-------

Copyright (c) 2025 Netfeasa Ltd.

SPDX-License-Identifier: Apache-2.0

----

**Last Updated:** 2025-12-06
