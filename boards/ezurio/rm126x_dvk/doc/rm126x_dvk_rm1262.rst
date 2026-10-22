.. zephyr:board:: rm126x_dvk_rm1262

.. |RM126x variant| replace:: RM1262
.. |SX126x variant| replace:: SX1262
.. |LoRa TX Power| replace:: 22dBm
.. |Board Quoted| replace:: ``rm126x_dvk_rm1262``
.. |Schematics Ref| replace:: `RM1262 DVK schematics`_

.. include:: rm126x_dvk_common_1.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: rm126x_dvk_rm1262
   :goals: build

.. include:: rm126x_dvk_common_2.rst.inc

.. _RM1262 DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-dvk-rm1262-devboard
