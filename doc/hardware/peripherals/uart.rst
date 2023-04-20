.. _uart_api:

Universal Asynchronous Receiver-Transmitter (UART)
##################################################

Overview
********

Zephyr provides three different ways to access the UART peripheral. Depending
on the method, different API functions are used according to below sections:

1. :ref:`uart_polling_api`
2. :ref:`uart_interrupt_api`
3. :ref:`uart_async_api` using :ref:`dma_api`

Polling is the most basic method to access the UART peripheral. The reading
function, `uart_poll_in`, is a non-blocking function and returns a character
or `-1` when no valid data is available. The writing function,
`uart_poll_out`, is a blocking function and the thread waits until the given
character is sent.

With the Interrupt-driven API, possibly slow communication can happen in the
background while the thread continues with other tasks. The Kernel's
:ref:`kernel_data_passing_api` features can be used to communicate between
the thread and the UART driver.

The Asynchronous API allows to read and write data in the background using DMA
without interrupting the MCU at all. However, the setup is more complex
than the other methods.


Configuration Options
*********************

Most importantly, the Kconfig options define whether the polling API (default),
the interrupt-driven API or the asynchronous API can be used. Only enable the
features you need in order to minimize memory footprint.

Related configuration options:

* :kconfig:option:`CONFIG_SERIAL`
* :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`
* :kconfig:option:`CONFIG_UART_ASYNC_API`
* :kconfig:option:`CONFIG_UART_WIDE_DATA`
* :kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE`
* :kconfig:option:`CONFIG_UART_LINE_CTRL`
* :kconfig:option:`CONFIG_UART_DRV_CMD`


API Reference
*************

.. doxygengroup:: uart_interface


.. _uart_polling_api:

Polling API
===========

.. doxygengroup:: uart_polling


.. _uart_interrupt_api:

Interrupt-driven API
====================

.. doxygengroup:: uart_interrupt


.. _uart_async_api:

Asynchronous API
================

.. doxygengroup:: uart_async
