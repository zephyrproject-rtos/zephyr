.. _arduino_uno_click:

Arduino UNO click shield
########################

Overview
********

The Arduino UNO click is an extension to the Arduino UNO R3 headers.
It's a simple shield that converts Arduino UNO R3 headers to two mikroBUS
host sockets that allow you to connect many other click shields to your
board.
In other words, the Arduino UNO click will generally be used by other
shields using the mikroBUS interface.

Two mikroBUS headers are exposed by the overlay: ``mikrobus_header_1`` and
``mikrobus_header_2``, each corresponding to a socket on the Arduino UNO
click shield.

The first socket (``mikrobus_header_1``) is the default socket which is
assigned the node label ``mikrobus_header`` in the overlay.

More information about the shield can be found at
`Arduino UNO click shield website`_.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino R3 connector.

The board must also define node aliases for arduino Serial,
SPI and I2C interfaces (see :ref:`shields` for more details).

Connecting shields should use the first socket (``mikrobus_header_1``). This
socket is assigned the ``mikrobus_header`` node label.

Programming
***********

Include ``--shield arduino_uno_click`` when you invoke ``west build`` with
other mikroBUS shields. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: sam_v71_xult/samv71q21
   :gen-args: -DOVERLAY_CONFIG=overlay-802154.conf
   :shield: arduino_uno_click,atmel_rf2xx_mikrobus
   :goals: build

References
**********

.. target-notes::

.. _Arduino UNO click shield website:
   https://www.mikroe.com/arduino-uno-click-shield
