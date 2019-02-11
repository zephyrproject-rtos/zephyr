.. _gcov:

Test coverage reports via GCOV
##############################

Overview
********
`GCC GCOV <https://gcc.gnu.org/onlinedocs/gcc/Gcov.html>`_ is a test coverage program
used together with the GCC compiler to analyze and create test coverage reports
for your programs, helping you create more efficient, faster running code and
discovering untested code paths

In Zephyr, gcov collects coverage profiling data in RAM (and not to a file
system) while your application is running. Support for gcov collection and
reporting is limited by available RAM size and so is currently enabled only
for QEMU emulation of embedded targets.

The purpose of this document is to detail the steps involved in the generation of
coverage report for embedded targets. The default gcov behavior will continue to
remain unchanged for targets which use POSIX_ARCH .

Details
*******
There are 2 parts to enable this feature. The first is to enable the coverage for the
device and the second to enable in the test application. As explained earlier the
code coverage with gcov is a function of RAM available. Therefore ensure that the
device has enough RAM when enabling the coverage for it. For example a small device
like frdm_k64f can run a simple test application but the more complex test
cases which consume more RAM will crash when coverage is enabled.

To enable the device for coverage, select :option:`CONFIG_HAS_COVERAGE_SUPPORT`
in the Kconfig.board file.

To report the coverage for the particular test application set :option:`CONFIG_COVERAGE`.

Steps to generate code coverage reports
***************************************

1. Build the code with CONFIG_COVERAGE=y::

     $ cmake -DBOARD=mps2_an385 -DCONFIG_COVERAGE=y ..

#. Store the build and run output on to a log file::

     $ make run > log.log

#. Generate the gcov gcda files from the log file that was saved::

     $ python3 scripts/gen_gcov_files.py -i log.log

#. Find the gcov binary placed in the SDK::

     $ find -iregex ".*gcov"

#. Run gcovr to get the reports::

     $ gcovr -r . --html -o gcov_report/coverage.html --html-details --gcov-executable <gcov_path_in_SDK>

