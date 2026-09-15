.. zephyr:code-sample:: tfm_secure_peripheral_st_secure_i2c
   :name: TF-M Secure I2C for ST boards

   Drive an I2C bus assigned to the Secure state by the STM32 GTZC from a TF-M
   secure partition, on behalf of the non-secure application.

Overview
********

This sample uses the STM32 Global TrustZone Controller (GTZC) to assign an I2C
bus instance and its GPIOs to the Secure image. A custom secure partition owns
the resulting peripheral and exposes it to the Zephyr non-secure application
(NSPE) over IPC, so the non-secure side never touches the bus itself.

The partition exposes two secure services, used here to talk to the STSAFE-A
secure element soldered on the I2C2 bus of the :zephyr:board:`b_u585i_iot02a`
board:

- ``TFM_SE_I2C_PROBE``: check that the device acknowledges its I2C address.
- ``TFM_SE_I2C_ECHO``: send a payload and read the answer back.

It requires TF-M to be built in isolation level 1; a higher level would require
a change in the TF-M core to let the secure user partition access the I2C
controller registers.

Hardware requirements
*********************

This sample requires an I2C device wired to the secured I2C2 bus:

- An I2C secure element (tested with an STSAFE-A) that implements the echo
  command, which the sample uses to validate the I2C exchange. Any I2C device
  acknowledging at address ``0x20`` will be detected, but the echo command and
  its response format are required for the sample to run fully.
- The board :zephyr:board:`b_u585i_iot02a` already has an STSAFE-A device
  connected to the I2C2 bus. The default implementation here supports
  revision D01 of the board. Refer to inline comments in
  ``se_i2c_partition/se_i2c_partition.c`` for older board revisions.

Key Files
*********

- ``se_i2c_partition/se_i2c_partition.c``: the secure partition. It configures
  the GTZC, initializes the I2C peripheral and implements the secure services.
- ``se_i2c_partition/tfm_manifest_list.yaml.in``: declares the partition to the
  TF-M build system.
- ``se_i2c_partition/tfm_se_i2c_partition.yaml.in``: the partition manifest,
  describing the services it exposes.
- ``src/se_i2c_partition.c``: the non-secure client of those services.

Building and Running
********************

This example is designed for the :zephyr:board:`b_u585i_iot02a` board. The default implementation targets the
``D01`` (or later) hardware revision.

To build and run the example for the default ``D01`` revision, follow these steps:

.. code-block:: bash

   west build -b b_u585i_iot02a/stm32u585xx/ns samples/tfm_integration/tfm_secure_peripheral/st_secure_i2c
   ./build/tfm/api_ns/regression.sh && west flash

If you are testing on a ``C02`` or older board revision, you must enable the corresponding configuration
switch in ``se_i2c_partition/se_i2c_partition.c``.

To run the example on another board, several modifications are required, including:

- Updating the I2C pin definitions in ``se_i2c_partition/se_i2c_partition.c`` to match the target board's
  I2C pins.
- Adjusting the I2C timing settings to ensure proper communication with the secure element.
- Modifying the I2C peripheral instance used in the code to match the target board's hardware configuration.

Sample Output
*************

.. code-block:: console

    [NOT] Booting TF-M v2.3.0**+g35210614a
    [NOT] Built Wed 03 Jun 2026 09:06:02 UTC
    [WRN] This device was provisioned with dummy keys. This device is NOT SECURE
    [INF] STSAFE powered (EN low)
    [INF] I2C2 initialized
    Creating an empty ITS flash layout.
    Creating an empty PS flash layout.
    [INF] [PS] Encryption alg: 0x5500200
    *** Booting Zephyr OS build v4.4.0-3756-ge4fe2cf94263 ***
    I2C probe: device ready
    I2C echo success. RX: 00 00 06 61 62 63 64 90 C2
