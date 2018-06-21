
Zephyr Sanity Tests
###################

This script scans for the set of unit test applications in the git repository
and attempts to execute them. By default, it tries to build each test
case on boards marked as default in the board definition file.

The default options will build the majority of the tests on a defined set of
boards and will run in an emulated environment (QEMU) if available for the
architecture or configuration being tested.

In normal use, sanitycheck runs a limited set of kernel tests (inside
QEMU).  Because of its limited text execution coverage, sanitycheck
cannot guarantee local changes will succeed in the full build
environment, but it does sufficient testing by building samples and
tests for different boards and different configurations to help keep the
full code tree buildable.

To run the script in the local tree, follow the steps below:

::

        $ source zephyr-env.sh
        $ ./scripts/sanitycheck

If you have a system with a large number of cores, you can build and run
all possible tests using the following options:

::

        $ ./scripts/sanitycheck --all --enable-slow

This will build for all available boards and run all applicable tests in
a simulated (QEMU) environment.

The sanitycheck script accepts the following optional arguments:

  -h, --help            show this help message and exit
  -p PLATFORM, --platform PLATFORM
                        Platform filter for testing. This option may be used
                        multiple times. Testcases will only be built/run on
                        the platforms specified. If this option is not used,
                        then platforms marked as default in the platform
                        metadata file will be chosen to build and test.
  -a ARCH, --arch ARCH  Arch filter for testing. Takes precedence over
                        --platform. If unspecified, test all arches. Multiple
                        invocations are treated as a logical 'or' relationship
  -t TAG, --tag TAG     Specify tags to restrict which tests to run by tag
                        value. Default is to not do any tag filtering.
                        Multiple invocations are treated as a logical 'or'
                        relationship
  -e EXCLUDE_TAG, --exclude-tag EXCLUDE_TAG
                        Specify tags of tests that should not run. Default is
                        to run all tests with all tags.
  -f, --only-failed     Run only those tests that failed the previous sanity
                        check invocation.
  -c CONFIG, --config CONFIG
                        Specify platform configuration values filtering. This
                        can be specified two ways: <config>=<value> or just
                        <config>. The defconfig for all platforms will be
                        checked. For the <config>=<value> case, only match
                        defconfig that have that value defined. For the
                        <config> case, match defconfig that have that value
                        assigned to any value. Prepend a '!' to invert the
                        match.
  -s TEST, --test TEST  Run only the specified test cases. These are named by
                        <path to test project relative to --testcase-
                        root>/<testcase.yaml section name>
  -l, --all             Build/test on all platforms. Any --platform arguments
                        ignored.
  -o TESTCASE_REPORT, --testcase-report TESTCASE_REPORT
                        Output a CSV spreadsheet containing results of the
                        test run
  -d DISCARD_REPORT, --discard-report DISCARD_REPORT
                        Output a CSV spreadsheet showing tests that were
                        skipped and why
  --compare-report COMPARE_REPORT
                        Use this report file for size comparison
  -B SUBSET, --subset SUBSET
                        Only run a subset of the tests, 1/4 for running the
                        first 25%, 3/5 means run the 3rd fifth of the total.
                        This option is useful when running a large number of
                        tests on different hosts to speed up execution time.
  -N, --ninja           Use the Ninja generator with CMake
  -y, --dry-run         Create the filtered list of test cases, but don't
                        actually run them. Useful if you're just interested in
                        --discard-report
  --list-tags           list all tags in selected tests
  --list-tests          list all tests.
  --detailed-report FILENAME
                        Generate a junit report with detailed testcase
                        results.
  -r, --release         Update the benchmark database with the results of this
                        test run. Intended to be run by CI when tagging an
                        official release. This database is used as a basis for
                        comparison when looking for deltas in metrics such as
                        footprint
  -w, --warnings-as-errors
                        Treat warning conditions as errors
  -v, --verbose         Emit debugging information, call multiple times to
                        increase verbosity
  -i, --inline-logs     Upon test failure, print relevant log data to stdout
                        instead of just a path to it
  --log-file FILENAME   log also to file
  -m, --last-metrics    Instead of comparing metrics from the last --release,
                        compare with the results of the previous sanity check
                        invocation
  -u, --no-update       do not update the results of the last run of the
                        sanity checks
  -F FILENAME, --load-tests FILENAME
                        Load list of tests to be run from file.
  -E FILENAME, --save-tests FILENAME
                        Save list of tests to be run to file.
  -b, --build-only      Only build the code, do not execute any of it in QEMU
  -j JOBS, --jobs JOBS  Number of cores to use when building, defaults to
                        number of CPUs * 2
  --device-testing      Test on device directly. Specify the serial device to
                        use with the --device-serial option.
  --device-serial DEVICE_SERIAL
                        Serial device for accessing the board (e.g.,
                        /dev/ttyACM0)
  --show-footprint      Show footprint statistics and deltas since last
                        release.
  -H FOOTPRINT_THRESHOLD, --footprint-threshold FOOTPRINT_THRESHOLD
                        When checking test case footprint sizes, warn the user
                        if the new app size is greater then the specified
                        percentage from the last release. Default is 5. 0 to
                        warn on any increase on app size
  -D, --all-deltas      Show all footprint deltas, positive or negative.
                        Implies --footprint-threshold=0
  -O OUTDIR, --outdir OUTDIR
                        Output directory for logs and binaries. This directory
                        will be deleted unless '--no-clean' is set.
  -n, --no-clean        Do not delete the outdir before building. Will result
                        in faster compilation since builds will be incremental
  -T TESTCASE_ROOT, --testcase-root TESTCASE_ROOT
                        Base directory to recursively search for test cases.
                        All testcase.yaml files under here will be processed.
                        May be called multiple times. Defaults to the
                        'samples' and 'tests' directories in the Zephyr tree.
  -A BOARD_ROOT, --board-root BOARD_ROOT
                        Directory to search for board configuration files. All
                        .yaml files in the directory will be processed.
  -z SIZE, --size SIZE  Don't run sanity checks. Instead, produce a report to
                        stdout detailing RAM/ROM sizes on the specified
                        filenames. All other command line arguments ignored.
  -S, --enable-slow     Execute time-consuming test cases that have been
                        marked as 'slow' in testcase.yaml. Normally these are
                        only built.
  -R, --enable-asserts  Build all test cases with assertions enabled. (This is
                        the default)
  --disable-asserts     Build all test cases with assertions disabled.
  -Q, --error-on-deprecations
                        Error on deprecation warnings.
  -x EXTRA_ARGS, --extra-args EXTRA_ARGS
                        Extra CMake cache entries to define when building test
                        cases. May be called multiple times. The key-value
                        entries will be prefixed with -D before being passed
                        to CMake. E.g "sanitycheck -x=USE_CCACHE=0" will
                        translate to "cmake -DUSE_CCACHE=0" which will
                        ultimately disable ccache.
  --enable-coverage     Enable code coverage when building unit tests and when
                        targeting the native_posix board
  -C, --coverage        Generate coverage report for unit tests, and tests and
                        samples run in native_posix. Implies --enable-coverage.


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

        identifier: quark_d2000_crb
        name: Quark D2000 Devboard
        type: mcu
        arch: x86
        toolchain:
          - zephyr
          - issm
        ram: 8
        flash: 32
        testing:
          default: true
          ignore_tags:
            - net
            - bluetooth


identifier:
  A string that matches how the board is defined in the build system. This same
  string is used when building, for example when calling 'cmake'::

  # cmake -DBOARD=quark_d2000_crb ..

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
            platform_whitelist: qemu_cortex_m3 qemu_x86 arduino_101
            tags: bluetooth
          test_br:
            build_only: true
            extra_args: CONF_FILE="prj_br.conf"
            filter: not CONFIG_DEBUG
            platform_exclude: quark_d2000_crb
            platform_whitelist: qemu_cortex_m3 qemu_x86
            tags: bluetooth


A sample with tests will have the same structure with additional information
related to the sample and what is being demonstrated:

::

        sample:
          name: hello world
          description: Hello World sample, the simplest Zephyr application
          platforms: all
        tests:
          test:
            build_only: true
            tags: samples tests
            min_ram: 16
          singlethread:
            build_only: true
            extra_args: CONF_FILE=prj_single.conf
            filter: not CONFIG_BT and not CONFIG_GPIO_SCH
            tags: samples tests
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
          test_resolution_6:
            extra_configs:
              - CONFIG_ADC_QMSI_SAMPLE_WIDTH=6
            platform_whitelist: quark_se_c1000_ss_devboard
            tags: hwtest


build_only: <True|False> (default False)
    If true, don't try to run the test under QEMU even if the
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
    matches. For example, if CONFIG_SOC="quark_se" then

        filter = CONFIG_SOC : "quark.*"

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

To use this feature, run sanitycheck with the following new options::

	scripts/sanitycheck --device-testing --device-serial /dev/ttyACM0 -p \
	frdm_k64f  -T tests/kernel

The ``--device-serial`` option denotes the serial device the board is connected to.
This needs to be accessible by the user running sanitycheck. You can run this on
only one board at a time, specified using the
``--platform`` option.

To produce test reports, use the ``--detailed-report FILENAME`` option which will
generate an XML file using the JUNIT syntax. This file can be used to generate
other reports, for example using ``junit2html`` which can be installed via PIP.
