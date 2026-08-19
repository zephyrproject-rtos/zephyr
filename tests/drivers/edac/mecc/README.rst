.. _edac_mecc_test:

Testing Error Detection and Correction (NXP MECC)
#################################################

This test validates the generic EDAC API against the NXP MECC EDAC driver
(``drivers/edac/edac_mcux_mecc.c``) on i.MX RT1180.

The test covers:

* EDAC mandatory API (ECC log get/clear, counters, callback registration)
* Error injection API (when enabled):

  * correctable (single-bit) injection
  * uncorrectable (double-bit / multi) injection

Prerequisites
*************

* A platform/board with an enabled ``nxp,mecc`` devicetree node.
* For injection variants: ``CONFIG_EDAC_ERROR_INJECT=y``.

Twister variants
****************

* ``edac.mecc.production``: basic API checks, no error injection.
* ``edac.mecc.injection``: enables ``CONFIG_EDAC_ERROR_INJECT`` and runs both
  correctable and uncorrectable injection tests.

Building and Running
********************

This project can be built as follows for i.MX RT1180 EVK:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/edac/mecc
   :board: mimxrt1180_evk/mimxrt1189/cm33
   :goals: build flash
   :compact:

Sample output
*************

.. code-block:: console

   *** Booting Zephyr OS build f7baa79242c4 ***
   Running TESTSUITE edac_mecc
   ===================================================================
   START - test_driver_ready
    PASS - test_driver_ready in 0.001 seconds
   ===================================================================
   START - test_inject_correctable_error
    PASS - test_inject_correctable_error in 0.001 seconds
   ===================================================================
   START - test_inject_uncorrectable_error
    PASS - test_inject_uncorrectable_error in 0.001 seconds
   ===================================================================
   START - test_mandatory_api
    PASS - test_mandatory_api in 0.001 seconds
   ===================================================================
   TESTSUITE edac_mecc succeeded
