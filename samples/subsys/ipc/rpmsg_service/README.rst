.. _RPMsg_Service_sample:

RPMsg Service sample Application
################################

Overview
********

RPMsg Service is an abstraction created over OpenAMP that makes initialization
and endpoints creation process easier.
This application demonstrates how to use RPMsg Service in Zephyr. It is designed
to demonstrate how to integrate RPMsg Service with Zephyr both from a build
perspective and code.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/rpmsg_service
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Building the application for mps2_an521
***************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/rpmsg_service
   :board: mps2_an521
   :goals: debug

Building the application for v2m_musca_b1
*****************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/rpmsg_service
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

   RPMsg Service [master] demo started
   Master core received a message: 1
   Master core received a message: 3
   Master core received a message: 5
   ...
   Master core received a message: 99
   RPMsg Service demo ended.


.. code-block:: console

   **** Booting Zephyr OS build zephyr-v1.14.0-2064-g888fc98fddaa ****
   Starting application thread!

   RPMsg Service [remote] demo started
   Remote core received a message: 0
   Remote core received a message: 2
   Remote core received a message: 4
   ...
   Remote core received a message: 98
   RPMsg Service demo ended.
