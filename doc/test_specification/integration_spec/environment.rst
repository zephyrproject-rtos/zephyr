.. _integration_spec_environment:

Environment
############

Description of intergration testing environment, including software, hardware environment.

We use a script named sanitycheck to run intergration testcases.
This script scans for the set of unit test applications in the git repository
and attempts to execute them. By default, it tries to build each test
case on boards marked as default in the board definition file.

The default options will build the majority of the tests on a defined set of
boards and will run in an emulated environment if available for the
architecture or configuration being tested.

In normal use, sanitycheck runs a limited set of kernel tests (inside
an emulator).  Because of its limited test execution coverage, sanitycheck
cannot guarantee local changes will succeed in the full build
environment, but it does sufficient testing by building samples and
tests for different boards and different configurations to help keep the
complete code tree buildable.

When using (at least) one ``-v`` option, sanitycheck's console output
shows for every test how the test is run (qemu, native_posix, etc.) or
whether the binary was just built.

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
