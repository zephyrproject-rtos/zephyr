.. _coverage:

Generating coverage reports
###########################

With Zephyr, you can generate code coverage reports to analyze which parts of
the code are covered by a given test or application.

You can do this in two ways:

* In a real embedded target or QEMU, using Zephyr's gcov integration
* Directly in your host computer, by compiling your application targeting
  the POSIX architecture

Test coverage reports in embedded devices or QEMU
*************************************************

Overview
========
`GCC GCOV <gcov_>`_ is a test coverage program
used together with the GCC compiler to analyze and create test coverage reports
for your programs, helping you create more efficient, faster running code and
discovering untested code paths

In Zephyr, gcov collects coverage profiling data in RAM (and not to a file
system) while your application is running. Support for gcov collection and
reporting is limited by available RAM size and so is currently enabled only
for QEMU emulation of embedded targets.

Details
=======
There are 2 parts to enable this feature. The first is to enable the coverage for the
device and the second to enable in the test application. As explained earlier the
code coverage with gcov is a function of RAM available. Therefore ensure that the
device has enough RAM when enabling the coverage for it. For example a small device
like frdm_k64f can run a simple test application but the more complex test
cases which consume more RAM will crash when coverage is enabled.

To enable the device for coverage, select :kconfig:option:`CONFIG_HAS_COVERAGE_SUPPORT`
in the Kconfig.board file.

To report the coverage for the particular test application set :kconfig:option:`CONFIG_COVERAGE`.

Steps to generate code coverage reports
=======================================

These steps will produce an HTML coverage report for a single application.

1. Build the code with CONFIG_COVERAGE=y.

   .. zephyr-app-commands::
      :board: mps2_an385
      :gen-args: -DCONFIG_COVERAGE=y -DCONFIG_COVERAGE_DUMP=y
      :goals: build
      :compact:

#. Capture the emulator output into a log file. You may need to terminate
   the emulator with :kbd:`Ctrl-A X` for this to complete after the coverage dump
   has been printed:

   .. code-block:: console

      ninja -Cbuild run | tee log.log

   or

   .. code-block:: console

      ninja -Cbuild run | tee log.log

#. Generate the gcov ``.gcda`` and ``.gcno`` files from the log file that was
   saved::

     $ python3 scripts/gen_gcov_files.py -i log.log

#. Find the gcov binary placed in the SDK. You will need to pass the path to
   the gcov binary for the appropriate architecture when you later invoke
   ``gcovr``::

     $ find $ZEPHYR_SDK_INSTALL_DIR -iregex ".*gcov"

#. Create an output directory for the reports::

     $ mkdir -p gcov_report

#. Run ``gcovr`` to get the reports::

     $ gcovr -r $ZEPHYR_BASE . --html -o gcov_report/coverage.html --html-details --gcov-executable <gcov_path_in_SDK>

.. _coverage_posix:

Coverage reports using the POSIX architecture
*********************************************

When compiling for the POSIX architecture, you utilize your host native tooling
to build a native executable which contains your application, the Zephyr OS,
and some basic HW emulation.

That means you can use the same tools you would while developing any
other desktop application.

To build your application with ``gcc``'s `gcov`_, simply set
:kconfig:option:`CONFIG_COVERAGE` before compiling it.
When you run your application, ``gcov`` coverage data will be dumped into the
respective ``gcda`` and ``gcno`` files.
You may postprocess these with your preferred tools. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :gen-args: -DCONFIG_COVERAGE=y
   :host-os: unix
   :board: native_posix
   :goals: build
   :compact:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe
   # Press Ctrl+C to exit
   lcov --capture --directory ./ --output-file lcov.info -q --rc lcov_branch_coverage=1
   genhtml lcov.info --output-directory lcov_html -q --ignore-errors source --branch-coverage --highlight --legend

.. note::

   You need a recent version of lcov (at least 1.14) with support for
   intermediate text format. Such packages exist in recent Linux distributions.

   Alternatively, you can use gcovr (at least version 4.2).

Coverage reports using Twister
******************************

Zephyr's :ref:`twister script <twister_script>` can automatically
generate a coverage report from the tests which were executed.
You just need to invoke it with the ``--coverage`` command line option.

For example, you may invoke::

    $ twister --coverage -p qemu_x86 -T tests/kernel

or::

    $ twister --coverage -p native_posix -T tests/bluetooth

which will produce ``twister-out/coverage/index.html`` with the report.

The process differs for unit tests, which are built with the host
toolchain and require a different board::

    $ twister --coverage -p unit_testing -T tests/unit

which produces a report in the same location as non-unit testing.

.. _gcov:
   https://gcc.gnu.org/onlinedocs/gcc/Gcov.html

Using different toolchains
==========================

Twister looks at the environment variable ``ZEPHYR_TOOLCHAIN_VARIANT``
to check which gcov tool to use by default. The following are used as the
default for the Twister ``--gcov-tool`` argument default:

+-----------+-----------------------+
| Toolchain | ``--gcov-tool`` value |
+-----------+-----------------------+
| host      | ``gcov``              |
+-----------+-----------------------+
| llvm      | ``llvm-cov gcov``     |
+-----------+-----------------------+
| zephyr    | ``gcov``              |
+-----------+-----------------------+
