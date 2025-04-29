.. zephyr:code-sample:: uart-circular-dma
   :name: UART circular mode
   :relevant-api: uart_interface

   Read data from the console and echo it back using a circular DMA configuration.

Overview
********

This sample demonstrates how to use STM32 UART serial driver with DMA in circular mode.
It reads data from the console and echoes it back.

By default, the UART peripheral that is normally assigned to the Zephyr shell
is used, hence the majority of boards should be able to run this sample.

Building and Running
********************

Build and flash the sample as follows, changing ``nucleo_g071rb`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/uart/circular_dma
   :board: nucleo_g071rb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    Enter message to fill RX buffer size and press enter :
    # Type e.g. :
    # "Lorem Ipsum is simply dummy text of the printing and typesetting industry.
    # Lorem Ipsum has been the industry's standard dummy text ever since the 1500s,
    # when an unknown printer took a galley of type and scrambled it to make a type specimen book.
    # It has survived not only five centuries, but also the leap into electronic typesetting,
    # remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset
    # sheets containing Lorem Ipsum passages, and more recently with desktop publishing software
    # like Aldus PageMaker including versions of Lorem Ipsum."

    Lorem Ipsum is simply dummy text of the printing and typesetting industry.
    Lorem Ipsum has been the industry's standard dummy text ever since the 1500s,
    when an unknown printer took a galley of type and scrambled it to make a type specimen book.
    It has survived not only five centuries, but also the leap into electronic typesetting,
    remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset
    sheets containing Lorem Ipsum passages, and more recently with desktop publishing software
    like Aldus PageMaker including versions of Lorem Ipsum.
