.. _slcd-sample:

SLCD Panel Sample
#################

Overview
********

This sample demonstrates the SLCD (Segmented LCD) Panel driver API by performing
the following operations:

1. Obtains the SLCD panel device through a chosen node in the device tree
2. Retrieves and logs the panel's capabilities (segment type, number of positions, icons, supported display types)
3. Displays numbers (0-9) sequentially on all supported positions
4. Displays uppercase letters (A-Z) sequentially on all supported positions (if supported)
5. Displays lowercase letters (a-z) sequentially on all supported positions (if supported)
6. Displays icons sequentially (if supported)
7. Each display element remains visible for 500 milliseconds
8. Enters blink mode for 3 seconds to demonstrate the blink functionality

Supported Hardware
******************

The sample works with any SLCD panel device supported by Zephyr. Example boards:

- **OD-6010 Shield**: 6-digit 7-segment LCD panel with 6 icons
  - Segment type: 7-segment
  - Positions: 6 digits
  - Icons: 6 indicators
  - Supports: numbers and letters

For other SLCD panels, ensure your board configuration includes:

- A compatible SLCD controller driver
- A SLCD panel device defined in device tree
- A ``zephyr_slcd_panel`` chosen node pointing to the panel device

Building and Running
********************

Build the sample for a board with SLCD support:

.. code-block:: console

    west build -b <board> zephyr/samples/drivers/slcd

If using the OD-6010 shield on a supported board:

.. code-block:: console

    west build -b <board> zephyr/samples/drivers/slcd -- -DSHIELD=od_6010

Run the sample:

.. code-block:: console

    west flash

Expected Output
***************

When running successfully, the sample will output:

.. code-block:: console

    [00:00:00.000,000] <inf> slcd_sample: SLCD panel sample for <device_name>
    [00:00:00.010,000] <inf> slcd_sample: Panel capabilities:
    [00:00:00.010,000] <inf> slcd_sample:   Segment type: 7-segment
    [00:00:00.010,000] <inf> slcd_sample:   Number of positions: 6
    [00:00:00.010,000] <inf> slcd_sample:   Number of icons: 6
    [00:00:00.010,000] <inf> slcd_sample:   Support number: 1
    [00:00:00.010,000] <inf> slcd_sample:   Support letter: 1
    [00:00:00.010,000] <inf> slcd_sample: Displaying numbers...
    [00:00:00.510,000] <inf> slcd_sample: Displaying uppercase letters...
    [00:00:03.510,000] <inf> slcd_sample: Displaying lowercase letters...
    [00:00:06.510,000] <inf> slcd_sample: Displaying icons...
    [00:00:09.510,000] <inf> slcd_sample: Entering blink mode for 3 seconds...
    [00:00:12.510,000] <inf> slcd_sample: Disabling blink mode
    [00:00:12.510,000] <inf> slcd_sample: Cycle complete. Restarting...

The display will cycle through:

1. **Numbers (0-9)**: Each digit is displayed on all positions for 500ms
2. **Uppercase Letters (A-Z)**: Each letter is displayed on all positions for 500ms
3. **Lowercase Letters (a-z)**: Each letter is displayed on all positions for 500ms
4. **Icons**: Each icon is displayed for 500ms (if the panel has icons)
5. **Blink Mode**: Panel blinks for 3 seconds

Device Tree Configuration
**************************

The sample uses the ``zephyr_slcd_panel`` chosen node to locate the SLCD panel device.

Add to your device tree or overlay:

.. code-block:: dts

    chosen {
        zephyr,slcd-panel = &my_slcd_panel;
    };

Or on a board with an SLCD controller:

.. code-block:: dts

    &slcd {
        my_slcd_panel: slcd-panel@0 {
            compatible = "vendor,panel-name";
            reg = <0>;
            status = "okay";
            segment-type = "7-segment";
            num-positions = <6>;
            num-icons = <6>;
            /* ... rest of panel configuration ... */
        };
    };

Configuration Options
*********************

The sample behavior can be configured via ``prj.conf``:

- ``CONFIG_SLCD``: Enable SLCD driver support
- ``CONFIG_SLCD_PANEL_LOG_LEVEL``: Control SLCD panel driver logging
- ``LOG_DEFAULT_LEVEL``: Control sample application logging

Implementation Details
**********************

The sample implements the following workflow:

1. **Device Initialization**: Uses ``DEVICE_DT_GET(DT_CHOSEN(zephyr_slcd_panel))`` to obtain the panel device
2. **Capability Query**: Calls ``slcd_panel_get_capabilities()`` to determine supported features
3. **Display Loop**: For each supported display type:
   - Iterates through all available values (0-9 for numbers, A-Z/a-z for letters, 0-num_icons for icons)
   - Displays each value on all positions simultaneously
   - Waits 500ms before moving to the next value
4. **Blink Demonstration**: Enables blink mode for 3 seconds to showcase dynamic control
5. **Cycle Repetition**: Returns to step 3 and repeats indefinitely

API Functions Used
******************

The sample demonstrates the following SLCD Panel API functions:

- ``slcd_panel_get_capabilities()``: Retrieve panel specifications
- ``slcd_panel_show_number()``: Display numeric digits (0-9)
- ``slcd_panel_show_letter()``: Display letter characters (A-Z, a-z)
- ``slcd_panel_show_icon()``: Display special icons
- ``slcd_panel_blink()``: Control panel blinking mode

See :ref:`slcd_panel_interface` for API documentation.

Troubleshooting
***************

**Sample doesn't start**: Ensure the SLCD panel device is defined in device tree and the chosen node is set correctly.

**Display shows wrong output**: Verify the panel configuration in device tree matches the actual hardware.

**No logging output**: Check that ``CONFIG_LOG`` is enabled and ``LOG_DEFAULT_LEVEL`` is set appropriately.

**Blink mode not working**: Some SLCD controllers may not support blink mode. Check driver documentation.
