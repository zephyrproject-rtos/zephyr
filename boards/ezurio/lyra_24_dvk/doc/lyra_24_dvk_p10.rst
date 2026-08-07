.. zephyr:board:: lyra_24_dvk_p10

.. |Lyra 24 variant| replace:: P10
.. |Datasheet Ref| replace:: `Lyra 24 P datasheet`_
.. |SoC| replace:: BGM240PB22VNA
.. |SoC Datasheet Ref| replace:: `BGM240P datasheet`_
.. |Antenna Type| replace:: on-chip
.. |Number of IO| replace:: 26
.. |BLE TX Power| replace:: 10dBm
.. |Board Quoted| replace:: ``lyra_24_dvk_p10``
.. |User Guide Ref| replace:: `Lyra 24 P DVK user guide`_
.. |Schematics Ref| replace:: `Lyra 24 P10 DVK schematics`_

.. include:: lyra_24_dvk_common_1.rst.inc
.. include:: lyra_24_dvk_p_pinmap.rst.inc
.. include:: lyra_24_dvk_common_2.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: lyra_24_dvk_p10
   :goals: build

.. include:: lyra_24_dvk_common_3.rst.inc
.. include:: lyra_24_dvk_p_references.rst.inc

.. _Lyra 24 P10 DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-dvk-lyra-24p-devboard-integrated-antenna-10dbm-453-00142-k1
