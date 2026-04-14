.. zephyr:code-sample:: zbus-proxy-agent-uart
   :name: zbus Proxy agent - UART
   :relevant-api: zbus_apis

   Forward zbus messages between separate boards over a dedicated UART using COBS framing.

Overview
********

This sample demonstrates zbus proxy agent communication across two boards using the UART backend.
It keeps the normal console UART for logs and uses a second UART exclusively for proxy traffic.
The overlays only point the sample at the hardware UART; the proxy agent itself is defined in code
with :c:macro:`ZBUS_PROXY_AGENT_UART_DEFINE`. In this sample, proxy-agent work uses the default
system workqueue, so the sample raises :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
accordingly.

The sample implements a request-response pattern where:

- The **requester** application publishes requests on ``request_channel`` and receives responses on
  ``response_channel``.
- The **responder** application receives requests on the shadow ``request_channel`` and publishes
  responses on ``response_channel``.
- The **UART proxy agent** serializes each proxy frame with COBS and sends it over a point-to-point
  UART link.

Board wiring used by the provided overlays
******************************************

This sample ships with board-specific overlays for the following pair:

- **nRF54L15 DK requester**: ``uart22`` on ``P1.11`` (TX) and ``P1.12`` (RX)
- **nRF9151 DK responder**: ``uart2`` on ``D7/P0.7`` (TX) and ``D6/P0.6`` (RX)

Connect the boards as follows:

- ``nRF54L15 P1.11 (TX)`` -> ``nRF9151 D6 / P0.6 (RX)``
- ``nRF54L15 P1.12 (RX)`` <- ``nRF9151 D7 / P0.7 (TX)``
- ``GND`` <-> ``GND``

Building and Running
********************

Build the requester for the nRF54L15 DK:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/proxy_agent_uart
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build

Build the responder for the nRF9151 DK from the companion application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/proxy_agent_uart/remote
   :board: nrf9151dk/nrf9151
   :goals: build

The requester keeps sending incrementing values. The responder increments each received value and
publishes it back.
