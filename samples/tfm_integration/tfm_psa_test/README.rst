.. _tfm_psa_test:

TF-M Platform Security Architecture Test Sample
###############################################

Overview
********

Run PSA test suites tests with Zephyr and TFM.

The PSA tests are implemented in the psa-arch-tests repo: https://github.com/ARM-software/psa-arch-tests

This sample is supported for platforms that have a port in psa-arch-tests.
See sample.yaml for a list of supported platforms.

Building and Running
********************

You must choose a suite via the CONFIG_TFM_PSA_TEST_* configs.

Only one of these suites can be run at a time, with the test suite set via one
of the following kconfig options:

* ``CONFIG_TFM_PSA_TEST_CRYPTO``
* ``CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE``
* ``CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE``
* ``CONFIG_TFM_PSA_TEST_STORAGE``
* ``CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION``

You can indicate the desired test suite at build time via a config flag:

   .. code-block:: bash

     $ west build samples/tfm_integration/tfm_psa_test/ \
       -p -b mps2_an521_ns -t run -- \
       -DCONFIG_TFM_PSA_TEST_STORAGE=y

Note that not all test suites are valid on all boards.

On Target
=========

Refer to :ref:`tfm_ipc` for detailed instructions.

On QEMU:
========

Refer to :ref:`tfm_ipc` for detailed instructions.
Following is an example based on ``west build``

   .. code-block:: bash

      $ west build samples/tfm_integration/tfm_psa_test/ -p -b mps2_an521_ns -t run -- -DCONFIG_TFM_PSA_TEST_STORAGE=y

Sample Output
=============

   .. code-block:: console

      *** Booting Zephyr OS build zephyr-v2.5.0-456-g06f4da459a99  ***

      ***** PSA Architecture Test Suite - Version 1.0 *****

      Running.. Storage Suite
      ******************************************

      TEST: 401 | DESCRIPTION: UID not found check
      [Info] Executing tests from non-secure

      [Info] Executing ITS tests
      [Check 1] Call get API for UID 6 which is not set
      [Check 2] Call get_info API for UID 6 which is not set
      [Check 3] Call remove API for UID 6 which is not set
      [Check 4] Call get API for UID 6 which is removed
      [Check 5] Call get_info API for UID 6 which is removed
      [Check 6] Call remove API for UID 6 which is removed
      Set storage for UID 6
      [Check 7] Call get API for different UID 5
      [Check 8] Call get_info API for different UID 5
      [Check 9] Call remove API for different UID 5

      [Info] Executing PS tests
      [Check 1] Call get API for UID 6 which is not set
      [Check 2] Call get_info API for UID 6 which is not set
      [Check 3] Call remove API for UID 6 which is not set
      [Check 4] Call get API for UID 6 which is removed
      [Check 5] Call get_info API for UID 6 which is removed
      [Check 6] Call remove API for UID 6 which is removed
      Set storage for UID 6
      [Check 7] Call get API for different UID 5
      [Check 8] Call get_info API for different UID 5
      [Check 9] Call remove API for different UID 5

      TEST RESULT: PASSED

      ******************************************

      [...]

      TEST: 417 | DESCRIPTION: Storage assest capacity modification check
      [Info] Executing tests from non-secure

      [Info] Executing PS tests
      Test Case skipped as Optional PS APIs not are supported.

      TEST RESULT: SKIPPED (Skip Code=0x0000002B)

      ******************************************

      ************ Storage Suite Report **********
      TOTAL TESTS     : 17
      TOTAL PASSED    : 11
      TOTAL SIM ERROR : 0
      TOTAL FAILED    : 0
      TOTAL SKIPPED   : 6
      ******************************************

      Entering standby..
