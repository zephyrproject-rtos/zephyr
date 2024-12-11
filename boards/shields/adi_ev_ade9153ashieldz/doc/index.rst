.. _adi_ev_ade9153ashieldz:

Analog Devices EV-ADE9153ASHIELDZ Shield
########################################

Overview
********

Analog Devices EV-ADE9153ASHIELDZ is a single-phase energy measurement shield compatible with
Arduino Uno, Arduino Zero, and ESP8266. EV-ADE9153ASHIELDZ features the ADE9153A IC and an onboard
shunt resistor for line current measurement. The shield enables quick evaluation and prototyping of
energy measurement systems that use the ADE9153A Energy Metering ICs with mSure® autocalibration.
Using mSure autocalibration, the designer can calibrate the shield to measure energy with 1%
accuracy over the dynamic range. To simplify the implementation of larger systems, Arduino library
and application examples are included.

.. figure:: EV-ADE9153ASHIELDZANGLE-web.png
   :align: center
   :alt: Analog Devices EV-ADE9153ASHIELDZ shield

   Analog Devices EV-ADE9153ASHIELDZ shield (Credit: Analog Devices)

Requirements
************

This shield can only be used with a board which provides a configuration for Arduino connectors and
defines node aliases for SPI and GPIO interfaces (see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=adi_ev_ade9153ashieldz`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ade9153a_shield/
   :board: nrf52833dk/nrf52833
   :shield: adi_ev_ade9153ashieldz
   :goals: build

.. _Analog Devices EV-ADE9153ASHIELDZ Shield :
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-ade9153a.html

