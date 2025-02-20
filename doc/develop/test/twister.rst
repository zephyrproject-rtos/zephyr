.. _twister_script:

Test Runner (Twister)
#####################

Twister scans for the set of test applications in the git repository
and attempts to execute them. By default, it tries to build each test
application on boards marked as default in the board definition file.

The default options will build the majority of the test applications on a
defined set of boards and will run in an emulated environment if available for
the architecture or configuration being tested.

Because of the limited test execution coverage, twister
cannot guarantee local changes will succeed in the full build
environment, but it does sufficient testing by building samples and
tests for different boards and different configurations to help keep the
complete code tree buildable.

When using (at least) one ``-v`` option, twister's console output
shows for every test application how the test is run (qemu, native_sim, etc.) or
whether the binary was just built. The resultant
:ref:`status <twister_statuses>`
of a test is likewise reported in the ``twister.json`` and other report files.
There are a few reasons why twister only builds a test and doesn't run it:

- The test is marked as ``build_only: true`` in its ``.yaml``
  configuration file.
- The test configuration has defined a ``harness`` but you don't have
  it or haven't set it up.
- The target device is not connected and not available for flashing
- You or some higher level automation invoked twister with
  ``--build-only``.

To run the script in the local tree, follow the steps below:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         $ source zephyr-env.sh
         $ ./scripts/twister

   .. group-tab:: Windows

      .. code-block:: bat

         zephyr-env.cmd
         python .\scripts\twister


If you have a system with a large number of cores and plenty of free storage space,
you can build and run all possible tests using the following options:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         $ ./scripts/twister --all --enable-slow

   .. group-tab:: Windows

      .. code-block:: bat

         python .\scripts\twister --all --enable-slow

This will build for all available boards and run all applicable tests in
a simulated (for example QEMU) environment.

If you want to run tests on one or more specific platforms, you can use
the ``--platform`` option, it is a platform filter for testing, with this
option, test suites will only be built/run on the platforms specified.
This option also supports different revisions of one same board,
you can use ``--platform board@revision`` to test on a specific revision.

The list of command line options supported by twister can be viewed using:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         $ ./scripts/twister --help

   .. group-tab:: Windows

      .. code-block:: bat

         python .\scripts\twister --help


Board Configuration
*******************

To build tests for a specific board and to execute some of the tests on real
hardware or in an emulation environment such as QEMU a board configuration file
is required which is generic enough to be used for other tasks that require a
board inventory with details about the board and its configuration that is only
available during build time otherwise.

The board metadata file is located in the board directory and is structured
using the YAML markup language. The example below shows a board with a data
required for best test coverage for this specific board:

.. code-block:: yaml

  identifier: frdm_k64f
  name: NXP FRDM-K64F
  type: mcu
  arch: arm
  toolchain:
    - zephyr
    - gnuarmemb
  supported:
    - arduino_gpio
    - arduino_i2c
    - netif:eth
    - adc
    - i2c
    - nvs
    - spi
    - gpio
    - usb_device
    - watchdog
    - can
    - pwm
  testing:
    default: true


identifier:
  A string that matches how the board is defined in the build system. This same
  string is used when building, for example when calling ``west build`` or
  ``cmake``:

  .. code-block:: console

     # with west
     west build -b reel_board
     # with cmake
     cmake -DBOARD=reel_board ..

name:
  The actual name of the board as it appears in marketing material.
type:
  Type of the board or configuration, currently we support 2 types: mcu, qemu
simulation:
  Simulator(s) used to simulate the platform, e.g. qemu.

  .. code-block:: yaml

      simulation:
        - name: qemu
        - name: armfvp
          exec: FVP_Some_Platform
        - name: custom
          exec: AnotherBinary

  By default, tests will be executed using the first entry in the simulation array. Another
  simulation can be selected with ``--simulation <simulation_name>``.
  The ``exec`` attribute is optional. If it is set but the required simulator is not available, the
  tests will be built only.
  If it is not set and the required simulator is not available the tests will fail to run.
  The simulation name must match one of the element of ``SUPPORTED_EMU_PLATFORMS``.
arch:
  Architecture of the board
toolchain:
  The list of supported toolchains that can build this board. This should match
  one of the values used for :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` when building on the command line
ram:
  Available RAM on the board (specified in KB). This is used to match test scenario
  requirements.  If not specified we default to 128KB.
flash:
  Available FLASH on the board (specified in KB). This is used to match test scenario
  requirements.  If not specified we default to 512KB.
supported:
  A list of features this board supports. This can be specified as a single word
  feature or as a variant of a feature class. For example:

  .. code-block:: yaml

        supported:
          - pci

  This indicates the board does support PCI. You can make a test scenario build or
  run only on such boards, or:

  .. code-block:: yaml

        supported:
          - netif:eth
          - sensor:bmi16

  A test scenario can depend on 'eth' to only test ethernet or on 'netif' to run
  on any board with a networking interface.

testing:
  testing relating keywords to provide best coverage for the features of this
  board.

.. _twister_default_testing_board:

  binaries:
    A list of custom binaries to be kept for device testing.
  default: [True|False]:
    This is a default board, it will tested with the highest priority and is
    covered when invoking the simplified twister without any additional
    arguments.
  ignore_tags:
    Do not attempt to build (and therefore run) tests marked with this list of
    tags.
  only_tags:
    Only execute tests with this list of tags on a specific platform.

  .. _twister_board_timeout_multiplier:

  timeout_multiplier: <float> (default 1)
    Multiply each test scenario timeout by specified ratio. This option allows to tune timeouts only
    for required platform. It can be useful in case naturally slow platform I.e.: HW board with
    power-efficient but slow CPU or simulation platform which can perform instruction accurate
    simulation but does it slowly.

env:
  A list of environment variables. Twister will check if all these environment variables are set,
  and otherwise skip this platform. This allows the user to define a platform which should be
  used, for example, only if some required software or hardware is present, and to signal that
  presence to twister using these environment variables.

.. _twister_tests_long_version:

Tests
******

Tests are detected by the presence of a ``testcase.yaml`` or a ``sample.yaml``
files in the application's project directory. This test application
configuration file may contain one or more entries in the ``tests:`` section each
identifying a Test Scenario.

.. _twister_test_project_diagram:

.. figure:: figures/twister_test_project.svg
   :alt: Twister and a Test application project.
   :figclass: align-center

   Twister and a Test application project.


Test application configurations are written using the YAML syntax and share the
same structure as samples.

A Test Scenario is a set of conditions and variables defined in a Test Scenario
entry, under which a set of Test Suites will be built and executed.

A Test Suite is a collection of Test Cases which are intended to be used to test
a software program to ensure it meets certain requirements. The Test Cases in a
Test Suite are either related or meant to be executed together.

Test Scenario, Test Suite, and Test Case names must follow to these basic rules:

#. The format of the Test Scenario identifier shall be a string without any spaces or
   special characters (allowed characters: alphanumeric and [\_=]) consisting
   of multiple sections delimited with a dot (``.``).

#. Each Test Scenario identifier shall start with a section name followed by a
   subsection names delimited with a dot (``.``). For example, a test scenario
   that covers semaphores in the kernel shall start with ``kernel.semaphore``.

#. All Test Scenario identifiers within a Test Configuration (``testcase.yaml`` file)
   need to be unique.
   For example a ``testcase.yaml`` file covering semaphores in the kernel can have:

   * ``kernel.semaphore``: For general semaphore tests
   * ``kernel.semaphore.stress``: Stress testing semaphores in the kernel.

#. The full canonical name of a Test Suite is:
   ``<Test Application Project path>/<Test Scenario identifier>``

#. Depending on the Test Suite implementation, its Test Case identifiers consist
   of **at least three sections** delimited with a dot (``.``):

   * **Ztest tests**:
     a Test Scenario identifier from the corresponding ``testcase.yaml`` file,
     a Ztest suite name, and a Ztest test name:
     ``<Test Scenario identifier>.<Ztest suite name>.<Ztest test name>``

   * **Standalone tests and samples**:
     a Test Scenario identifier from the corresponding ``testcase.yaml`` (or
     ``sample.yaml``) file where the last section signifies the standalone
     Test Case name, for example: ``debug.coredump.logging_backend``.


The ``--no-detailed-test-id`` command line option modifies the above rules in this way:

#. A Test Suite name has only ``<Test Scenario identifier>`` component.
   Its Application Project path can be found in ``twister.json`` report as ``path:`` property.

#. With short Test Suite names in this mode, all corresponding Test Scenario names
   must be unique for the Twister execution scope.

#. **Ztest** Test Case names have only Ztest components ``<Ztest suite name>.<Ztest test name>``.
   Its parent Test Suite name equals to the corresponding Test Scenario identifier.


The following is an example test configuration with a few options that are
explained in this document.


  .. code-block:: yaml

        tests:
          bluetooth.gatt:
            build_only: true
            platform_allow: qemu_cortex_m3 qemu_x86
            tags: bluetooth
          bluetooth.gatt.br:
            build_only: true
            extra_args: CONF_FILE="prj_br.conf"
            filter: not CONFIG_DEBUG
            platform_exclude: up_squared
            platform_allow: qemu_cortex_m3 qemu_x86
            tags: bluetooth


A sample with tests will have the same structure with additional information
related to the sample and what is being demonstrated:

  .. code-block:: yaml

        sample:
          name: hello world
          description: Hello World sample, the simplest Zephyr application
        tests:
          sample.basic.hello_world:
            build_only: true
            tags: tests
            min_ram: 16
          sample.basic.hello_world.singlethread:
            build_only: true
            extra_args: CONF_FILE=prj_single.conf
            filter: not CONFIG_BT
            tags: tests
            min_ram: 16

A Test Scenario entry in the ``tests:`` YAML dictionary has its Test Scenario
identifier as a key.

Each Test Scenario entry in the Test Application configuration can define the
following key/value pairs:

..  _test_config_args:

tags: <list of tags> (required)
    A set of string tags for the test scenario. Usually pertains to
    functional domains but can be anything. Command line invocations
    of this script can filter the set of tests to run based on tag.

skip: <True|False> (default False)
    skip test scenario unconditionally. This can be used for broken tests for
    example.

slow: <True|False> (default False)
    Don't run this test scenario unless ``--enable-slow`` or ``--enable-slow-only`` was
    passed in on the command line. Intended for time-consuming test scenarios that
    are only run under certain circumstances, like daily builds. These test
    scenarios are still compiled.

extra_args: <list of extra arguments>
    Extra arguments to pass to build tool when building or running the
    test scenario.

    Using namespacing, it is possible to apply extra_args only to some
    hardware. Currently architectures/platforms/simulation are supported:

    .. code-block:: yaml

        common:
          tags: drivers adc
        tests:
          test:
            depends_on: adc
          test_async:
            extra_args:
              - arch:x86:CONFIG_ADC_ASYNC=y
              - platform:qemu_x86:CONFIG_DEBUG=y
              - platform:mimxrt1060_evk:SHIELD=rk043fn66hs_ctg
              - simulation:qemu:CONFIG_MPU=y

extra_configs: <list of extra configurations>
    Extra configuration options to be merged with a main prj.conf
    when building or running the test scenario. For example:

    .. code-block:: yaml

        common:
          tags: drivers adc
        tests:
          test:
            depends_on: adc
          test_async:
            extra_configs:
              - CONFIG_ADC_ASYNC=y

    Using namespacing, it is possible to apply a configuration only to some
    hardware. Currently both architectures and platforms are supported:

    .. code-block:: yaml

        common:
          tags: drivers adc
        tests:
          test:
            depends_on: adc
          test_async:
            extra_configs:
              - arch:x86:CONFIG_ADC_ASYNC=y
              - platform:qemu_x86:CONFIG_DEBUG=y


build_only: <True|False> (default False)
    If true, twister will not try to run the test even if the test is runnable
    on the platform.

    This keyword is reserved for tests that are used to test if some code
    actually builds. A ``build_only`` test is not designed to be run in any
    environment and should not be testing any functionality, it only verifies
    that the code builds.

    This option is often used to test drivers and the fact that they are correctly
    enabled in Zephyr and that the code builds, for example sensor drivers. Such
    test shall not be used to verify the functionality of the driver.

build_on_all: <True|False> (default False)
    If true, attempt to build test scenario on all available platforms. This is mostly
    used in CI for increased coverage. Do not use this flag in new tests.

depends_on: <list of features>
    A board or platform can announce what features it supports, this option
    will enable the test only those platforms that provide this feature.

levels: <list of levels>
    Test levels this test should be part of. If a level is present, this
    test will be selectable using the command line option ``--level <level name>``

min_ram: <integer>
    estimated minimum amount of RAM in KB needed for this test to build and run. This is
    compared with information provided by the board metadata.

min_flash: <integer>
    estimated minimum amount of ROM in KB needed for this test to build and run. This is
    compared with information provided by the board metadata.

.. _twister_test_case_timeout:

timeout: <number of seconds>
    Length of time to run test before automatically killing it.
    Default to 60 seconds.

arch_allow: <list of arches, such as x86, arm, arc>
    Set of architectures that this test scenario should only be run for.

arch_exclude: <list of arches, such as x86, arm, arc>
    Set of architectures that this test scenario should not run on.

vendor_allow: <list of vendors>
    Set of platform vendors that this test scenario should only be run for.  The
    vendor is defined as part of the board definition. Boards associated with
    this vendors will be included. Other boards, including those without a
    vendor will be excluded.

vendor_exclude: <list of vendors>
    Set of platform vendors that this test scenario should not run on.
    The vendor is defined as part of the board. Boards associated with this
    vendors will be excluded.

platform_allow: <list of platforms>
    Set of platforms that this test scenario should only be run for. Do not use
    this option to limit testing or building in CI due to time or resource
    constraints, this option should only be used if the test or sample can
    only be run on the allowed platform and nothing else.

integration_platforms: <YML list of platforms/boards>
    This option limits the scope to the listed platforms when twister is
    invoked with the ``--integration`` option. Use this instead of
    platform_allow if the goal is to limit scope due to timing or
    resource constraints.

integration_toolchains: <YML list of toolchain variants>
    This option expands the scope to all the listed toolchains variants and
    adds another vector of testing where desired. By default, test
    configurations are generated based on the toolchain configured in the environment:

    test scenario -> platforms1 -> toolchain1
    test scenario -> platforms2 -> toolchain1


    When a platform supports multiple toolchains that are available during the
    twister run, it is possible to expand the test configurations to include
    additional tests for each toolchain. For example, if a platform supports
    toolchains ``toolchain1`` and ``toolchain2``, and the test scenario
    includes:

    .. code-block:: yaml

      integration_toolchains:
        - toolchain1
        - toolchain2

    the following configurations are generated:

    test scenario -> platforms1 -> toolchain1
    test scenario -> platforms1 -> toolchain2
    test scenario -> platforms2 -> toolchain1
    test scenario -> platforms2 -> toolchain2


    .. note::

      This functionality is evaluated always and is not limited to the
      ``--integration`` option.

platform_exclude: <list of platforms>
    Set of platforms that this test scenario should not run on.

extra_sections: <list of extra binary sections>
    When computing sizes, twister will report errors if it finds
    extra, unexpected sections in the Zephyr binary unless they are named
    here. They will not be included in the size calculation.

sysbuild: <True|False> (default False)
    Build the project using sysbuild infrastructure. Only the main project's
    generated devicetree and Kconfig will be used for filtering tests.
    on device testing must use the hardware map, or west flash to load
    the images onto the target. The ``--erase`` option of west flash is
    not supported with this option. Usage of unsupported options will result
    in tests requiring sysbuild support being skipped.

harness: <string>
    A harness keyword in the ``testcase.yaml`` file identifies a Twister
    harness needed to run a test successfully. A harness is a feature of
    Twister and implemented by Twister, some harnesses are defined as
    placeholders and have no implementation yet.

    A harness can be seen as the handler that needs to be implemented in
    Twister to be able to evaluate if a test passes criteria. For example, a
    keyboard harness is set on tests that require keyboard interaction to reach
    verdict on whether a test has passed or failed, however, Twister lack this
    harness implementation at the moment.

    Supported harnesses:

    - ztest
    - test
    - console
    - pytest
    - gtest
    - robot
    - ctest
    - shell

    See :ref:`twister_harnesses` for more information.

platform_key: <list of platform attributes>
    Often a test needs to only be built and run once to qualify as passing.
    Imagine a library of code that depends on the platform architecture where
    passing the test on a single platform for each arch is enough to qualify the
    tests and code as passing. The platform_key attribute enables doing just
    that.

    For example to key on (arch, simulation) to ensure a test is run once
    per arch and simulation (as would be most common):

    .. code-block:: yaml

      platform_key:
        - arch
        - simulation

    Adding platform (board) attributes to include things such as soc name,
    soc family, and perhaps sets of IP blocks implementing each peripheral
    interface would enable other interesting uses. For example, this could enable
    building and running SPI tests once for each unique IP block.

harness_config: <harness configuration options>
    Extra harness configuration options to be used to select a board and/or
    for handling generic Console with regex matching. Config can announce
    what features it supports. This option will enable the test to run on
    only those platforms that fulfill this external dependency.


    fixture: <expression>
        Specify a test scenario dependency on an external device(e.g., sensor),
        and identify setups that fulfill this dependency. It depends on
        specific test setup and board selection logic to pick the particular
        board(s) out of multiple boards that fulfill the dependency in an
        automation setup based on ``fixture`` keyword. Some sample fixture names
        are i2c_hts221, i2c_bme280, i2c_FRAM, ble_fw and gpio_loop.

        Only one fixture can be defined per test scenario and the fixture name has to
        be unique across all tests in the test suite.


    The following is an example yaml file with robot harness_config options.

    .. code-block:: yaml

        tests:
          robot.example:
            harness: robot
            harness_config:
              robot_testsuite: [robot file path]

    It can be more than one test suite using a list.

    .. code-block:: yaml

        tests:
          robot.example:
            harness: robot
            harness_config:
              robot_testsuite:
                - [robot file path 1]
                - [robot file path 2]
                - [robot file path n]

    One or more options can be passed to robotframework.

    .. code-block:: yaml

        tests:
          robot.example:
            harness: robot
            harness_config:
              robot_testsuite: [robot file path]
              robot_option:
                - --exclude tag
                - --stop-on-error

filter: <expression>
    Filter whether the test scenario should be run by evaluating an expression
    against an environment containing the following values:

    .. code-block:: none

            { ARCH : <architecture>,
              PLATFORM : <platform>,
              <all CONFIG_* key/value pairs in the test's generated defconfig>,
              *<env>: any environment variable available
            }

    Twister will first evaluate the expression to find if a "limited" cmake call, i.e. using package_helper cmake script,
    can be done. Existence of "dt_*" entries indicates devicetree is needed.
    Existence of "CONFIG*" entries indicates kconfig is needed.
    If there are no other types of entries in the expression a filtration can be done without creating a complete build system.
    If there are entries of other types a full cmake is required.

    The grammar for the expression language is as follows:

    .. code-block:: antlr

        expression : expression 'and' expression
                   | expression 'or' expression
                   | 'not' expression
                   | '(' expression ')'
                   | symbol '==' constant
                   | symbol '!=' constant
                   | symbol '<' NUMBER
                   | symbol '>' NUMBER
                   | symbol '>=' NUMBER
                   | symbol '<=' NUMBER
                   | symbol 'in' list
                   | symbol ':' STRING
                   | symbol
                   ;

        list : '[' list_contents ']';

        list_contents : constant (',' constant)*;

        constant : NUMBER | STRING;

    For the case where ``expression ::= symbol``, it evaluates to ``true``
    if the symbol is defined to a non-empty string.

    Operator precedence, starting from lowest to highest:

       * or (left associative)
       * and (left associative)
       * not (right associative)
       * all comparison operators (non-associative)

    ``arch_allow``, ``arch_exclude``, ``platform_allow``, ``platform_exclude``
    are all syntactic sugar for these expressions. For instance:

    .. code-block:: none

        arch_exclude = x86 arc

    Is the same as:

    .. code-block:: none

        filter = not ARCH in ["x86", "arc"]

    The ``:`` operator compiles the string argument as a regular expression,
    and then returns a true value only if the symbol's value in the environment
    matches. For example, if ``CONFIG_SOC="stm32f107xc"`` then

    .. code-block:: none

        filter = CONFIG_SOC : "stm.*"

    Would match it.

required_snippets: <list of needed snippets>
    :ref:`Snippets <snippets>` are supported in twister for test scenarios that
    require them. As with normal applications, twister supports using the base
    zephyr snippet directory and test application directory for finding
    snippets. Listed snippets will filter supported tests for boards (snippets
    must be compatible with a board for the test to run on them, they are not
    optional).

    The following is an example yaml file with 2 required snippets.

    .. code-block:: yaml

        tests:
          snippet.example:
            required_snippets:
              - cdc-acm-console
              - user-snippet-example

The set of test scenarios that actually run depends on directives in the test scenario
filed and options passed in on the command line. If there is any confusion,
running with ``-v`` or examining the discard report
(:file:`twister_discard.csv`) can help show why particular test scenarios were
skipped.

Metrics (such as pass/fail state and binary size) for the last code
release are stored in ``scripts/release/twister_last_release.csv``.
To update this, pass the ``--all --release`` options.

To load arguments from a file, add ``+`` before the file name, e.g.,
``+file_name``. File content must be one or more valid arguments separated by
line break instead of white spaces.

Most everyday users will run with no arguments.

.. _twister_harnesses:

Harnesses
*********

Harnesses ``ztest``, ``gtest`` and ``console`` are based on parsing of the
output and matching certain phrases. ``ztest`` and ``gtest`` harnesses look
for pass/fail/etc. frames defined in those frameworks.

Some widely used harnesses that are not supported yet:

- keyboard
- net
- bluetooth

The following is an example yaml file with a few harness_config options.

.. code-block:: yaml

      sample:
        name: HTS221 Temperature and Humidity Monitor
      common:
        tags: sensor
        harness: console
        harness_config:
          type: multi_line
          ordered: false
          regex:
            - "Temperature:(.*)C"
            - "Relative Humidity:(.*)%"
          fixture: i2c_hts221
      tests:
        test:
          tags: sensors
          depends_on: i2c

Ctest
=====

ctest_args: <list of arguments> (default empty)
    Specify a list of additional arguments to pass to ``ctest`` e.g.:
    ``ctest_args: [‘--repeat until-pass:5’]``. Note that
    ``--ctest-args`` can be passed multiple times to pass several arguments
    to the ctest.


Gtest
=====

Use ``gtest`` harness if you've already got tests written in the gTest
framework and do not wish to update them to zTest.

Pytest
======

The :ref:`pytest harness <integration_with_pytest>` is used to execute pytest
test suites in the Zephyr test. The following options apply to the pytest harness:

.. _pytest_root:

pytest_root: <list of pytest testpaths> (default pytest)
    Specify a list of pytest directories, files or subtests that need to be
    executed when a test scenario begins to run. The default pytest directory is
    ``pytest``. After the pytest run is finished, Twister will check if
    the test scenario passed or failed according to the pytest report.
    As an example, a list of valid pytest roots is presented below:

    .. code-block:: yaml

        harness_config:
          pytest_root:
            - "pytest/test_shell_help.py"
            - "../shell/pytest/test_shell.py"
            - "/tmp/test_shell.py"
            - "~/tmp/test_shell.py"
            - "$ZEPHYR_BASE/samples/subsys/testsuite/pytest/shell/pytest/test_shell.py"
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

Console
=======

The ``console`` harness tells Twister to parse a test's text output for a
regex defined in the test's YAML file.

The following options are currently supported:

type: <one_line|multi_line> (required)
    Depends on the regex string to be matched

regex: <list of regular expressions> (required)
    Strings with regular expressions to match with the test's output
    to confirm the test runs as expected.

ordered: <True|False> (default False)
    Check the regular expression strings in orderly or randomly fashion

record: <recording options> (optional)
  regex: <list of regular expressions> (required)
    Regular expressions with named subgroups to match data fields found
    in the test instance's output lines where it provides some custom data
    for further analysis. These records will be written into the build
    directory ``recording.csv`` file as well as ``recording`` property
    of the test suite object in ``twister.json``.

    With several regular expressions given, each of them will be applied
    to each output line producing either several different records from
    the same output line, or different records from different lines,
    or similar records from different lines.

    The .CSV file will have as many columns as there are fields detected
    in all records; missing values are filled by empty strings.

    For example, to extract three data fields ``metric``, ``cycles``,
    ``nanoseconds``:

    .. code-block:: yaml

      record:
        regex:
          - "(?P<metric>.*):(?P<cycles>.*) cycles, (?P<nanoseconds>.*) ns"

  merge: <True|False> (default False)
    Allows to keep only one record in a test instance with all the data
    fields extracted by the regular expressions. Fields with the same name
    will be put into lists ordered as their appearance in recordings.
    It is possible for such multi value fields to have different number
    of values depending on the regex rules and the test's output.

  as_json: <list of regex subgroup names> (optional)
    Data fields, extracted by the regular expressions into named subgroups,
    which will be additionally parsed as JSON encoded strings and written
    into ``twister.json`` as nested ``recording`` object properties.
    The corresponding ``recording.csv`` columns will contain JSON strings
    as-is.

    Using this option, a test log can convey layered data structures
    passed from the test image for further analysis with summary results,
    traces, statistics, etc.

    For example, this configuration:

    .. code-block:: yaml

      record:
        regex: "RECORD:(?P<type>.*):DATA:(?P<metrics>.*)"
        as_json: [metrics]

    when matched to a test log string:

    .. code-block:: none

      RECORD:jitter_drift:DATA:{"rollovers":0, "mean_us":1000.0}

    will be reported in ``twister.json`` as:

    .. code-block:: json

      "recording":[
          {
                "type":"jitter_drift",
                "metrics":{
                    "rollovers":0,
                    "mean_us":1000.0
                }
          }
      ]

Robot
=====

The ``robot`` harness is used to execute Robot Framework test suites
in the Renode simulation framework.

robot_testsuite: <robot file path> (default empty)
    Specify one or more paths to a file containing a Robot Framework test suite to be run.

robot_option: <robot option> (default empty)
    One or more options to be send to robotframework.

Bsim
====

Harness ``bsim`` is implemented in limited way - it helps only to copy the
final executable (``zephyr.exe``) from build directory to BabbleSim's
``bin`` directory (``${BSIM_OUT_PATH}/bin``).

This action is useful to allow BabbleSim's tests to directly run after.
By default, the executable file name is (with dots and slashes
replaced by underscores): ``bs_<platform_name>_<test_path>_<test_scenario_name>``.
This name can be overridden with the ``bsim_exe_name`` option in
``harness_config`` section.

bsim_exe_name: <string>
    If provided, the executable filename when copying to BabbleSim's bin
    directory, will be ``bs_<platform_name>_<bsim_exe_name>`` instead of the
    default based on the test path and scenario name.

Shell
=====

The shell harness is used to execute shell commands and parse the output and utilizes the pytest
framework and the pytest harness of twister.

The following options apply to the shell harness:

shell_commands: <list of pairs of commands and their expected output> (default empty)
    Specify a list of shell commands to be executed and their expected output.
    For example:

    .. code-block:: yaml

        harness_config:
          shell_commands:
          - command: "kernel cycles"
            expected: "cycles: .* hw cycles"
          - command: "kernel version"
            expected: "Zephyr version .*"
          - command: "kernel sleep 100"


    If expected output is not provided, the command will be executed and the output
    will be logged.

shell_commands_file: <string> (default empty)
    Specify a file containing test parameters to be used in the test.
    The file should contain a list of commands and their expected output. For example:

    .. code-block:: yaml

      - command: "mpu mtest 1"
        expected: "The value is: 0x.*"
      - command: "mpu mtest 2"
        expected: "The value is: 0x.*"


    If no file is specified, the shell harness will use the default file
    ``test_shell.yml`` in the test directory.
    ``shell_commands`` will take precedence over ``shell_commands_file``.

Selecting platform scope
************************

One of the key features of Twister is its ability to decide on which platforms a given
test scenario should run. This behavior has its roots in Twister being developed as
a test runner for Zephyr's CI. With hundreds of available platforms and thousands of
tests, the testing tools should be able to adapt the scope and select/filter out what
is relevant and what is not.

Twister always prepares an initial list of platforms in scope for a given test,
based on command line arguments and the :ref:`test's configuration <test_config_args>`. Then,
platforms that don't fulfill the conditions required in the configuration yaml
(e.g. minimum ram) are filtered out from the scope.
Using ``--force-platform`` allows to override filtering caused by ``platform_allow``,
``platform_exclude``, ``arch_allow`` and ``arch_exclude`` keys in test configuration
files.

Command line arguments define the initial scope in the following way:

* ``-p/--platform <platform_name>`` (can be used multiple times): only platforms
  passed with this argument;
* ``-l/--all``: all available platforms;
* ``-G/--integration``: all platforms from an ``integration_platforms`` list in
  a given test configuration file. If a test has no ``integration_platforms``
  *"scope presumption"* will happen;
* No scope argument: *"scope presumption"* will happen.

*"Scope presumption"*: A list of Twister's :ref:`default platforms <twister_default_testing_board>`
is used as the initial list. If nothing is left after the filtration, the ``platform_allow`` list
is used as the initial scope.

Managing tests timeouts
***********************

There are several parameters which control tests timeouts on various levels:

* ``timeout`` option in each test scenario. See :ref:`here <twister_test_case_timeout>` for more
  details.
* ``timeout_multiplier`` option in board configuration. See
  :ref:`here <twister_board_timeout_multiplier>` for more details.
* ``--timeout-multiplier`` twister option which can be used to adjust timeouts in exact twister run.
  It can be useful in case of simulation platform as simulation time may depend on the host
  speed & load or we may select different simulation method (i.e. cycle accurate but slower
  one), etc...

Overall test scenario timeout is a multiplication of these three parameters.

Running in Integration Mode
***************************

This mode is used in continuous integration (CI) and other automated
environments used to give developers fast feedback on changes. The mode can
be activated using the ``--integration`` option of twister and narrows down
the scope of builds and tests if applicable to platforms defined under the
integration keyword in the test configuration file (``testcase.yaml`` and
``sample.yaml``).


Running tests on custom emulator
********************************

Apart from the already supported QEMU and other simulated environments, Twister
supports running any out-of-tree custom emulator defined in the board's :file:`board.cmake`.
To use this type of simulation, add the following properties to
:file:`custom_board/custom_board.yaml`:

.. code-block:: yaml

   simulation:
     - name: custom
       exec: <name_of_emu_binary>

This tells Twister that the board is using a custom emulator called ``<name_of_emu_binary>``,
make sure this binary exists in the PATH.

Then, in :file:`custom_board/board.cmake`, set the supported emulation platforms to ``custom``:

.. code-block:: cmake

   set(SUPPORTED_EMU_PLATFORMS custom)

Finally, implement the ``run_custom`` target in :file:`custom_board/board.cmake`.
It should look something like this:

.. code-block:: cmake

   add_custom_target(run_custom
     COMMAND
     <name_of_emu_binary to invoke during 'run'>
     <any args to be passed to the command, i.e. ${BOARD}, ${APPLICATION_BINARY_DIR}/zephyr/zephyr.elf>
     WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
     DEPENDS ${logical_target_for_zephyr_elf}
     USES_TERMINAL
     )

Running Tests on Hardware
*************************

Beside being able to run tests in QEMU and other simulated environments,
twister supports running most of the tests on real devices and produces
reports for each run with detailed FAIL/PASS results.


Executing tests on a single device
===================================

To use this feature on a single connected device, run twister with
the following new options:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

	      scripts/twister --device-testing --device-serial /dev/ttyACM0 \
	      --device-serial-baud 115200 -p frdm_k64f  -T tests/kernel

   .. group-tab:: Windows

      .. code-block:: bat

	      python .\scripts\twister --device-testing --device-serial COM1 \
	      --device-serial-baud 115200 -p frdm_k64f  -T tests/kernel

The ``--device-serial`` option denotes the serial device the board is connected to.
This needs to be accessible by the user running twister. You can run this on
only one board at a time, specified using the ``--platform`` option.

The ``--device-serial-baud`` option is only needed if your device does not run at
115200 baud.

To support devices without a physical serial port, use the ``--device-serial-pty``
option. In this cases, log messages are captured for example using a script.
In this case you can run twister with the following options:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         scripts/twister --device-testing --device-serial-pty "script.py" \
         -p intel_adsp/cavs25 -T tests/kernel

   .. group-tab:: Windows

      .. note::

         Not supported on Windows OS

The script is user-defined and handles delivering the messages which can be
used by twister to determine the test execution status.

The ``--device-flash-timeout`` option allows to set explicit timeout on the
device flash operation, for example when device flashing takes significantly
large time.

The ``--device-flash-with-test`` option indicates that on the platform
the flash operation also executes a test scenario, so the flash timeout is
increased by a test scenario timeout.

Executing tests on multiple devices
===================================

To build and execute tests on multiple devices connected to the host PC, a
hardware map needs to be created with all connected devices and their
details such as the serial device, baud and their IDs if available.
Run the following command to produce the hardware map:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         ./scripts/twister --generate-hardware-map map.yml

   .. group-tab:: Windows

      .. code-block:: bat

         python .\scripts\twister --generate-hardware-map map.yml

The generated hardware map file (map.yml) will have the list of connected
devices, for example:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: yaml

         - connected: true
           id: OSHW000032254e4500128002ab98002784d1000097969900
           platform: unknown
           product: DAPLink CMSIS-DAP
           runner: pyocd
           serial: /dev/cu.usbmodem146114202
         - connected: true
           id: 000683759358
           platform: unknown
           product: J-Link
           runner: unknown
           serial: /dev/cu.usbmodem0006837593581

   .. group-tab:: Windows

      .. code-block:: yaml

         - connected: true
           id: OSHW000032254e4500128002ab98002784d1000097969900
           platform: unknown
           product: unknown
           runner: unknown
           serial: COM1
         - connected: true
           id: 000683759358
           platform: unknown
           product: unknown
           runner: unknown
           serial: COM2


Any options marked as ``unknown`` need to be changed and set with the correct
values, in the above example the platform names, the products and the runners need
to be replaced with the correct values corresponding to the connected hardware.
In this example we are using a reel_board and an nrf52840dk/nrf52840:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: yaml

         - connected: true
           id: OSHW000032254e4500128002ab98002784d1000097969900
           platform: reel_board
           product: DAPLink CMSIS-DAP
           runner: pyocd
           serial: /dev/cu.usbmodem146114202
           baud: 9600
         - connected: true
           id: 000683759358
           platform: nrf52840dk/nrf52840
           product: J-Link
           runner: nrfjprog
           serial: /dev/cu.usbmodem0006837593581
           baud: 9600

   .. group-tab:: Windows

      .. code-block:: yaml

         - connected: true
           id: OSHW000032254e4500128002ab98002784d1000097969900
           platform: reel_board
           product: DAPLink CMSIS-DAP
           runner: pyocd
           serial: COM1
           baud: 9600
         - connected: true
           id: 000683759358
           platform: nrf52840dk/nrf52840
           product: J-Link
           runner: nrfjprog
           serial: COM2
           baud: 9600

The baud entry is only needed if not running at 115200.

If the map file already exists, then new entries are added and existing entries
will be updated. This way you can use one single master hardware map and update
it for every run to get the correct serial devices and status of the devices.

With the hardware map ready, you can run any tests by pointing to the map

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         ./scripts/twister --device-testing --hardware-map map.yml -T samples/hello_world/

   .. group-tab:: Windows

      .. code-block:: bat

         python .\scripts\twister --device-testing --hardware-map map.yml -T samples\hello_world

The above command will result in twister building tests for the platforms
defined in the hardware map and subsequently flashing and running the tests
on those platforms.

.. note::

  Currently only boards with support for pyocd, nrfjprog, jlink, openocd, or dediprog
  are supported with the hardware map features. Boards that require other runners to flash the
  Zephyr binary are still work in progress.

Hardware map allows to set ``--device-flash-timeout`` and ``--device-flash-with-test``
command line options as ``flash-timeout`` and ``flash-with-test`` fields respectively.
These hardware map values override command line options for the particular platform.

Serial PTY support using ``--device-serial-pty``  can also be used in the
hardware map:

.. code-block:: yaml

   - connected: true
     id: None
     platform: intel_adsp/cavs25
     product: None
     runner: intel_adsp
     serial_pty: path/to/script.py
     runner_params:
       - --remote-host=remote_host_ip_addr
       - --key=/path/to/key.pem


The runner_params field indicates the parameters you want to pass to the
west runner. For some boards the west runner needs some extra parameters to
work. It is equivalent to following west and twister commands.

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         west flash --remote-host remote_host_ip_addr --key /path/to/key.pem

         twister -p intel_adsp/cavs25 --device-testing --device-serial-pty script.py
         --west-flash="--remote-host=remote_host_ip_addr,--key=/path/to/key.pem"

   .. group-tab:: Windows

      .. note::

         Not supported on Windows OS

.. note::

  For serial PTY, the "--generate-hardware-map" option cannot scan it out
  and generate a correct hardware map automatically. You have to edit it
  manually according to above example. This is because the serial port
  of the PTY is not fixed and being allocated in the system at runtime.

Fixtures
+++++++++

Some tests require additional setup or special wiring specific to the test.
Running the tests without this setup or test fixture may fail. A test scenario can
specify the fixture it needs which can then be matched with hardware capability
of a board and the fixtures it supports via the command line or using the hardware
map file.

Fixtures are defined in the hardware map file as a list:

.. code-block:: yaml

      - connected: true
        fixtures:
          - gpio_loopback
        id: 0240000026334e450015400f5e0e000b4eb1000097969900
        platform: frdm_k64f
        product: DAPLink CMSIS-DAP
        runner: pyocd
        serial: /dev/ttyACM9

When running ``twister`` with ``--device-testing``, the configured fixture
in the hardware map file will be matched to test scenarios requesting the same fixtures
and these tests will be executed on the boards that provide this fixture.

.. figure:: figures/fixtures.svg
   :figclass: align-center

Fixtures can also be provided via twister command option ``--fixture``, this option
can be used multiple times and all given fixtures will be appended as a list. And the
given fixtures will be assigned to all boards, this means that all boards set by
current twister command can run those test scenarios which request the same fixtures.

Some fixtures allow for configuration strings to be appended, separated from the
fixture name by a ``:``. Only the fixture name is matched against the fixtures
requested by test scenarios.

Notes
+++++

It may be useful to annotate board descriptions in the hardware map file
with additional information.  Use the ``notes`` keyword to do this.  For
example:

.. code-block:: yaml

    - connected: false
      fixtures:
        - gpio_loopback
      id: 000683290670
      notes: An nrf5340dk/nrf5340 is detected as an nrf52840dk/nrf52840 with no serial
        port, and three serial ports with an unknown platform.  The board id of the serial
        ports is not the same as the board id of the development kit.  If you regenerate
        this file you will need to update serial to reference the third port, and platform
        to nrf5340dk/nrf5340/cpuapp or another supported board target.
      platform: nrf52840dk/nrf52840
      product: J-Link
      runner: jlink
      serial: null

Overriding Board Identifier
+++++++++++++++++++++++++++

When (re-)generated the hardware map file will contain an ``id`` keyword
that serves as the argument to ``--board-id`` when flashing.  In some
cases the detected ID is not the correct one to use, for example when
using an external J-Link probe.  The ``probe_id`` keyword overrides the
``id`` keyword for this purpose.   For example:

.. code-block:: yaml

    - connected: false
      id: 0229000005d9ebc600000000000000000000000097969905
      platform: mimxrt1060_evk
      probe_id: 000609301751
      product: DAPLink CMSIS-DAP
      runner: jlink
      serial: null

Using Single Board For Multiple Variants
++++++++++++++++++++++++++++++++++++++++

  The ``platform`` attribute can be a list of names or a string
  with names separated by spaces. This allows to run tests for
  different platform variants on the same physical board, without
  re-configuring the hardware map file for each variant. For example:

.. code-block:: yaml

    - connected: true
      id: '001234567890'
      platform:
      - nrf5340dk/nrf5340/cpuapp
      - nrf5340dk/nrf5340/cpuapp/ns
      product: J-Link
      runner: nrfjprog
      serial: /dev/ttyACM1

Quarantine
++++++++++

Twister allows user to provide configuration files defining a list of tests or
platforms to be put under quarantine. Such tests will be skipped and marked
accordingly in the output reports. This feature is especially useful when
running larger test suits, where a failure of one test can affect the execution
of other tests (e.g. putting the physical board in a corrupted state).

To use the quarantine feature one has to add the argument
``--quarantine-list <PATH_TO_QUARANTINE_YAML>`` to a twister call.
Multiple quarantine files can be used.
The current status of tests on the quarantine list can also be verified by adding
``--quarantine-verify`` to the above argument. This will make twister skip all tests
which are not on the given list.

A quarantine yaml has to be a sequence of dictionaries. Each dictionary has to have
``scenarios`` and ``platforms`` entries listing combinations of scenarios and platforms
to put under quarantine. In addition, an optional entry ``comment`` can be used, where
some more details can be given (e.g. link to a reported issue). These comments will also
be added to the output reports.

When quarantining a class of tests or many scenarios in a single testsuite or
when dealing with multiple issues within a subsystem, it is possible to use
regular expressions, for example, **kernel.*** would quarantine
all kernel tests.

An example of entries in a quarantine yaml:

.. code-block:: yaml

    - scenarios:
        - sample.basic.helloworld
      comment: "Link to the issue: https://github.com/zephyrproject-rtos/zephyr/pull/33287"

    - scenarios:
        - kernel.common
        - kernel.common.(misra|tls)
        - kernel.common.nano64
      platforms:
        - .*_cortex_.*
        - native_sim

To exclude a platform, use the following syntax:

.. code-block:: yaml

    - platforms:
      - qemu_x86
      comment: "broken qemu"

Additionally you can quarantine entire architectures or a specific simulator for executing tests.

Test Configuration
******************

A test configuration can be used to customize various aspects of twister
and the default enabled options and features. This allows tweaking the filtering
capabilities depending on the environment and makes it possible to adapt and
improve coverage when targeting different sets of platforms.

The test configuration also adds support for test levels and the ability to
assign a specific test to one or more levels. Using command line options of
twister it is then possible to select a level and just execute the tests
included in this level.

Additionally, the test configuration allows defining level
dependencies and additional inclusion of tests into a specific level if
the test itself does not have this information already.

In the configuration file you can include complete components using
regular expressions and you can specify which test level to import from
the same file, making management of levels easier.

To help with testing outside of upstream CI infrastructure, additional
options are available in the configuration file, which can be hosted
locally. As of now, those options are available:

- Ability to ignore default platforms as defined in board definitions
  (Those are mostly emulation platforms used to run tests in upstream
  CI)
- Option to specify your own list of default platforms overriding what
  upstream defines.
- Ability to override ``build_on_all`` options used in some test scenarios.
  This will treat tests or sample as any other just build for default
  platforms you specify in the configuration file or on the command line.
- Ignore some logic in twister to expand platform coverage in cases where
  default platforms are not in scope.


Platform Configuration
======================

The following options control platform filtering in twister:

- ``override_default_platforms``: override default key a platform sets in board
  configuration and instead use the list of platforms provided in the
  configuration file as the list of default platforms. This option is set to
  False by default.
- ``increased_platform_scope``: This option is set to True by default, when
  disabled, twister will not increase platform coverage automatically and will
  only build and run tests on the specified platforms.
- ``default_platforms``: A list of additional default platforms to add. This list
  can either be used to replace the existing default platforms or can extend it
  depending on the value of ``override_default_platforms``.

And example platforms configuration:

.. code-block:: yaml

	platforms:
	  override_default_platforms: true
	  increased_platform_scope: false
	  default_platforms:
	    - qemu_x86


Test Level Configuration
========================

The test configuration allows defining test levels, level dependencies and
additional inclusion of tests into a specific test level if the test itself
does not have this information already.

In the configuration file you can include complete components using
regular expressions and you can specify which test level to import from
the same file, making management of levels simple.

And example test level configuration:

.. code-block:: yaml

	levels:
	  - name: my-test-level
	    description: >
	      my custom test level
	    adds:
	      - kernel.threads.*
	      - kernel.timer.behavior
	      - arch.interrupt
	      - boards.*


Combined configuration
======================

To mix the Platform and level configuration, you can take an example as below:

An example platforms plus level configuration:

.. code-block:: yaml

	platforms:
	  override_default_platforms: true
	  default_platforms:
	    - frdm_k64f
	levels:
	  - name: smoke
	    description: >
	        A plan to be used verifying basic zephyr features.
	  - name: unit
	    description: >
	        A plan to be used verifying unit test.
	  - name: integration
	    description: >
	        A plan to be used verifying integration.
	  - name: acceptance
	    description: >
	        A plan to be used verifying acceptance.
	  - name: system
	    description: >
	        A plan to be used verifying system.
	  - name: regression
	    description: >
	        A plan to be used verifying regression.


To run with above test_config.yaml file, only default_platforms with given test level
test scenarios will run.

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         scripts/twister --test-config=<path to>/test_config.yaml
          -T tests --level="smoke"



Running in Tests in Random Order
********************************
Enable ZTEST framework's :kconfig:option:`CONFIG_ZTEST_SHUFFLE` config option to
run your tests in random order.  This can be beneficial for identifying
dependencies between test cases. For native_sim platforms, you can provide
the seed to the random number generator by providing ``-seed=value`` as an
argument to twister. See :ref:`Shuffling Test Sequence <ztest_shuffle>` for more
details.

Robot Framework Tests
*********************
Zephyr supports `Robot Framework <https://robotframework.org/>`_ as one of solutions for automated testing.

Robot files allow you to express interactive test scenarios in human-readable text format and execute them in simulation or against hardware.
At this moment Zephyr integration supports running Robot tests in the `Renode <https://renode.io/>`_ simulation framework.

To execute a Robot test suite with twister, run the following command:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: bash

         $ ./scripts/twister --platform hifive1 --test samples/subsys/shell/shell_module/sample.shell.shell_module.robot

   .. group-tab:: Windows

      .. code-block:: bat

         python .\scripts\twister --platform hifive1 --test samples/subsys/shell/shell_module/sample.shell.shell_module.robot

Writing Robot tests
===================

For the list of keywords provided by the Robot Framework itself, refer to `the official Robot documentation <https://robotframework.org/robotframework/>`_.

Information on writing and running Robot Framework tests in Renode can be found in `the testing section <https://renode.readthedocs.io/en/latest/introduction/testing.html>`_ of Renode documentation.
It provides a list of the most commonly used keywords together with links to the source code where those are defined.

It's possible to extend the framework by adding new keywords expressed directly in Robot test suite files, as an external Python library or, like Renode does it, dynamically via XML-RPC.
For details see the `extending Robot Framework <https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html#extending-robot-framework>`_ section in the official Robot documentation.

Running a single testsuite
==========================

To run a single testsuite instead of a whole group of test you can run:

.. code-block:: bash

   $ twister -p qemu_riscv32 -s tests/kernel/interrupt/arch.shared_interrupt
