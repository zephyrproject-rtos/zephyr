.. zephyr:code-sample:: poweroff
   :name: System poweroff with RTC and GPIO wakeup
   :relevant-api: sys_poweroff rtc_interface gpio_interface wuc_interface

   Power off a system and wake it with an RTC alarm or GPIO.

Overview
********

This sample configures an RTC alarm and a GPIO button as wakeup sources,
then calls :c:func:`sys_poweroff`. Hold the button for one second to enter
system-off mode. The system restarts when the RTC alarm expires after ten
seconds or when the button is pressed.

Wakeup routing is platform-specific and is described by the board
devicetree. If the RTC node has a ``wakeup-ctrls`` property and
:kconfig:option:`CONFIG_WUC` is enabled, the sample enables that wakeup
source after programming the alarm. The GPIO driver configures its wakeup
path when ``GPIO_INT_WAKEUP`` is requested.

Requirements
************

The board must provide:

* ``button`` and ``rtc`` devicetree aliases
* RTC alarm support
* a GPIO controller capable of ``GPIO_INT_WAKEUP``
* support for :c:func:`sys_poweroff`
* platform-specific routing for the RTC and GPIO wakeup sources

Building and Running
********************

Build and flash the sample, replacing ``<board>`` with a supported board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pm/poweroff
   :board: <board>
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   Poweroff example with GPIO and RTC wakeup sources
   Hold button 1 s to arm wake sources and power off.
   Power-off button held for 1 second; powering off.
   RTC wake alarm armed for 10 seconds.
   GPIO is set as wakeup source, click to wake!

After the RTC expires or the button is pressed, the system boots again.
