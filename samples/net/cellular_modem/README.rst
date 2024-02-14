.. zephyr:code-sample:: cellular-modem
   :name: Cellular modem

   Use a cellular modem to communicate with a UDP server.

Overview
********

This sample consists of a simple application which powers on
the modem, brings up the net interface, then sends a packet
with pseudo random data to the endpoint test-endpoint.com,
which is a publicly hosted server which runs the Python
script found in the server folder. DNS is used to look
up the IP of test-endpoint.com.

Notes
*****

This sample uses the devicetree alias modem to identify
the modem instance to use. The sample also presumes that
the modem driver creates the only PPP network interface.

Setup
*****

Start by setting up the devicetree with the required
devicetree node:

.. code-block:: devicetree

   /dts-v1/;

   / {
           aliases {
                   modem = &modem;
           };
   };

   &usart2 {
           pinctrl-0 = <&usart2_tx_pa2 &usart2_rx_pa3 &usart2_rts_pa1 &usart2_cts_pa0>;
           pinctrl-names = "default";
           current-speed = <115200>;
           hw-flow-control;
           status = "okay";

           modem: modem {
                   compatible = "quectel,bg9x";
                   mdm-power-gpios = <&gpioe 2 GPIO_ACTIVE_HIGH>;
                   mdm-reset-gpios = <&gpioe 3 GPIO_ACTIVE_HIGH>;
                   status = "okay";
           };
   };

Next, the UART API must be specified using ``CONFIG_UART_INTERRUPT_DRIVEN=y`` or
``CONFIG_UART_ASYNC_API=y``. The driver doesn't support UART polling.

Lastly, the APN must be configured using ``CONFIG_MODEM_CELLULAR_APN=""``.
