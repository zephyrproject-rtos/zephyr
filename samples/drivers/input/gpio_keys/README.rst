.. zephyr:code-sample:: samples_drivers_input_gpio_keys
   :name: Using GPIO-KEYS to Wake-Up system from a Power-Off state
   :relevant-api: input_events sys_poweroff


Overview
********

This example explores the system :ref:`poweroff` mode and wake-up sources
using gpio-keys. The application boots and waits for 10s before putting the
whole system in power-off mode. When the system is in power-off mode some
devices may lost the J-TAG/SWD interfaces and, as a consequence, it loses the
access to the program/debug tap interface. In the same way, user may lost reset
functionality and the 10s sleep counter helps to keep device in a safe state
to easy recover from power-off application. For those cases, the only way to
wake-up the device is using the user button on the board with wake-up
functionality.


Requirements
************

A GPIO that can be used as wake-up source be defined at devicetree. Some
platforms may requerie that pinctrl be defined to use such feature. The
power-off is available when selecting the :kconfig:option:`CONFIG_POWEROFF`
in the ``prj.conf`` file.


Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/input/gpio_keys
   :board: sam_v71_xult
   :goals: build flash
   :compact:


Sample Output
=================

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.5.0-583-g91585c5b3720 ***
   System Wake-Up: Going to Power-Off in 10s
   ..........
   Power-Off: Press the user button defined at gpio-key to Wake-Up the system
   *** Booting Zephyr OS build zephyr-v3.5.0-583-g91585c5b3720 ***
   System Wake-Up: Going to Power-Off in 10s
   ..........
   Power-Off: Press the user button defined at gpio-key to Wake-Up the system
