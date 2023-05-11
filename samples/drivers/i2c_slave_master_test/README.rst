.. _i2c_slave_master_test:

I2C Slave Master Test
#####################

Overview
********

This example utilizes the two I2Cs. One I2C is configured in a master-mode and
the other is configured in a slave mode.

The master writes three bytes in the Slave and then reads 10 bytes from the Slave.
This is repeated after every 10 seconds. When master reads form the Slave,
it first sends 12 and then reads 10 bytes.

Building and Running
********************

.. code-block:: console

    west build -p auto -b cy8cproto_063_ble

Sample Output
=============

 .. code-block:: console

    *** Booting Zephyr OS build deb243fcf94e  ***
    I2C- Slave test
    I2C Master Device is ready
    SLV RX: Received byte: 1
    SLV RX: Received byte: 2
    SLV RX: Received byte: 3
    SLV :Stop is received
    SLV RX: Received byte: 12
    SLV :Stop is received
    SLV TX:10
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV :Stop is received
    MSR RX: 01: 0x00
    MSR RX: 02: 0x0B
    MSR RX: 03: 0x0C
    MSR RX: 04: 0x0D
    MSR RX: 05: 0x0E
    MSR RX: 06: 0x0F
    MSR RX: 07: 0x10
    MSR RX: 08: 0x11
    MSR RX: 09: 0x12
    MSR RX: 10: 0x13
    SLV RX: Received byte: 2
    SLV RX: Received byte: 3
    SLV RX: Received byte: 4
    SLV :Stop is received
    SLV RX: Received byte: 12
    SLV :Stop is received
    SLV TX:11
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV :Stop is received
    MSR RX: 01: 0x0B
    MSR RX: 02: 0x0C
    MSR RX: 03: 0x0D
    MSR RX: 04: 0x0E
    MSR RX: 05: 0x0F
    MSR RX: 06: 0x10
    MSR RX: 07: 0x11
    MSR RX: 08: 0x12
    MSR RX: 09: 0x13
    MSR RX: 10: 0x14
    SLV RX: Received byte: 3
    SLV RX: Received byte: 4
    SLV RX: Received byte: 5
    SLV :Stop is received
    SLV RX: Received byte: 12
    SLV :Stop is received
    SLV TX:12
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV:Read request is processed
    SLV :Stop is received
    MSR RX: 01: 0x0C
    MSR RX: 02: 0x0D
    MSR RX: 03: 0x0E
    MSR RX: 04: 0x0F
    MSR RX: 05: 0x10
    MSR RX: 06: 0x11
    MSR RX: 07: 0x12
    MSR RX: 08: 0x13
    MSR RX: 09: 0x14
    MSR RX: 10: 0x15
