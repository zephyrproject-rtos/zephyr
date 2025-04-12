TM1637 Display Driver
======================

The TM1637 driver provides support for controlling 7-segment displays using the
TM1637 chip.

Supported Features:
-------------------
- GPIO control for CLK and DIO pins
- Display of numeric characters

Driver Configuration
--------------------
To use the TM1637 driver, configure the following options in your project:

.. code-block:: prn
    CONFIG_DISPLAY_TM1637=y

Device Tree Bindings
---------------------
The TM1637 driver is configured via Device Tree. An example of how to configure
the driver in your `dts` file is as follows:

.. code-block:: dts
    &gpio0 {
        tm1637: tm1637_0 {
            compatible = "hw,tm1637";
            status = "okay";
            clk-gpios = <&gpio0 17 GPIO_ACTIVE_HIGH>;
            dio-gpios = <&gpio0 18 GPIO_ACTIVE_HIGH>;
        };
    };

Functionality
-------------
The driver supports basic functionality for controlling the 7-segment display:
- Initializing the TM1637 chip
- Displaying numbers or characters on the display

For additional configuration options and advanced features, refer to the source code.

Example Usage
-------------
The following example demonstrates how to use the TM1637 driver to display numbers:

.. code-block:: c
    #include <zephyr/kernel.h>
    #include <zephyr/device.h>
    #include <zephyr/drivers/display_tm1637.h>

    #define TM1637_NODE DT_NODELABEL(tm1637)

    void main(void)
    {
        const struct device *tm1637 = DEVICE_DT_GET(TM1637_NODE);

        if (!device_is_ready(tm1637)) {
            printk("TM1637 device not ready\n");
            return;
        }

        /* Displaying a single raw segment */
        tm1637_display_raw_segments(tm1637, 1, 0x3f);

        /* Or displaying a number */
        tm1637_display_number(tm1637, 1234, true);
    }

