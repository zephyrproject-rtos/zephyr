.. zephyr:code-sample:: threadless_shell
   :name: Threadless shell

   Run shell in single threaded build

Overview
********

Demonstrate how to configure and use a shell in a single threaded build
(:kconfig:option:`CONFIG_MULTITHREADING` disabled). The sample uses the
UART shell backend using either the async
(:kconfig:option:`CONFIG_UART_ASYNC_API`) or interrupt driven
(:kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`) APIs. The sample
implements a single command, which simply echoes back the single
argument passed to it, along with the built-in "help" command.

.. code-block:: console

   uart:~$ sample foobar
   foobar
   uart:~$

Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/threadless
   :host-os: unix
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: run
   :compact:

To build for another board, change "nrf54l15dk/nrf54l15/cpuapp" above
to that board's name.

Sample Output
*************

.. code-block:: console

   uart:~$
