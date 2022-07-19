.. _openAMP_performance_sample:

OpenAMP performance Sample Application
######################################

Overview
********

This application demonstrates how to measure OpenAMP performance / throughput
with Zephyr.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp_performance
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is host another is remote:

.. code-block:: console

   OpenAMP[master] demo started
   Δpkt: 6193 (496 B/pkt) | throughput: 24573824 bit/s
   Δpkt: 6203 (496 B/pkt) | throughput: 24613504 bit/s
   Δpkt: 6203 (496 B/pkt) | throughput: 24613504 bit/s
   Δpkt: 6202 (496 B/pkt) | throughput: 24609536 bit/s
   Δpkt: 6202 (496 B/pkt) | throughput: 24609536 bit/s
   Δpkt: 6202 (496 B/pkt) | throughput: 24609536 bit/s
   Δpkt: 6202 (496 B/pkt) | throughput: 24609536 bit/s


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-2383-g147aee22f273  ***

   OpenAMP[remote] demo started
