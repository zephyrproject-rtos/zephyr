.. zephyr:code-sample:: c
   :name: UART RS485 Example
   :relevant-api: uart_interface

   Send and receive data over RS485 using the UART API.

Overview
********

The UART RS485 Example demonstrates how to send and receive data over a UART interface using RS485. It uses interrupt-driven communication to handle data transmission and reception efficiently.

The source code shows how to:

#. Retrieve a UART device from the :ref:`devicetree <dt-guide>`.
#. Set up the UART callback function for handling both transmission and reception interrupts.
#. Use a semaphore to coordinate data transmission completion.

Requirements
************

Your board must:

#. Have a UART peripheral configured to support RS485.
#. Use a proper configuration in the devicetree for the UART device.

Building and Running
********************

Build and flash the UART RS485 Example as follows, changing ``nucleo_f411re`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/uart_rs485
   :board: nucleo_f411re
   :goals: build flash
   :compact:

After flashing, the application will start sending data over the UART in RS485 mode, and messages about transmission and reception will be printed on the console. If a runtime error occurs, the sample exits without further console output.

Build errors
************

You will see a build error if the UART device specified in the code does not match any supported devices in the board devicetree.

On GCC-based toolchains, the error might look like this:

.. code-block:: none

   error: 'DEVICE_DT_GET(DT_NODELABEL(usart1))' undeclared here (not in a function)

Adding board support
********************

To add support for your board, add something like this to your devicetree:

.. code-block:: DTS

   &usart1 {
       pinctrl-0 = <&usart1_tx_pb6 &usart1_rx_pb7>;
       pinctrl-names = "default";
       current-speed = <115200>;
       rs485-enabled;
       rs485-de-gpios = <&gpiob 7 GPIO_ACTIVE_HIGH>;
       rs485-deassertion-time-de-us = <500>;
       status = "okay";
   };

The above sets your board's ``usart1`` configuration, specifying the TX and RX pins, enabling RS485 mode, and configuring the DE GPIO pin and de-assertion timing. The RS485-related properties help in properly configuring the RS485 communication, including de-assertion timing for RS485 DE line.

Tips:

- See :dtcompatible:`st,stm32-uart` for more information on defining UART peripherals in devicetree.

- If you're not sure what to do, check the devicetrees for supported boards that use the same SoC as your target. See :ref:`get-devicetree-outputs` for details.

- See :zephyr_file:`include/zephyr/dt-bindings/uart/uart.h` for the flags you can use in the devicetree.

- Proper RS485 settings are crucial for reliable communication. Ensure the correct timing values for the DE line in your devicetree.
