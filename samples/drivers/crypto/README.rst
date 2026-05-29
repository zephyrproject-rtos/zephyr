.. zephyr:code-sample:: crypto
   :name: Crypto
   :relevant-api: crypto

   Use the crypto APIs to perform various encryption/decryption operations.

Overview
********
An example to illustrate the usage of :ref:`crypto APIs <crypto_api>`.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/crypto
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    [general] [INF] main: Encryption Sample

    [general] [INF] cbc_mode: CBC Mode

    [general] [INF] cbc_mode: cbc mode ENCRYPT - Match

    [general] [INF] cbc_mode: cbc mode DECRYPT - Match

    [general] [INF] ctr_mode: CTR Mode

    [general] [INF] ctr_mode: ctr mode ENCRYPT - Match

    [general] [INF] ctr_mode: ctr mode DECRYPT - Match

    [general] [INF] ccm_mode: CCM Mode

    [general] [INF] ccm_mode: CCM mode ENCRYPT - Match

    [general] [INF] ccm_mode: CCM mode DECRYPT - Match

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
