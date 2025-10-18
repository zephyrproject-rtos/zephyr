.. zephyr:code-sample:: zbus-uart-forwarder
   :name: zbus Proxy agent - UART forwarder
   :relevant-api: zbus_apis

   Forward zbus messages between different devices using UART.

Overview
********
This sample demonstrates zbus inter-device communication using UART forwarders between separate devices.
The sample implements a request-response pattern where:

- **Device A** acts as a requester, publishing requests on a master channel and receiving responses on a shadow channel
- **Device B** acts as a responder, receiving requests on a shadow channel and publishing responses on a master channel
- **UART forwarders** automatically synchronize channel data between devices using the zbus proxy functionality over UART

The ``common/common.h`` file defines shared channels using :c:macro:`ZBUS_MULTIDOMAIN_CHAN_DEFINE` with conditional
compilation to create master channels on one device and shadow channels on the other.

This architecture enables message passing between different devices while maintaining zbus's
publish-subscribe structure across device boundaries.

Building and Running
********************

This sample requires two separate devices connected via UART. Each device runs a different application:

For Device A (Requester):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/uart_forwarder/dev_a
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

For Device B (Responder):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/uart_forwarder/dev_b
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

Hardware Setup
==============

Connect the two devices via UART:

- Device A TX → Device B RX
- Device A RX → Device B TX
- Connect GND between devices

Sample Output
=============

Device A Output (Requester)
****************************

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1157-g7bf51d719a31 ***
   <inf> main: ZBUS Multidomain UART Forwarder Sample Application
   <inf> main: Channel request_channel is a master channel
   <inf> main: Channel response_channel is a shadow channel
   <inf> main: Published on channel request_channel. Request ID=1, Min=-1, Max=1
   <inf> main: Received message on channel response_channel
   <inf> main: Response ID: 1, Value: 1
   <inf> main: Published on channel request_channel. Request ID=2, Min=-2, Max=2
   <inf> main: Received message on channel response_channel
   <inf> main: Response ID: 2, Value: 0

Device B Output (Responder)
****************************

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1157-g7bf51d719a31 ***
   <inf> main: ZBUS Multidomain UART Forwarder Sample Application
   <inf> main: Channel request_channel is a shadow channel
   <inf> main: Channel response_channel is a master channel
   <inf> main: Received message on channel request_channel
   <inf> main: Request ID: 1, Min: -1, Max: 1
   <inf> main: Sending response: ID=1, Value=1
   <inf> main: Response published on channel response_channel
   <inf> main: Received message on channel request_channel
   <inf> main: Request ID: 2, Min: -2, Max: 2
   <inf> main: Sending response: ID=2, Value=-1
   <inf> main: Response published on channel response_channel
