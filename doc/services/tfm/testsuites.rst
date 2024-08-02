Test Suites
###########

TF-M includes two sets of test suites:

* tf-m-tests - Standard TF-M specific regression tests
* psa-arch-tests - Test suites for specific PSA APIs (secure storage, etc.)

These test suites can be run from Zephyr via an appropriate sample application
in the samples/tfm_integration folder.

TF-M Regression Tests
*********************

The regression test suite can be run via the :ref:`tfm_regression_test` sample.

This sample tests various services and communication mechanisms across the
NS/S boundary via the PSA APIs. They provide a useful sanity check for proper
integration between the NS RTOS (Zephyr in this case) and the secure
application (TF-M).

PSA Arch Tests
**************

The PSA Arch Test suite, available via :ref:`tfm_psa_test`, contains a number of
test suites that can be used to validate that PSA API specifications are
being followed by the secure application, TF-M being an implementation of
the Platform Security Architecture (PSA).

Only one of these suites can be run at a time, with the available test suites
described via ``CONFIG_TFM_PSA_TEST_*`` KConfig flags:

Purpose
*******

The output of these test suites is required to obtain PSA Certification for
your specific board, RTOS (Zephyr here), and PSA implementation (TF-M in this
case).

They also provide a useful test case to validate any PRs that make meaningful
changes to TF-M, such as enabling a new TF-M board target, or making changes
to the core TF-M module(s). They should generally be run as a coherence check
before publishing a new PR for new board support, etc.
