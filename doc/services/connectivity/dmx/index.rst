.. _dmx:

DMX512
######

`DMX512-A <https://tsp.esta.org/tsp/documents/published_docs.php>`_ (ANSI E1.11 - 2024) is an
asynchronous serial protocol used in the entertainment industry for controlling lighting, effects,
and related equipment. A single DMX *universe* carries up to 512 data *slots* (channels) at 250
kbaud over an RS-485 physical layer.

Zephyr's DMX subsystem provides:

* Zero-copy frame reception via :c:func:`dmx_rx_claim` / :c:func:`dmx_rx_free`.
* Typed packet structs for NULL START code (dimmer data), test, ASCII text, UTF-8 text,
  manufacturer-specific, and System Information Packets (SIP).
* Start-code filtering so applications only receive the packet types they need.
* Per-frame 16-bit ones-complement checksum (for future SIP cross-packet verification).
* Signal-loss detection via :c:func:`dmx_rx_is_receiving` and an automatic timeout timer.
* Receive statistics for errors (:c:func:`dmx_rx_stats_get`).

Architecture
************

The subsystem is split into a generic core and backend drivers:

.. code-block:: text

   Application
       |
       v
   dmx.h API  (dmx_rx_claim, dmx_set_filter, ...)
       |
       v
   dmx_core.c  (frame assembly, packet parsing, spsc_pbuf)
       |
       v
   Backend driver  (dmx_uart_isr.c via UART IRQ API)
       |
       v
   UART peripheral + RS-485 transceiver

The **UART ISR backend** detects DMX BREAK conditions via UART framing errors and reads slot data
byte-by-byte in the IRQ callback. It works on any platform with ``CONFIG_UART_INTERRUPT_DRIVEN``
support.

Supported Platforms
*******************

The UART ISR backend works on any platform that provides:

* ``CONFIG_UART_INTERRUPT_DRIVEN`` support
* The ``framing-error-reporting`` devicetree property on its UART binding (see
  :zephyr_file:`dts/bindings/serial/uart-controller.yaml`)

It has been tested on PL011 (RP2040), nRF UARTE (nRF52840), STM32 USART (STM32L4), NXP LPUART
(i.MX RT1060), Atmel SAM0 SERCOM (SAMD21), and Espressif UART (ESP32-S3).

Hardware Setup
**************

A DMX receiver requires:

1. A UART peripheral configured for 250000 baud, 8N2.
2. An RS-485 transceiver (e.g., MAX485, SN75176) connecting the UART TX/RX to the DMX bus.
3. Optional GPIO pins for Driver Enable (DE) and Receiver Enable (RE) on the transceiver.

The DMX device is defined as a child node of the UART in devicetree:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/dmx/dmx.h>
   #include <zephyr/dt-bindings/gpio/gpio.h>

   &uart1 {
       status = "okay";
       current-speed = <DMX_BAUD_RATE>;

       dmx0: dmx {
           compatible = "zephyr,dmx-uart-isr";
           de-gpios = <&gpio0 11 GPIO_ACTIVE_HIGH>;
           re-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
       };
   };

The ``de-gpios`` and ``re-gpios`` properties are optional. If omitted, the application must manage
transceiver direction externally (e.g., hardware tying RE low and DE low for receive-only
operation).

Framing Error Reporting
=======================

DMX BREAK detection relies on UART framing error reporting, which varies across hardware. The
parent UART node should set the ``framing-error-reporting`` property to describe its behavior:

.. code-block:: devicetree

   &uart1 {
       framing-error-reporting = "LAST_READ";
       /* ... */
   };

See the :zephyr_file:`dts/bindings/serial/uart-controller.yaml` binding for the full list of values
and their meanings.

Configuration
*************

Enable the DMX subsystem with:

.. code-block:: kconfig

   CONFIG_DMX=y
   CONFIG_UART_INTERRUPT_DRIVEN=y
   CONFIG_GPIO=y  # if using DE/RE GPIOs

Key Kconfig options:

* ``CONFIG_DMX_RX_BUF_SIZE``: Size of the receive ring buffer in bytes (default 2048). Must hold at
  least two full frames (each frame is up to 512 slots plus a header).
* ``CONFIG_DMX_INIT_PRIORITY``: Device initialization priority (default 90).

Usage
*****

.. code-block:: c

   #include <zephyr/dmx/dmx.h>

   const struct device *dmx_dev = DEVICE_DT_GET(DT_NODELABEL(dmx0));

   /* Configure filter to accept NULL SC and test packets. */
   struct dmx_filter filter = {
       .sc_null = true,
       .sc_test = true,
   };
   dmx_set_filter(dmx_dev, &filter);

   /* Set mode and enable. */
   dmx_set_mode(dmx_dev, DMX_MODE_INPUT);
   dmx_enable(dmx_dev);

   while (true) {
       const struct dmx_rx_header *hdr;

       if (dmx_rx_claim(dmx_dev, &hdr, K_FOREVER) == 0) {
           switch (hdr->start_code) {
           case DMX_SC_NULL: {
               const struct dmx_null_packet *pkt = (const struct dmx_null_packet *)hdr;
               /* pkt->data[0] is DMX slot 1, etc. */
               process_dmx_data(pkt->data, pkt->slot_count);
               break;
           }
           case DMX_SC_TEST: {
               const struct dmx_test_packet *pkt = (const struct dmx_test_packet *)hdr;
               LOG_INF("Test packet: %s", kt->pass ? "PASS" : "FAIL");
               break;
           }
           }
           dmx_rx_free(dmx_dev, hdr);
       }
   }

Samples
*******

* :zephyr:code-sample:`dmx-input`: Receives and logs DMX frames, supporting all defined packet
  types.

API Reference
*************

.. doxygengroup:: dmx
