.. _twister_bsim_harness:

Bsim
####

The ``bsim`` harness extends the :ref:`script harness <twister_script_harness>` to support
BabbleSim tests.
During the build phase it copies the final executable (``zephyr.exe``) from
the build directory to BabbleSim's ``bin`` directory
(``${BSIM_OUT_PATH}/bin``). During the run phase it executes the test scripts
listed in ``tests_scripts``.

By default, the executable file name is (with dots and slashes
replaced by underscores): ``bs_<platform_name>_<test_path>_<test_scenario_name>``.
This name can be overridden with the ``bsim_exe_name`` option in
``harness_config`` section.

Additional ``bsim`` harness keys extending the :ref:`script harness <twister_script_harness>`:

bsim_exe_name: <string>
    If provided, the executable filename when copying to BabbleSim's bin
    directory, will be ``bs_<platform_name>_<bsim_exe_name>`` instead of the
    default based on the test path and scenario name.

Example configuration with a multi-images BabbleSim test where the advertiser
is build-only and the scanner references it via :ref:`required_applications <required_applications>`:

.. code-block:: yaml

    common:
      platform_allow:
        - nrf52_bsim/native
      harness: bsim
    tests:
      bluetooth.host.adv.extended.advertiser:
        build_only: true
        harness_config:
          bsim_exe_name: tests_bsim_bluetooth_host_adv_extended_prj_advertiser_conf
        extra_args:
          CONF_FILE=prj_advertiser.conf
      bluetooth.host.adv.extended.scanner:
        harness_config:
          bsim_exe_name: tests_bsim_bluetooth_host_adv_extended_prj_scanner_conf
          tests_scripts:
            - tests_scripts/run_adv_extended.sh
        extra_args:
          CONF_FILE=prj_scanner.conf
        required_applications:
          - application: bluetooth.host.adv.extended.advertiser
