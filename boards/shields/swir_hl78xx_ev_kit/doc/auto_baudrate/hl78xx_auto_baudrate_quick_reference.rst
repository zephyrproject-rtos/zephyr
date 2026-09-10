.. _hl78xx_auto_baudrate_quick_reference:

HL78xx Auto Baud Rate - Quick Reference
=======================================

Enable Feature
--------------

.. code-block:: ini

   # In prj.conf
   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y  # Your target rate

Common Configurations
---------------------

Preset 1: Smart Default (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600,9600,57600,38400,115200"
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=4
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

- **Use when:** General purpose, known and unknown baud rates
- **Boot time impact:** ~50-100 ms (already at target), ~4-20 s (detection)
- **Why smart:** Full detection only runs if initial communication fails

Preset 2: Ultra-Fast Boot (Known Rate)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600"
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=1
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=1
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

- **Use when:** Modem is known to be at target rate
- **Boot time impact:** ~50-100 ms
- **Warning:** Fails if modem baud rate differs

Preset 3: High Speed
~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600,460800,230400,115200"
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=3
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

- **Use when:** High throughput required
- **Boot time impact:** ~50 ms (best), ~3-12 s (worst)
- **Note:** Ensure UART hardware supports high baud rates

Preset 4: Robust Production
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,38400,19200,9600,921600"
   CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=8
   CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=5
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

- **Use when:** Field deployment, maximum reliability
- **Boot time impact:** ~50 ms (best), ~40 s (worst)
- **Features:** Diagnostic fallback, comprehensive detection

Preset 5: Testing / Development (Non-Persistent)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: ini

   CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
   CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=n
   CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
   CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

- **Use when:** Temporary testing
- **Note:** Modem reverts baud rate on power cycle

Supported Baud Rates
--------------------

.. list-table::
   :header-rows: 1
   :widths: 15 45 30

   * - Rate
     - Kconfig Option
     - Use Case
   * - 9600
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_9600``
     - Legacy
   * - 19200
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_19200``
     - Legacy
   * - 38400
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_38400``
     - Standard
   * - 57600
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_57600``
     - Standard
   * - 115200
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_115200``
     - Default (recommended)
   * - 230400
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_230400``
     - High speed
   * - 460800
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_460800``
     - High speed
   * - 921600
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600``
     - Very high speed
   * - 3000000
     - ``CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_3000000``
     - Maximum (HL7812 only)

State Flow
----------

::

   Power ON
       ↓
   [START_WITH_TARGET=y?] → Try target first → [AUTOBAUD_AT_BOOT=y?]
       ↓                                        ↓
   Wait KSUP → [ONLY_IF_COMMS_FAIL=y?] → Try Target
       ↓                                        ↓
   Post-Restart → Communication Success? → Skip Detection
       ↓                     ↓
       |              Communication Fails
       |                     ↓
       +----------→ Detect Baud Rate
                             ↓
                     Rate Detected?
                             ↓
                    Switch to Target (AT+IPR)
                             ↓
                 [CHANGE_PERSISTENT=y?] → AT&W
                             ↓
                     Initialize Modem

Log Messages
------------

Success - Smart Default
~~~~~~~~~~~~~~~~~~~~~~~

::

   [hl78xx] Checking if modem is ready at target baud rate 921600...
   [hl78xx] Modem ready at target rate, skipping detection
   [hl78xx] Baud rate switch not needed

Detection Triggered
~~~~~~~~~~~~~~~~~~~

::

   [hl78xx] Modem not responding at target rate 921600, starting detection...
   [hl78xx] Trying baud rate: 9600
   [hl78xx] Modem responded at 9600 baud
   [hl78xx] Saving persistent baud rate with AT&W...

Non-Persistent Mode
~~~~~~~~~~~~~~~~~~~

::

   [hl78xx] Switching baud rate from 9600 to 921600
   [hl78xx] Non-persistent mode: skipping AT&W

Failure
~~~~~~~

::

   [hl78xx] Failed to detect modem baud rate
   [hl78xx] Retrying baud rate detection (attempt 2/3)

Troubleshooting
---------------

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Symptom
     - Likely Cause
     - Solution
   * - Failed to detect modem baud rate
     - Detection list missing rate
     - Add rates to detection list
   * - Times out on each rate
     - Modem not ready
     - Increase timeout
   * - Hangs on AT&W
     - UART mismatch
     - Fixed: UART reconfigured first
   * - Rate not persistent
     - Persistence disabled
     - Enable ``CHANGE_PERSISTENT``

AT Commands Reference
---------------------

.. code-block:: bash

   AT+IPR?
   AT+IPR=115200
   AT&W

API Functions (Internal)
------------------------

.. code-block:: c

   int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate);
   int hl78xx_detect_current_baudrate(struct hl78xx_data *data);
   int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate);

Performance Tips
----------------

1. Enable smart flags for fastest boot
2. Order detection list by likelihood
3. Use shorter timeouts in known environments
4. Enable persistence to avoid re-detection
5. Use ``AUTOBAUD_AT_BOOT`` only if timing is controlled

Compatibility
-------------

- HL7812 / HL7800
- Zephyr 4.4+
- All supported boards
- Smart detection reduces boot from ~20 s to ~50-100 ms

License
-------

Copyright (c) 2025 Netfeasa Ltd.

SPDX-License-Identifier: Apache-2.0

----

**Last Updated:** 2025-12-06
