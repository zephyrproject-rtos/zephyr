.. _dali_api:

DALI
####

DALI is the digital addressable lighting interface, a communication standard for professional lighting solutions.
This API is unstable and subject to change.

Basic Operation
***************

The DALI standard uses a communication model that is based on the exchange of frames via a bus system.
A frame is manchester encoded data, terminated by a stop conidition. The DALI standard
requires specific behaviour of tranmitter and receiver.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_DALI`
* :kconfig:option:`CONFIG_DALI_PWM`


API Reference
*************

.. doxygengroup:: dali_interface
