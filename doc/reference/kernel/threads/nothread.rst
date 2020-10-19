.. _nothread:

Zephyr Without Threads
######################

Thread support is not necessary in some applications:

* Bootloaders
* Simple event-driven applications
* Examples intended to demonstrate core functionality

Thread support can be disabled in Zephyr by setting
:option:`CONFIG_MULTITHREADING` to ``n``.  Since this configuration has
a significant impact on Zephyr's functionality and testing of it has
been limited, there are conditions on what can be expected to work in
this configuration.

What Can be Expected to Work
****************************

These core capabilities shall function correctly when
:option:`CONFIG_MULTITHREADING` is disabled:

* The :ref:`build system <application>`

* The ability to boot the application to ``main()``

* :ref:`Interrupt management <interrupts_v2>`

* The system clock including :c:func:`k_uptime_get`

* Timers, i.e. :c:func:`k_timer`

* Non-sleeping delays e.g. :c:func:`k_busy_wait`.

* Specifically identified drivers in certain subsystems, listed below.

The expectations above affect selection of other features; for example
:option:`CONFIG_SYS_CLOCK_EXISTS` cannot be set to ``n``.

What Cannot be Expected to Work
*******************************

Functionality that will not work with :option:`CONFIG_MULTITHREADING`
includes but is not limited to:

* :c:func:`k_sleep()` and any other API that causes a thread to
  :ref:`api_term_sleep` except if using the :ref:`api_term_no-wait`
  path.

.. contents::
    :local:
    :depth: 1

Subsystem Behavior Without Thread Support
*****************************************

The sections below list driver and functional subsystems that are
expected to work to some degree when :option:`CONFIG_MULTITHREADING` is
disabled.  Subsystems that are not listed here should not be expected to
work.

Some existing drivers within the listed subsystems do not work when
threading is disabled, but are within scope based on their subsystem, or
may be sufficiently isolated that supporting them on a particular
platform is low-impact.  Enhancements to add support to existing
capabilities that were not originally implemented to work with threads
disabled will be considered.

Flash
=====

The :ref:`flash_api` is expected to work for all SoC flash peripheral
drivers.  Bus-accessed devices like serial memories may not be
supported.

*List/table of supported drivers to go here*

GPIO
====

The :ref:`gpio_api` is expected to work for all SoC GPIO peripheral
drivers.  Bus-accessed devices like GPIO extenders may not be supported.

*List/table of supported drivers to go here*

UART
====

A subset of the :ref:`uart_api` is expected to work for all SoC UART
peripheral drivers.

* Applications that select :option:`CONFIG_UART_INTERRUPT_DRIVEN` may
  work, depending on driver implementation.

* Applications that select :option:`CONFIG_UART_ASYNC_API` may
  work, depending on driver implementation.

* Applications that do not select either :option:`CONFIG_UART_ASYNC_API`
  or :option:`CONFIG_UART_INTERRUPT_DRIVEN` are expected to work.

*List/table of supported drivers to go here, including which API options
are supported*
