.. zephyr:board:: lyra_24_dvk_p20rf

.. |Lyra 24 variant| replace:: P20RF
.. |Datasheet Ref| replace:: `Lyra 24 P datasheet`_
.. |SoC| replace:: BGM240PB32VNN
.. |SoC Datasheet Ref| replace:: `BGM240P datasheet`_
.. |Antenna Type| replace:: external
.. |Number of IO| replace:: 26
.. |BLE TX Power| replace:: 20dBm
.. |Board Quoted| replace:: ``lyra_24_dvk_p20rf``
.. |User Guide Ref| replace:: `Lyra 24 P DVK user guide`_
.. |Schematics Ref| replace:: `Lyra 24 P20RF DVK schematics`_

.. include:: lyra_24_dvk_common_1.rst.inc
.. include:: lyra_24_dvk_p_pinmap.rst.inc
.. include:: lyra_24_dvk_common_2.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: lyra_24_dvk_p20rf
   :goals: build

.. include:: lyra_24_dvk_common_3.rst.inc
.. include:: lyra_24_dvk_p_references.rst.inc

.. _Lyra 24 P20RF DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-dvk-lyra-24p-devboard-rf-trace-pad-20dbm-453-00148-k1
