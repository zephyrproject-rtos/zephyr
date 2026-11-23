SDHC Emulator Tests
###################

These tests verify the SDHC emulator driver across multiple card personalities
(SD, eMMC, SDIO) and framework configurations. All tests run on ``native_sim``
and require no hardware.

To build and run all test variants:

.. code-block:: bash

   west twister -T tests/drivers/sdhc/emul --platform native_sim
