.. _nothread:

Operation without Threads
#########################

Thread support is not necessary in some applications:

* Bootloaders
* Simple event-driven applications
* Examples intended to demonstrate core functionality

Thread support can be disabled by setting
:kconfig:option:`CONFIG_MULTITHREADING` to ``n``.  Since this configuration has
a significant impact on Zephyr's functionality and testing of it has
been limited, there are conditions on what can be expected to work in
this configuration.

What Can be Expected to Work
****************************

These core capabilities shall function correctly when
:kconfig:option:`CONFIG_MULTITHREADING` is disabled:

* The :ref:`build system <application>`

* The ability to boot the application to ``main()``

* :ref:`Interrupt management <interrupts_v2>`

* The system clock including :c:func:`k_uptime_get`

* Timers, i.e. :c:func:`k_timer`

* Non-sleeping delays e.g. :c:func:`k_busy_wait`.

* Sleeping :c:func:`k_cpu_idle`.

* Pre ``main()`` drivers and subsystems initialization e.g. :c:macro:`SYS_INIT`.

* :ref:`kernel_memory_management_api`

* Specifically identified drivers in certain subsystems, listed below.

The expectations above affect selection of other features; for example
:kconfig:option:`CONFIG_SYS_CLOCK_EXISTS` cannot be set to ``n``.

What Cannot be Expected to Work
*******************************

Functionality that will not work with :kconfig:option:`CONFIG_MULTITHREADING`
includes majority of the kernel API:

* :ref:`threads_v2`

* :ref:`scheduling_v2`

* :ref:`workqueues_v2`

* :ref:`polling_v2`

* :ref:`semaphores_v2`

* :ref:`mutexes_v2`

* :ref:`condvar`

* :ref:`kernel_data_passing_api`

.. contents::
    :local:
    :depth: 1

Subsystem Behavior Without Thread Support
*****************************************

The sections below list driver and functional subsystems that are
expected to work to some degree when :kconfig:option:`CONFIG_MULTITHREADING` is
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

* Applications that select :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN` may
  work, depending on driver implementation.

* Applications that select :kconfig:option:`CONFIG_UART_ASYNC_API` may
  work, depending on driver implementation.

* Applications that do not select either :kconfig:option:`CONFIG_UART_ASYNC_API`
  or :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN` are expected to work.

*List/table of supported drivers to go here, including which API options
are supported*
