.. zephyr:code-sample:: stm32_pm_gpio_wkup_src
   :name: GPIO as wake-up pin
   :relevant-api: sys_poweroff gpio_interface

   Use a GPIO pin as wake-up pin.

Overview
********

This sample is a minimum application to demonstrate usage of GPIO as wake-up pin on STM32 SoCs
in two scenarios: Poweroff and Suspend-to-RAM. Each scenario can be selected by enabling the
corresponding Kconfig option:

* ``CONFIG_SAMPLE_SCENARIO_POWEROFF`` for the Poweroff scenario (default for this sample)
* ``CONFIG_SAMPLE_SCENARIO_S2RAM`` for the Suspend-to-RAM scenario

This sample requires a devicetree ``wkup-src`` alias pointing to a node with the ``gpios`` property.

On most STM32 boards, at least one of the on-board button(s) is connected to GPIO pin that
can be used as wake-up pin. On these boards, make ``wkup-src`` an alias of the node which
represents this button (one of the children of node ``/gpio_keys``).

For some boards, none of the on-board button(s) are connected to GPIO pins that can be used
as wake-up pins. On those boards, create a new ``/gpio_keys`` child node whose ``gpios``
property corresponds to one of the wake-up pin GPIOs in the board overlay, and use it as the
``wkup-src`` alias; in this case, a jumper wire must be used to make the wake-up pin enter
its active state.

In the following paragraphs, the term *WAKEUP button pin* will be used to refer to the GPIO pin
designated by the ``wkup-src`` alias and *pressing the WAKEUP button* will be used to designate
the action of making the WAKEUP button pin enter its active state, regardless of whether the pin
is actually connected to a button or not.

Usage of wake-up pins in Poweroff
---------------------------------

In the Poweroff scenario, the WAKEUP button pin is configured as GPIO input and wake-up pin, then
the system is powered off by calling the :c:func:`sys_poweroff` function after the number of
milliseconds specified by ``CONFIG_SAMPLE_BUSY_WAIT_TIME_MS`` (4000 by default).

Pressing the WAKEUP button once Poweroff state has been entered should power on the SoC, which will
start as if the ``nRESET`` pin had been pulled low. On most STM32 series, software can distinguish
resets triggered by a wake-up pin from resets triggered by the ``nRESET`` pin: if possible, the
sample will print a dedicated message to the console:

.. code-block:: console

   Reset from low-power state detected.
   Continuing sample execution...

.. note::

   On STM32 platforms, the Poweroff state is mapped to the STM32 low-power mode with the lowest power
   consumption, which is usually called *Standby* or *Shutdown* (depending on the SoC series).

Usage of wake-up pins in Suspend-to-RAM
---------------------------------------

In the Suspend-to-RAM scenario, the WAKEUP button pin is configured as GPIO input and wake-up pin,
then registers a GPIO callback and enables interrupts on the pin. This makes the GPIO callback
called if the WAKEUP button pin becomes active, regardless of whether the system is in Run mode
or Suspend-to-RAM state (in the former case, the interrupt is seen by the GPIO controller; in
the latter case, it is seen by the Power Controller). The GPIO callback prints a message to the
console then signals a semaphore.

The sample will then enter the demonstration loop which repeats forever:
* A message is printed then the sample (busy-)waits for ``CONFIG_SAMPLE_BUSY_WAIT_TIME_MS`` milliseconds.

* The sample attempts to acquire the semaphore with a ``CONFIG_SAMPLE_BUTTON_PRESS_TIMEOUT_MS`` timeout.
  If the button has not been pressed yet, the system should enter Suspend-to-RAM state.

* While in Suspend-to-RAM state, pressing the WAKEUP button will wake up the system. The registered GPIO
  callback will be invoked, print its message and signal the semaphore. This allows the sample's main
  thread (which was waiting on the semaphore) to resume execution, ensuring the system does not enter
  in Suspend-to-RAM state again immediately (because no threads are available to run).

* Once the semaphore is signaled or the timeout expires, the sample continues execution:

  * If the semaphore was acquired, a success message is printed and the loop starts over.

  * If the timeout expired, a timeout message is printed and the sample pauses until the WAKEUP button
    is pressed, then the loop starts over.

.. note::

   Wake-up pins only support **single-edge** detection (rising or falling edge).

.. _gpio-as-a-wkup-pin-src-sample-requirements:

Requirements
************

The SoC must implement support for the :ref:`hwinfo_api` API and :ref:`poweroff` API.

Additionally, the Suspend-to-RAM scenario requires existence of such a low-power mode
on the target SoC and software support for the feature (indicated by the presence of
a :dtcompatible:`zephyr,power-state` node with property ``power-state-name`` set to
the value ``"suspend-to-ram"`` in Devicetree).

A per-board overlay must be added to this sample to provide the ``wkup-src`` alias.

Building and Running
********************

Build and flash wkup_pins as follows, changing ``nucleo_u5a5zj_q`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/power_mgmt/wkup_pins
   :board: nucleo_u5a5zj_q
   :goals: build flash
   :compact:

After reset, the board's LED is ON. It will turn off when the system is powered off
or enters in the Suspend-to-RAM state (depending on selected scenario).
