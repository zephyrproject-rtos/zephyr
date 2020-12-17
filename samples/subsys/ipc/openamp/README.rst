.. _openAMP_sample:

OpenAMP Sample Application
##########################

Overview
********

This application demonstrates how to use OpenAMP with Zephyr. It is designed to
demonstrate how to integrate OpenAMP with Zephyr both from a build perspective
and code.

Building the application for lpcxpresso54114_m4
***********************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: lpcxpresso54114_m4
   :goals: debug

Building the application for lpcxpresso55s69_cpu0
*************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: lpcxpresso55s69_cpu0
   :goals: debug

Building the application for mps2_an521
***************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: mps2_an521
   :goals: debug

Building the application for v2m_musca
**************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: v2m_musca
   :goals: debug

Building the application for v2m_musca_b1
*****************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: v2m_musca_b1
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is master another is remote:

.. code-block:: console

   **** Booting Zephyr OS build zephyr-v1.14.0-2064-g888fc98fddaa ****
   Starting application thread!

   OpenAMP[master] demo started
   Master core received a message: 1
   Master core received a message: 3
   Master core received a message: 5
   ...
   Master core received a message: 99
   OpenAMP demo ended.


.. code-block:: console

   **** Booting Zephyr OS build zephyr-v1.14.0-2064-g888fc98fddaa ****
   Starting application thread!

   OpenAMP[remote] demo started
   Remote core received a message: 0
   Remote core received a message: 2
   Remote core received a message: 4
   ...
   Remote core received a message: 98
   OpenAMP demo ended.
