.. zephyr:board:: rm126x_dvk_rm1261

.. |RM126x variant| replace:: RM1261
.. |SX126x variant| replace:: SX1261
.. |LoRa TX Power| replace:: 15dBm
.. |Board Quoted| replace:: ``rm126x_dvk_rm1261``
.. |Schematics Ref| replace:: `RM1261 DVK schematics`_

.. include:: rm126x_dvk_common_1.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: rm126x_dvk_rm1261
   :goals: build

.. include:: rm126x_dvk_common_2.rst.inc

.. _RM1261 DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-dvk-rm1261-devboard
