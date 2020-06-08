.. _entropy_sample:

Entropy
#######

Overview
********
Sample for the entropy gathering driver

Building and Running
********************

This project writes an infinite series of random numbers to the
console, 9 per second.

It can be built and executed on frdm_k64f as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/entropy
   :host-os: unix
   :board: frdm_k64f
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   Entropy Example! arm
   entropy device is 0x2000008c, name is ENTROPY_0
     0xd7  0x42  0xb0  0x7b  0x56  0x3b  0xc3  0x43  0x8a  0xa3
     0xfa  0xec  0xd8  0xc3  0x36  0xf8  0x7b  0x82  0x2b  0x39
