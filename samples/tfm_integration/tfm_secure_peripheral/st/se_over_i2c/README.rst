.. zephyr:code-sample:: se_over_i2c
    :name: TF-M I2C Communication

    Enables communication between a TF-M application and an SE device using I2C.

Overview
********

This sample demonstrates how a TF-M secure partition can own an I2C peripheral
and expose it to the Zephyr non-secure application (NSPE) through PSA services.
It requires TF-M to be built in isolation level 1; a higher level would require
a change in the TF-M core to let the secure user partition access the I2C2 controller
SoC registers.

Hardware requirements
*********************

This sample requires an external I2C device wired to the secured I2C2 bus:

- An I2C secure element (tested with an STSAFE-A) that implements the echo
  command, which the sample uses to validate the I2C exchange. Any I2C device
  acknowledging at address ``0x20`` will be detected, but the echo command is
  required for the sample to run fully.
- The board :zephyr:board:`b_u585i_iot02a` already has an STSAFE-A device
  connected to the I2C2 bus. The default implementation here supports
  revision D01 of the board. Refer to inline comments in se_over_i2c.c for older
  board revisions.

Key Files
*********

- ``se_over_i2c/se_over_i2c.c``: Contains the implementation of the I2C communication logic, including
  service calls to the TF-M secure services for I2C operations.
- ``se_over_i2c/tfm_manifest_list.yaml.in``: Defines the manifest for the TF-M partition, specifying the
  services and permissions.

Building and Running
********************

This example is designed for the :zephyr:board:`b_u585i_iot02a` board. The default implementation targets the
``D01`` (or later) hardware revision.

To build and run the example for the default ``D01`` revision, follow these steps:

.. code-block:: bash

   west build -b b_u585i_iot02a/stm32u585xx/ns samples/tfm_integration/tfm_secure_peripheral/st/se_over_i2c
   ./build/tfm/api_ns/regression.sh && west flash

If you are testing on a ``C02`` or older board revision, you must enable the corresponding configuration
switch in ``se_over_i2c/se_over_i2c.c``.

To run the example on another board, several modifications are required, including:

- Updating the I2C pin definitions in ``se_over_i2c/se_over_i2c.c`` to match the target board's I2C pins.
- Adjusting the I2C timing settings in ``se_over_i2c/se_over_i2c.c`` to ensure proper communication with the SE device.
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
    I2C probe: hal_status=0 isr=0x0001
    I2C echo success. RX: 00 00 06 61 62 63 64 90 C2
