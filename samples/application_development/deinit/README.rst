.. zephyr:code-sample:: deinit
   :name: Device deinit
   :relevant-api: gpio_interface spi_interface device_model

Overview
********

This sample demonstrates how to initialize and deinitialize device
drivers at runtime, which allows switching between different devices
and device drivers which share pins or share hardware.

Detailed description
********************

This sample demonstrates how to share a SPI CS (chip-select) pin
between a SPI peripheral and a GPIO port. The SPI CS pin chosen is
connected to an LED on the board used for the sample. This sample
blinks the LED, by either manually toggling the pin using the GPIO
port, or performing a mock SPI transaction with ``SPI_HOLD_ON_CS``
set which keeps the SPI CS pin asserted from the start of the SPI
transaction until we call :c:func:`spi_release`.

We use ``zephyr,deferred-init`` to prevent initializing the SPI
device driver during boot. The sample does the following after
booting:

1. Configure the CS pin as an output in active state with
   :c:func:`gpio_pin_configure_dt`
2. Wait
3. Configure the CS pin as an input with
   :c:func:`gpio_pin_configure_dt`

This is the first blink

4. Initialize the SPI device driver with :c:func:`device_init`
5. Perform mock SPI transaction with :c:func:`spi_transceive`
6. Wait
7. Release SPI with :c:func:`spi_release`

This is the second blink

8. Deinitialize SPI device driver with :c:func:`device_deinit`
9. Configure the CS pin as an output in active state with
   :c:func:`gpio_pin_configure_dt`
10. Wait
11. Configure the CS pin as an input
    :c:func:`gpio_pin_configure_dt`

This is the third blink

12. Initialize the SPI device driver with :c:func:`device_init`
13. Perform mock SPI transaction with :c:func:`spi_transceive`
14. Wait
15. Release SPI with :c:func:`spi_release`

This is the last blink.

16. print ``sample complete`` to console

If any errors occur during the sample, the error is printed to
the console and the sample is stopped.

Porting guide
*************

Use the following devicetree overlay example to create an
overlay for your board:

.. code-block:: devicetree

   &port0 {
           status = "okay";
   };

   &spi0 {
           status = "okay";
           zephyr,deferred-init;
           cs-gpios = <&port0 0 GPIO_ACTIVE_LOW>;
   };

   / {
           zephyr,user {
                   spi = <&spi0>;

                   /*
                    * Must match &spi0 cs-gpios and should
                    * preferably be a pin connected to an LED
                    * on the board, which illuminates when
                    * active.
                    */
                   cs-gpios = <&port0 0 GPIO_ACTIVE_LOW>;
           };
   };
