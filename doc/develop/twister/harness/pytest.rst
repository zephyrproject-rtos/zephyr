.. _twister_pytest_harness:

Pytest
######

The :ref:`pytest harness <integration_with_pytest>` is used to execute pytest
test suites in the Zephyr test. The following options apply to the pytest harness:

.. _pytest_root:

pytest_root: <list of pytest testpaths> (default pytest)
    Specify a list of pytest directories, files or subtests that need to be
    executed when a test scenario begins to run. The default pytest directory is
    ``pytest``. After the pytest run is finished, Twister will check if
    the test scenario passed or failed according to the pytest report.
    Environment variables and Zephyr module directory variables are expanded
    (see :ref:`twister_module_dir_vars`).
    As an example, a list of valid pytest roots is presented below:

    .. code-block:: yaml

        harness_config:
          pytest_root:
            - "pytest/test_shell_help.py"
            - "../shell/pytest/test_shell.py"
            - "/tmp/test_shell.py"
            - "~/tmp/test_shell.py"
            - "$ZEPHYR_BASE/samples/subsys/testsuite/pytest/shell/pytest/test_shell.py"
            - "$ZEPHYR_HAL_NORDIC_MODULE_DIR/tests/pytest/test_hal.py"  # path inside a module
            - "pytest/test_shell_help.py::test_shell2_sample"  # select pytest subtest
            - "pytest/test_shell_help.py::test_shell2_sample[param_a]"  # select pytest parametrized subtest

.. _pytest_args:

pytest_args: <list of arguments> (default empty)
    Specify a list of additional arguments to pass to ``pytest`` e.g.:
    ``pytest_args: [‘-k=test_method’, ‘--log-level=DEBUG’]``. Note that
    ``--pytest-args`` can be passed multiple times to pass several arguments
    to the pytest.

.. _pytest_dut_scope:

pytest_dut_scope: <function|class|module|package|session> (default function)
    The scope for which ``dut`` and ``shell`` pytest fixtures are shared.
    If the scope is set to ``function``, DUT is launched for every test case
    in python script. For ``session`` scope, DUT is launched only once.


  The following is an example yaml file with pytest harness_config options,
  default pytest_root name "pytest" will be used if pytest_root not specified.
  please refer the examples in samples/subsys/testsuite/pytest/.

  .. code-block:: yaml

      common:
        harness: pytest
      tests:
        pytest.example.directories:
          harness_config:
            pytest_root:
              - pytest_dir1
              - $ENV_VAR/samples/test/pytest_dir2
        pytest.example.files_and_subtests:
          harness_config:
            pytest_root:
              - pytest/test_file_1.py
              - test_file_2.py::test_A
              - test_file_2.py::test_B[param_a]

.. _required_devices:

required_devices: <list of required device entries> (default empty)
    Specify additional DUTs required for a multi-DUT test scenario.
    Each entry configures one extra device to reserve and flash alongside
    the main DUT. An empty entry ``{}`` reserves a second device with
    the same platform and application as the main DUT.

    Multi-DUT testing is supported for hardware device testing and
    ``native_sim`` simulation. QEMU
    is not supported. For simulation targets, Twister
    automatically creates the required placeholder DUT entries, no
    hardware map is needed. For hardware testing, provide a hardware map
    file (``--hardware-map``) with a matching entry per required device.
    More details in
    :ref:`twister_multi_duts_testing` section.

    Each entry supports the following optional fields:


    platform: <string> (optional, defaults to current test's platform)
        Platform to use for this required device. If not specified,
        the same platform as the main DUT is used.

    application: <string> (optional, defaults to current test application)
        Test application ID to flash on this required device. When
        specified, Twister builds the application separately and assigns its build
        directory to the reserved DUT before flashing.
        If not specified, the same application as the main DUT is used.

        It uses same mechanism as :ref:`required_applications <required_applications>`.

    path: <string> (optional)
        Directory path where Twister should search for the application
        specified in ``application``. Can be an absolute path or a path
        relative to the directory containing the test's YAML file.
        Environment variables and Zephyr module directory variables are
        expanded (see :ref:`twister_module_dir_vars`). If not specified,
        Twister searches in the same directory as the referring test's
        YAML file.

    fixture: <list of fixture names> (optional, defaults to empty)
        List of fixture names that must be present on the reserved device.
        See :ref:`Fixtures <twister_fixtures>` for details.

        Fixtures support an optional parameter suffix using the ``name:param``
        syntax (e.g., ``io_adapter:channel_a``). When the main DUT has a fixture
        with a parameter, Twister uses that parameter value to match required
        devices - only devices carrying the **same parameter** for that fixture
        name are considered. This ensures that physically connected DUT pairs
        (e.g., two boards wired together and registered with the same fixture
        parameter in the hardware map) are always reserved together and not
        mixed with unrelated boards.

        Example hardware map entries for a paired setup:

        .. code-block:: yaml

            - id: 01
              platform: nrf52840dk/nrf52840
              serial: /dev/ttyACM0
              fixtures:
                - io_adapter:channel_a
            - id: 02
              platform: nrf52840dk/nrf52840
              serial: /dev/ttyACM1
              fixtures:
                - io_adapter:channel_a

        With ``fixture: [io_adapter]`` on both the main DUT and the required
        device, Twister selects only boards sharing the same ``channel_a``
        parameter, guaranteeing the physically paired boards are picked.


    Example configurations with multiple required devices:

    .. code-block:: yaml

        tests:
          # Two DUTs, same platform and application
          multidut.basic:
            harness_config:
              required_devices:
                - {}
          # Second DUT fixed to a specific platform
          multidut.fixed_platform:
            harness_config:
              required_devices:
                - platform: nrf52840dk/nrf52840
          # Second DUT flashed with a different application
          multidut.other_app:
            harness_config:
              required_devices:
                - application: multidut.basic
