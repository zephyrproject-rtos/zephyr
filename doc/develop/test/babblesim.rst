.. _babblesim:

BabbleSim tests support
#######################

Tests dedicated for BabbleSim simulator require special build and run workflow
and at this moment they cannot be run by Twister test runner. To solve this
problem special Pytest plugin was designed.

Requirements
************

- Python >= 3.8

Python modules:

- pytest >= 7.1.2
- strictyaml >= 1.6.1
- filelock >= 3.7.1
- pytest-xdist >= 2.5.0

Pytest plugin
*************

In usual case Pytest test runner is used most often for Python unit tests.
However, thanks to the extensive system of hooks and plugins Pytest make it
possible to define how tests gathering should looks like and what should be
performed during test run. It enables to design own plugin dedicated for
BabbleSim tests.

Using Pytest test runner allows to use ready-made functionalities as:

- scanning directories for tests' definitions
- filtering tests by name (including/excluding tests)
- tests execution and storing tests' results
- generating several types of test report (``junit`` (``xml``), ``json``,
  ``html``)
- splitting tests into sub-groups (by ``split-tests`` plugin)
- running tests in parallel (by ``xdist`` plugin)

How to run tests?
*****************

To run BabbleSim tests defined to run by Pytest, following environmental
variables have to be exported (of course below paths may vary depending on
user's Zephyr and BabbleSim installation place):

::

    export ZEPHYR_BASE="${HOME}/zephyrproject/zephyr"
    export BSIM_OUT_PATH="${HOME}/bsim/"
    export BSIM_COMPONENTS_PATH="${HOME}/bsim/components/"

Next below command should be called (from ZEPHYR_BASE directory level):

::

    pytest tests/bluetooth/bsim_bt

Exemplary output could look like:

::

    ========================= test session starts ==========================
    platform linux -- Python 3.8.10, pytest-6.2.4, py-1.10.0, pluggy-0.13.1
    rootdir: /home/redbeard/zephyrproject/zephyr
    plugins: typeguard-2.13.3, forked-1.4.0, xdist-2.5.0
    collected 8 items

    tests/bluetooth/bsim_bt/bsim_test_advx/bs_testsuite.yaml .        [ 12%]
    tests/bluetooth/bsim_bt/bsim_test_app/bs_testsuite.yaml ....      [ 62%]
    tests/bluetooth/bsim_bt/bsim_test_eatt/bs_testsuite.yaml ..       [ 87%]
    tests/bluetooth/bsim_bt/bsim_test_gatt/bs_testsuite.yaml .        [100%]

    ===================== 8 passed in 67.68s (0:01:07) =====================

For more verbose output special arguments could be added to basic command:

::

    pytest tests/bluetooth/bsim_bt -vv -o log_cli=true

How it works
************

BabbleSim tests are defined in ``bs_testsuite.yaml`` files (similar to "classic"
Twister approach with defining tests in ``testcase.yaml`` files). At the
beginning Pytest scan directories and try to find ``bs_testsuite.yaml`` and
tests defined inside it. Exemplary ``bs_testsuite.yaml`` for
``tests/bluetooth/bsim_bt/bsim_test_gatt`` test may look like below:

.. code-block:: yaml

    tests:
      bluetooth.bsim.gatt:
        platform_allow:
          - nrf52_bsim
        tags:
          - bluetooth
        extra_args:
          - '-DCMAKE_C_FLAGS="-Werror"'
          - "-DCONFIG_COVERAGE=y"
          - "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        bsim_config:
          devices:
            - testid: gatt_client
            - testid: gatt_server
          phy_layer:
            name: bs_2G4_phy_v1
            sim_length: 60e6

This YAML file define necessary information to build and run BabbleSim tests.
Next after collecting all tests, Pytest filter tests if some filter rules were
passed in CLI.

Each test consists of two phases:

1.  Building process
        -   get CMake extra arguments from ``bs_testsuite.yaml`` file and
            prepare CMake command
        -   run CMake
        -   run Ninja generator
2.  Run simulation
        -   parse ``bsim_config`` from ``bs_testsuite.yaml`` file and prepare
            suitable commands to run each simulated devices and physical layer
        -   run simulation
        -   if some error/failure occurs during run simulation then mark test as
            ``FAILED`` - otherwise as ``PASSED``

There is also possibility to generate final report. More information about this
can be found in chapter `reporting (from Pytest)`_.

Minimal test configuration
**************************

When test source code is already prepared, then to make it possible to build
and run them by Pytest, the ``bs_testsuite.yaml`` file have to be defined.
Let's assume, that exemplary test source is placed in
``tests\bluetooth\bsim_bt\some_test`` directory. Yaml file have to be defined
inside them.

Minimal ``bs_testsuite.yaml`` file consists list of test scenarios with
BabbleSim configs. Each test scenario must have unique name among all test
scenarios (otherwise Pytest will rise an error). This is very significant,
because this name will be used to mark BabbleSim simulation ID and to create
test output directory.

Exemplary basic ``bs_testsuite.yaml`` file could look like:

.. code-block:: yaml

    tests:
      bluetooth.bsim.some_test:
        bsim_config:
          devices:
            - testid: name_of_first_device
            - testid: name_of_second_device
          phy_layer:
            name: bs_2G4_phy_v1
            sim_length: 60e6

After running Pytest, it will scan such prepared yaml file and will save
``bluetooth.bsim.some_test`` test on list of available tests. During test
execution following CMake command will be called:

::

    cmake -B${ZEPHYR_BASE}/bsim_tests_out/bluetooth_bsim_some_test/build \
    -S${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/some_test -GNinja \
    -DBOARD_ROOT=${ZEPHYR_BASE} -DBOARD=nrf52_bsim -DCONF_FILE=prj.conf

As it can be observed some options are set by default:

1.  output build directory is placed in
    ``${ZEPHYR_BASE}/bsim_tests_out/bluetooth_bsim_some_test/build``
2.  build system is ``Ninja``
3.  the target board is ``nrf52_bsim``, and root dir for search this board
    definition is ``${ZEPHYR_BASE}``
4.  configuration file is ``prj.conf``

At this moment only last option (configuration file) can be changed by user in
``bs_testsuite.yaml`` file. It will be described more detailed in chapter
`extra_args`_.

After running this CMake command, the Ninja generator is run. Next built
``zephyr.exe`` application is copied into ``${BSIM_OUT_PATH}/bin`` directory and
renamed into ``bs_nrf52_bsim_bluetooth_bsim_some_test``.

Finally for such defined tests in yaml file, following BabbleSim command is
prepared:

::

    ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_bluetooth_bsim_some_test -s=bluetooth_bsim_some_test -d=0 -testid=name_of_first_device &
    ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_bluetooth_bsim_some_test -s=bluetooth_bsim_some_test -d=1 -testid=name_of_second_device &
    ${BSIM_OUT_PATH}/bin/bs_2G4_phy_v1 -s=bluetooth_bsim_some_test -D=2 -sim_length=60e6

It is crated basing on those rules:

1.  ``-s=bluetooth_bsim_some_test`` - simulation ID is the same as test scenario
    name (dots are replaced by underscore)
2.  ``-testid=name_of_first_device`` - test ID for particular simulated devices
    is taken from yaml file from ``bsim_config -> devices`` options list
3.  Physical layer name (``bs_2G4_phy_v1``) and simulation length
    ``-sim_length=60e6`` are taken from yaml file from
    ``bsim_config -> phy_layer`` options list

Additional features
*******************

extra_args
==========

If user would like to pass some extra arguments to CMake command, it can be done
by define ``extra_args`` option in yaml file. Listed arguments will be joined
entirely to CMake call, so they should already start with "-D" (or similar)
characters. By this option special conf file name could be passed. If it is not
passed explicitly in ``extra_args`` the default name is ``prj.conf``.

For example such defined ``extra_args`` in ``bs_testsuite.yaml`` file:

.. code-block:: yaml

    tests:
      bluetooth.bsim.app_split:
        extra_args:
          - "-DCONF_FILE=prj_split.conf"
        bsim_config:
          ...

will be used in CMake command as follow:

::

    cmake -B${ZEPHYR_BASE}/bsim_tests_out/bluetooth_bsim_app_split/build \
    -S${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/bsim_test_app -GNinja \
    -DBOARD_ROOT=${ZEPHYR_BASE} -DBOARD=nrf52_bsim -DCONF_FILE=prj_split.conf

extra_run_args in bsim_config
=============================

If user would like to pass some extra arguments to run simulated device or
physical layer it can be done by ``extra_run_args`` option added in proper
place in ``bsim_config`` option

For example, such defined ``bsim_config`` with ``extra_run_args`` options in
``bs_testsuite.yaml`` file:

.. code-block:: yaml

    tests:
      bluetooth.bsim.app_split:
        bsim_config:
          devices:
            - testid: peripheral
              extra_run_args:
                - "-rs=23"
            - testid: central
              extra_run_args:
                - "-rs=6"
          phy_layer:
            name: bs_2G4_phy_v1
            sim_length: 20e6
            extra_run_args:
              - "-v=5"

will be used in BabbleSim run command as follow:

::

    ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_bluetooth_bsim_app_split -s=bluetooth_bsim_app_split -d=0 -testid=peripheral -rs=23 &
    ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_bluetooth_bsim_app_split -s=bluetooth_bsim_app_split -d=1 -testid=central -rs=6 &
    ${BSIM_OUT_PATH}/bin/bs_2G4_phy_v1 -s=bluetooth_bsim_app_split -D=2 -sim_length=60e6 -v=5


common
======

Similar to "classic" Twister test defining approach, there is also possibility
to define ``common`` option used by all test scenarios defined in
``bs_testsuite.yaml`` file.

When "common" entry is used in bs_testsuite.yaml file, then test scenario
options can be updated with following rules:

1.  If the same option occurs in ``common`` and test scenario entries and
    they are a **list** (like for example ``extra_args``) then join them
    together.
2.  If the same options occur in "common" and test scenario entries and
    they are **NOT a list** (like for example ``bsim_config``), then do **NOT**
    overwrite test scenario option by common one.
3.  If some option occurs in ``common`` and not occur in tests scenario entry,
    then add this option to test scenario opitons.

For example, such defined ``extra_args`` in ``common`` option in
``bs_testsuite.yaml`` file:

.. code-block:: yaml

    common:
      extra_args:
        - '-DCMAKE_C_FLAGS="-Werror"'

    tests:
      bluetooth.bsim.app_split:
        extra_args:
          - "-DCONF_FILE=prj_split.conf"
        bsim_config:
          ...

      bluetooth.bsim.app_split_low_lat:
        extra_args:
          - "-DCONF_FILE=prj_split_low_lat.conf"
        bsim_config:
          ...

will be used during defining CMake command for both tests:

::

    # for bluetooth.bsim.app_split:
    cmake -B${ZEPHYR_BASE}/bsim_tests_out/bluetooth_bsim_app_split/build \
    -S${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/bsim_test_app -GNinja \
    -DBOARD_ROOT=${ZEPHYR_BASE} -DBOARD=nrf52_bsim -DCONF_FILE=prj_split.conf \
    -DCMAKE_C_FLAGS="-Werror"

    # for bluetooth.bsim.app_split_low_lat:
    cmake -B${ZEPHYR_BASE}/bsim_tests_out/bluetooth_bsim_app_split/build \
    -S${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/bsim_test_app -GNinja \
    -DBOARD_ROOT=${ZEPHYR_BASE} -DBOARD=nrf52_bsim \
    -DCONF_FILE=prj_split_low_lat.conf -DCMAKE_C_FLAGS="-Werror"


built_exe_name
==============

Some of tests do not need to be rebuilt every time and they can base on once
built exe file. For this case ``built_exe_name`` option can be used. It defines
how exe file name should looks like in ``${BSIM_OUT_PATH}/bin/`` directory.
Normally this exe name is based on test scenario name as follow:

::

    bs_{platform_name}_{test_name}

If two (or more) tests have explicitly defined the same ``built_exe_name``
names, then this exe file will be built **only once during Pytest call**.

Exemplary ``bs_testsuite.yaml`` file can looks like:

.. code-block:: yaml

    tests:
      bluetooth.bsim.app_split:
        bsim_config:
          built_exe_name: "bs_nrf52_bsim_bluetooth_bsim_app_split"
          ...

      bluetooth.bsim.app_split_encrypted:
        bsim_config:
          built_exe_name: "bs_nrf52_bsim_bluetooth_bsim_app_split"
          ...

In this case when ``bluetooth.bsim.app_split`` test is run, after building
process, built ``zephyr.exe`` file will be moved into ``${BSIM_OUT_PATH}/bin/``
directory and renamed into ``bs_nrf52_bsim_bluetooth_bsim_app_split``. When
``bluetooth.bsim.app_split_encrypted`` test is run, the building process will be
skipped and this test will use already built exe file.

.. warning::

    ``built_exe_name`` has to be unique among whole BabbleSim tests to avoid
    exe file overwriting. So, it is recommended to use for this at least one
    affected test name (which ensures uniqueness).

Information about what exe applications were already built are stored in
``${ZEPHYR_BASE}/bsim_tests_out/build_info.json`` file. It is very important
when tests are run in parallel and two tests which are based on the same exe
file, are run at the same time. Then to avoid built overwriting, special lock
mechanism is used and, in this situation, when first test start building
process, second one wait until first will finish its job.

Log saving
==========

During run BabbleSim tests by Pytest, following logs are saved:

1.  Internal logs from plugin are saved in
    ``${ZEPHYR_BASE}/bsim_tests_out/bsim_test_plugin.log`` file. They include
    information about performed actions (CMake, Ninja, BabbleSim commands) and
    location of saved logs from building and simulation.
2.  Logs from building process (``cmake_out.log`` and ``ninja_out.log``) are
    saved in ``${ZEPHYR_BASE}/bsim_tests_out/${TEST_NAME}/build`` directory.
3.  Logs from executed simulation (from each simulated devices like ``central``
    or ``peripheral`` and from physical layer) are stored in
    ``${ZEPHYR_BASE}/bsim_tests_out/${TEST_NAME}`` directory.


Test selection (from Pytest)
============================

Test selection based on its node ID
-----------------------------------

To specify particular test, it should be passed full path to specific yaml file,
and test name after double colon (::) during Pytest call.

Usage examples:

::

    # to run only bluetooth.bsim.eatt_encryption test:
    pytest tests/bluetooth/bsim_bt/bsim_test_eatt/bs_testsuite.yaml::bluetooth.bsim.eatt_encryption

    # # to run both bluetooth.bsim.eatt_encryption and bluetooth.bsim.eatt_collision tests:
    pytest \
    tests/bluetooth/bsim_bt/bsim_test_eatt/bs_testsuite.yaml::bluetooth.bsim.eatt_encryption \
    tests/bluetooth/bsim_bt/bsim_test_eatt/bs_testsuite.yaml::bluetooth.bsim.eatt_collision


More information about how this filter works can be found here:
https://docs.pytest.org/en/latest/example/markers.html#selecting-tests-based-on-their-node-id

Test selection based on its name
--------------------------------

In some cases it will be easier to select particular test or group of tests by
their name. To do this, special argument ``-k`` (from "keyword") has to be added
during Pytest call.

Usage examples:

::

    # to run only bluetooth.bsim.gatt test:
    pytest tests/bluetooth/bsim_bt -k "gatt"

    # to run only bluetooth.bsim.gatt and bluetooth.bsim.advx tests:
    pytest tests/bluetooth/bsim_bt -k "gatt or advx"

    # to run all tests without bluetooth.bsim.app_ tests:
    pytest tests/bluetooth/bsim_bt -k "not app"

More information about how this filter works can be found here:
https://docs.pytest.org/en/latest/example/markers.html#using-k-expr-to-select-tests-based-on-their-name

Reporting (from Pytest)
=======================

To generate JUnit report ``--junitxml`` argument with report path has to be
added to Pytest call. For example:

::

    pytest tests/bluetooth/bsim_bt \
    --junitxml=${ZEPHYR_BASE}/bsim_tests_out/report.xml

Parallelization (from Pytest)
=============================

To run tests in parallel (to accelerate time execution) ``pytest-xdist`` module
has to be installed. Then ``-n`` argument with number of spawned processes has
to be added to Pytest call. For example:

::

    pytest tests/bluetooth/bsim_bt -n 4

More information about ``pytest-xdist`` can be found here:
https://pytest-xdist.readthedocs.io/en/latest/

Plugin debugging
****************

For development of Pytest plugin it may be necessary to debug it. For this
purpose ``pytest.set_trace()`` method can be very useful. It should be added
into source code in breakpoint line and then after running tests by Pytest,
program will stop at this method and Python Debugger (``pdb``) will be enabled.

More information about debugging with ``pdb`` can be found here:
https://docs.python.org/3/library/pdb.html
