.. _ecdsa:

Crypto : ECDSA
###########

Overview
********

A simple sample that demonstrates the [web3-iot-sdk](https://github.com/machinefi/web3-iot-sdk)https://github.com/machinefi/web3-iot-sdk
implementation of the ecdsa algorithm.web3-iot-sdk is an SDK for connecting IOT devices to Web3 applications.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/web3stream/crypto/ecdsa
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

Success to generate a key pair: key id : 2147483616
Exported a pair key len 32
0b ab cd 63 6d fb 46 b8 d0 df 18 86 a9 e5 a1 1b 6e 61 f2 da d9 a4 bb 0b 09 10 35 58 a1 b9 44 de
Exported a public key len 65
04 f1 8c cb a6 3a 9d 93 a0 c1 63 15 34 17 d5 ff 35 1e 22 25 c6 b9 c2 e5 2f 74 f0 92 fa 99 b8 2c 8f 72 73 b8 dc 26 f1 cd 28 42 c5 22 5b 54 0d 82 df 03 b5 d2 6a 2f 6b 45 79 9c ec 92 ef 5c e2 7c 24
Success to verify message

Exit QEMU by pressing :kbd:`CTRL+C`.
