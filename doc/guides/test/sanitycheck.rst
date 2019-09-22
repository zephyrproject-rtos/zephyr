
.. _sanitycheck_script:

Sanity Tests
#############

This script scans for the set of unit test applications in the git repository
and attempts to execute them. By default, it tries to build each test
case on boards marked as default in the board definition file.

The default options will build the majority of the tests on a defined set of
boards and will run in an emulated environment if available for the
architecture or configuration being tested.

In normal use, sanitycheck runs a limited set of kernel tests (inside
an emulator).  Because of its limited text execution coverage, sanitycheck
cannot guarantee local changes will succeed in the full build
environment, but it does sufficient testing by building samples and
tests for different boards and different configurations to help keep the
complete code tree buildable.

When using (at least) one ``-v`` option, sanitycheck's console output
shows for every test how the test is run (qemu, native_posix, etc.) or
whether the binary was just built.  There are a few reasons why sanitycheck
only builds a test and doesn't run it:

- The test is marked as ``build_only: true`` in its ``.yaml``
  configuration file.
- The test configuration has defined a ``harness`` but you don't have
  it or haven't set it up.
- The target device is not connected and not available for flashing
- You or some higher level automation invoked sanitycheck with
  ``--build-only``.

These also affect the outputs of ``--testcase-report`` and
``--detailed-report``, see their respective ``--help`` sections.

To run the script in the local tree, follow the steps below:

::

        $ source zephyr-env.sh
        $ ./scripts/sanitycheck

If you have a system with a large number of cores, you can build and run
all possible tests using the following options:

::

        $ ./scripts/sanitycheck --all --enable-slow

This will build for all available boards and run all applicable tests in
a simulated (for example QEMU) environment.

The list of command line options supported by sanitycheck can be viewed using::

        $ ./scripts/sanitycheck --help



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
    - xtools
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
    - hwinfo
    - can
    - pwm
  testing:
    default: true


identifier:
  A string that matches how the board is defined in the build system. This same
  string is used when building, for example when calling ``west build`` or
  ``cmake``::

     # with west
     west build -b tinytile
     # with cmake
     cmake -DBOARD=tinytile ..

name:
  The actual name of the board as it appears in marketing material.
type:
  Type of the board or configuration, currently we support 2 types: mcu, qemu
arch:
  Architecture of the board
toolchain:
  The list of supported toolchains that can build this board. This should match
  one of the values used for 'ZEPHYR_TOOLCHAIN_VARIANT' when building on the command line
ram:
  Available RAM on the board (specified in KB). This is used to match testcase
  requirements.  If not specified we default to 128KB.
flash:
  Available FLASH on the board (specified in KB). This is used to match testcase
  requirements.  If not specified we default to 512KB.
supported:
  A list of features this board supports. This can be specified as a single word
  feature or as a variant of a feature class. For example:

  ::

        supported:
          - pci

  This indicates the board does support PCI. You can make a testcase build or
  run only on such boards, or:

  ::

        supported:
          - netif:eth
          - sensor:bmi16

  A testcase can both depend on 'eth' to only test ethernet or on 'netif' to run
  on any board with a networking interface.

testing:
  testing relating keywords to provide best coverage for the features of this
  board.

  default: [True|False]:
    This is a default board, it will tested with the highest priority and is
    covered when invoking the simplified sanitycheck without any additional
    arguments.
  ignore_tags:
    Do not attempt to build (and therefore run) tests marked with this list of
    tags.

Test Cases
**********

Test cases are detected by the presence of a 'testcase.yaml' or a 'sample.yaml'
files in the application's project directory. This file may contain one or more
entries in the test section each identifying a test scenario. The name of
the test case only needs to be unique for the test cases specified in
that testcase meta-data.

Test cases are written using the YAML syntax and share the same structure as
samples. The following is an example test with a few options that are
explained in this document.


::

        tests:
          test:
            build_only: true
            platform_whitelist: qemu_cortex_m3 qemu_x86
            tags: bluetooth
          test_br:
            build_only: true
            extra_args: CONF_FILE="prj_br.conf"
            filter: not CONFIG_DEBUG
            platform_exclude: up_squared
            platform_whitelist: qemu_cortex_m3 qemu_x86
            tags: bluetooth


A sample with tests will have the same structure with additional information
related to the sample and what is being demonstrated:

::

        sample:
          name: hello world
          description: Hello World sample, the simplest Zephyr application
        tests:
          test:
            build_only: true
            tags: tests
            min_ram: 16
          singlethread:
            build_only: true
            extra_args: CONF_FILE=prj_single.conf
            filter: not CONFIG_BT
            tags: tests
            min_ram: 16

The full canonical name for each test case is:

::

        <path to test case>/<test entry>

Each test block in the testcase meta data can define the following key/value
pairs:

tags: <list of tags> (required)
    A set of string tags for the testcase. Usually pertains to
    functional domains but can be anything. Command line invocations
    of this script can filter the set of tests to run based on tag.

skip: <True|False> (default False)
    skip testcase unconditionally. This can be used for broken tests.

slow: <True|False> (default False)
    Don't run this test case unless --enable-slow was passed in on the
    command line. Intended for time-consuming test cases that are only
    run under certain circumstances, like daily builds. These test cases
    are still compiled.

extra_args: <list of extra arguments>
    Extra arguments to pass to Make when building or running the
    test case.

extra_configs: <list of extra configurations>
    Extra configuration options to be merged with a master prj.conf
    when building or running the test case. For example::

        common:
          tags: drivers adc
        tests:
          test:
            depends_on: adc
          test_async:
            extra_configs:
              - CONFIG_ADC_ASYNC=y


build_only: <True|False> (default False)
    If true, don't try to run the test even if the
    selected platform supports it.

build_on_all: <True|False> (default False)
    If true, attempt to build test on all available platforms.

depends_on: <list of features>
    A board or platform can announce what features it supports, this option
    will enable the test only those platforms that provide this feature.

min_ram: <integer>
    minimum amount of RAM needed for this test to build and run. This is
    compared with information provided by the board metadata.

min_flash: <integer>
    minimum amount of ROM needed for this test to build and run. This is
    compared with information provided by the board metadata.

timeout: <number of seconds>
    Length of time to run test in QEMU before automatically killing it.
    Default to 60 seconds.

arch_whitelist: <list of arches, such as x86, arm, arc>
    Set of architectures that this test case should only be run for.

arch_exclude: <list of arches, such as x86, arm, arc>
    Set of architectures that this test case should not run on.

platform_whitelist: <list of platforms>
    Set of platforms that this test case should only be run for.

platform_exclude: <list of platforms>
    Set of platforms that this test case should not run on.

extra_sections: <list of extra binary sections>
    When computing sizes, sanitycheck will report errors if it finds
    extra, unexpected sections in the Zephyr binary unless they are named
    here. They will not be included in the size calculation.

harness: <string>
    A harness string needed to run the tests successfully. This can be as simple as
    a loopback wiring or a complete hardware test setup for sensor and IO testing.
    Usually pertains to external dependency domains but can be anything such as
    console, sensor, net, keyboard, or Bluetooth.

harness_config: <harness configuration options>
    Extra harness configuration options to be used to select a board and/or
    for handling generic Console with regex matching. Config can announce
    what features it supports. This option will enable the test to run on
    only those platforms that fulfill this external dependency.

    The following options are currently supported:

    type: <one_line|multi_line> (required)
        Depends on the regex string to be matched


    record: <recording options>

      regex: <expression> (required)
        Any string that the particular test case prints to record test
        results.

    regex: <expression> (required)
        Any string that the particular test case prints to confirm test
        runs as expected.

    ordered: <True|False> (default False)
        Check the regular expression strings in orderly or randomly fashion

    repeat: <integer>
        Number of times to validate the repeated regex expression

    fixture: <expression>
        Specify a test case dependency on an external device(e.g., sensor),
        and identify setups that fulfill this dependency. It depends on
        specific test setup and board selection logic to pick the particular
        board(s) out of multiple boards that fulfill the dependency in an
        automation setup based on "fixture" keyword. Some sample fixture names
        are fixture_i2c_hts221, fixture_i2c_bme280, fixture_i2c_FRAM,
        fixture_ble_fw and fixture_gpio_loop.

    The following is an example yaml file with a few harness_config options.

    ::

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
             fixture: fixture_i2c_hts221
         tests:
           test:
             tags: sensors
             depends_on: i2c

filter: <expression>
    Filter whether the testcase should be run by evaluating an expression
    against an environment containing the following values:

    ::

            { ARCH : <architecture>,
              PLATFORM : <platform>,
              <all CONFIG_* key/value pairs in the test's generated defconfig>,
              *<env>: any environment variable available
            }

    The grammar for the expression language is as follows:

    expression ::= expression "and" expression
                 | expression "or" expression
                 | "not" expression
                 | "(" expression ")"
                 | symbol "==" constant
                 | symbol "!=" constant
                 | symbol "<" number
                 | symbol ">" number
                 | symbol ">=" number
                 | symbol "<=" number
                 | symbol "in" list
                 | symbol ":" string
                 | symbol

    list ::= "[" list_contents "]"

    list_contents ::= constant
                    | list_contents "," constant

    constant ::= number
               | string


    For the case where expression ::= symbol, it evaluates to true
    if the symbol is defined to a non-empty string.

    Operator precedence, starting from lowest to highest:

        or (left associative)
        and (left associative)
        not (right associative)
        all comparison operators (non-associative)

    arch_whitelist, arch_exclude, platform_whitelist, platform_exclude
    are all syntactic sugar for these expressions. For instance

        arch_exclude = x86 arc

    Is the same as:

        filter = not ARCH in ["x86", "arc"]

    The ':' operator compiles the string argument as a regular expression,
    and then returns a true value only if the symbol's value in the environment
    matches. For example, if CONFIG_SOC="stm32f107xc" then

        filter = CONFIG_SOC : "stm.*"

    Would match it.

The set of test cases that actually run depends on directives in the testcase
filed and options passed in on the command line. If there is any confusion,
running with -v or --discard-report can help show why particular test cases
were skipped.

Metrics (such as pass/fail state and binary size) for the last code
release are stored in scripts/sanity_chk/sanity_last_release.csv.
To update this, pass the --all --release options.

To load arguments from a file, write '+' before the file name, e.g.,
+file_name. File content must be one or more valid arguments separated by
line break instead of white spaces.

Most everyday users will run with no arguments.


Running Tests on Hardware
*************************

Beside being able to run tests in QEMU and other simulated environments,
sanitycheck supports running most of the tests on real devices and produces
reports for each run with detailed FAIL/PASS results.


Executing tests on a single device
===================================

To use this feature on a single connected device, run sanitycheck with
the following new options::

	scripts/sanitycheck --device-testing --device-serial /dev/ttyACM0 -p \
	frdm_k64f  -T tests/kernel

The ``--device-serial`` option denotes the serial device the board is connected to.
This needs to be accessible by the user running sanitycheck. You can run this on
only one board at a time, specified using the ``--platform`` option.


Executing tests on multiple devices
===================================

To build and execute tests on multiple devices connected to the host PC, a
hardware map needs to be created with all connected devices and their
details such as the serial device and their IDs if available. Run the following
command to produce the hardware map::

    ./scripts/sanitycheck --generate-hardware-map map.yml

The generated hardware map file (map.yml) will have the list of connected
devices, for example::

  - available: true
    id: OSHW000032254e4500128002ab98002784d1000097969900
    platform: unknown
    product: DAPLink CMSIS-DAP
    runner: pyocd
    serial: /dev/cu.usbmodem146114202
  - available: true
    id: 000683759358
    platform: unknown
    product: J-Link
    runner: unknown
    serial: /dev/cu.usbmodem0006837593581


Any options marked as 'unknown' need to be changed and set with the correct
values, in the above example both the platform names and the runners need to be
replaced with the correct values corresponding to the connected hardware. In
this example we are using a reel_board and an nrf52840_pca10056::

  - available: true
    id: OSHW000032254e4500128002ab98002784d1000097969900
    platform: reel_board
    product: DAPLink CMSIS-DAP
    runner: pyocd
    serial: /dev/cu.usbmodem146114202
  - available: true
    id: 000683759358
    platform: nrf52840_pca10056
    product: J-Link
    runner: nrfjprog
    serial: /dev/cu.usbmodem0006837593581

If the map file already exists, then new entries are added and existing entries
will be updated. This way you can use one single master hardware map and update
it for every run to get the correct serial devices and status of the devices.

With the hardware map ready, you can run any tests by pointing to the map
file::

  ./scripts/sanitycheck --device-testing --hardware-map map.yml -T samples/hello_world/

The above command will result in sanitycheck building tests for the platforms
defined in the hardware map and subsequently flashing and running the tests
on those platforms.

.. note::

  Currently only boards with support for both pyocd and nrfjprog are supported
  with the hardware map features. Boards that require other runners to flash the
  Zephyr binary are still work in progress.

To produce test reports, use the ``--detailed-report FILENAME`` option which will
generate an XML file using the JUNIT syntax. This file can be used to generate
other reports, for example using ``junit2html`` which can be installed via PIP.
