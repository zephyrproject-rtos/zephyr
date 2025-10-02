.. _apollo4p_evb_disp_shield_rev2:

Apollo4 Plus EVB Display Shield Rev2
####################################

Overview
********

Support for the Apollo4 Plus EVB display shield.

Apollo4 Plus EVB Display Shield
********************************

Pin Assignments of the Shield Connector
=======================================

+-----------------------+--------------------+------------------+
| Shield Connector Pin  | Function           | Apollo4 Plus Pin |
+=======================+====================+==================+
+-----------------------+--------------------+------------------+
| J1-29                 | IOM0_SCK           | 5                |
+-----------------------+--------------------+------------------+
| J1-31                 | IOM0_MOSI          | 6                |
+-----------------------+--------------------+------------------+
| J1-33                 | IOM0_MISO          | 7                |
+-----------------------+--------------------+------------------+
| J1-34                 | IOM0_CS            | 50               |
+-----------------------+--------------------+------------------+

Programming
***********

Set ``--shield apollo4p_evb_disp_shield_rev2`` when you
invoke ``west build`` or ``cmake`` in your Zephyr application.
For example when running the :zephyr:code-sample:`accel_polling` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: apollo4p_evb
   :shield: apollo4p_evb_disp_shield_rev2
   :goals: build
