.. _edac_ibecc_tests:

Testing Error Detection and Correction
######################################

Tests verify API and use error injection method to inject
errors.

Prerequisites
*************

IBECC should be enabled in BIOS. This is usually enabled in the default
BIOS configuration. Verify following is enabled::

   Intel Advanced Menu -> Memory Configuration -> In-Band ECC ->  <Enabled>

Verify also operational mode with::

   Intel Advanced Menu -> Memory Configuration -> In-Band ECC Operation Mode -> 2

For injection test Error Injection should be enabled.

Error Injection
===============

IBECC includes a feature to ease the verification effort such as Error
Injection capability. This helps to test the error checking, logging and
reporting mechanism within IBECC.

In order to use Error Injection user need to use BIOS Boot Guard 0 profile.

Additionally Error Injection need to be enabled in the following BIOS menu::

   Intel Advanced Menu -> Memory Configuration -> In-Band ECC Error -> <Enabled>

.. note::

   Due to high security risk Error Injection capability should not be
   enabled for production. Due to this reason test has production configuration
   and debug configuration. The main difference is that debug configuration
   includes Error Injection.

Building and Running
********************

This project can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: tests/subsys/edac/ibecc
   :board: ehl_crb
   :goals: build
   :compact:

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-747-gd421737f433e  ***
   Running TESTSUITE ibecc
   ===================================================================
   START - test_edac_dummy_api
    PASS - test_edac_dummy_api in 0.001 seconds
   ===================================================================
   START - test_ibecc_initialized
   Test ibecc driver is initialized
    PASS - test_ibecc_initialized in 0.004 seconds
   ===================================================================
   START - test_ibecc_injection
    SKIP - test_ibecc_injection in 0.001 seconds
   ===================================================================
   TESTSUITE ibecc succeeded
