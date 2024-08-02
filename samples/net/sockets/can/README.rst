.. zephyr:code-sample:: socket-can
   :name: SocketCAN
   :relevant-api: bsd_sockets socket_can

   Send and receive raw CAN frames using BSD sockets API.

Overview
********

The socket CAN sample is a server/client application that sends and receives
raw CAN frames using BSD socket API.

The application consists of these functions:

* Setup function which creates a CAN socket, binds it to a CAN network
  interface, and then installs a CAN filter to the socket so that the
  application can receive CAN frames.
* Receive function which starts to listen the CAN socket and prints
  information about the CAN frames.
* Send function which starts to send raw CAN frames to the bus.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/can`.

Requirements
************

You need a CANBUS enabled board like :ref:`nucleo_l432kc_board` or
:ref:`stm32f072b_disco_board`.

Building and Running
********************

Build the socket CAN sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/can
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the nucleo_l432kc:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/can
   :host-os: unix
   :board: nucleo_l432kc
   :goals: run
   :compact:
