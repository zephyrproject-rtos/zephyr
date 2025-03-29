.. zephyr:code-sample:: clock
   :name: Real-Time Clock (RTC)
   :relevant-api: rtc_interface

   Set and read the date/time from a Real-Time Clock,
   Print the time to OLED12864.

Overview
********

This sample demonstrates how to use the :ref:`rtc driver API <rtc_api>` to interact with a Real-Time Clock (RTC).  
The application sets the current date and time and displays it on an SSD1306 OLED screen (if available).  
It is intended for boards that include RTC hardware support.

**Note:** This sample does **not** print the time to the console.

.. figure:: img/rtc_display_demo.JPG
   :alt: RTC OLED Display Demo
   :width: 500px
   :align: center

   Real-Time Clock output shown on SSD1306 OLED screen.

Requirements
************

- A board with built-in RTC support (e.g., ``xiao_ble`` or other).
- SSD1306-compatible display with Zephyr driver support (e.g., via I2C).
- A supported expansion board or shield if needed (e.g., ``seeed_xiao_expansion_board``).

Building and Running
********************

Replace ``xiao_ble`` with the target board you are using if different.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/clock
   :board: xiao_ble
   :shield: seeed_xiao_expansion_board
   :goals: build flash
   :compact:

Once flashed, the device will initialize the RTC, set a default time, and display the current time on the OLED screen (if connected properly).

Expected Behavior
*****************

- The current date and time will be shown and updated periodically on the OLED display.
- No output is printed to the console.

Customization
*************

To modify the default date/time, edit the source code in `main.c`:

.. code-block:: c

   struct rtc_time tm = {
      .tm_year = 2025 - 1900,
      .tm_mon = 3 - 1,
      .tm_mday = 29,
      .tm_hour = 16,
      .tm_min = 30,
      .tm_sec = 0,
    };

Sample Output Device Support
****************************

- SSD1306 OLED (required for visual output)

Sample Files
************

- `main.c`: Implements RTC initialization, time setting, and display logic.
- `prj.conf`: Enables RTC and SSD1306 drivers.
