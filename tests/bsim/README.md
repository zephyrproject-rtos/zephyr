This folder contains tests meant to be run with BabbleSim's physical layer
simulation, and therefore cannot be run directly from twister.

The compile.sh and run_parallel.sh scripts are used by the CI system to build
the needed images and execute these tests in batch.

You can also run them manually if desired, but be sure to call them setting
the variables they expect. For example, from Zephyr's root folder, you can run:

```
WORK_DIR=${ZEPHYR_BASE}/bsim_out tests/bsim/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml SEARCH_PATH=tests/bsim tests/bsim/run_parallel.sh
```

Or to run only a specific subset, e.g. host advertising tests:

```
WORK_DIR=${ZEPHYR_BASE}/bsim_out tests/bsim/bluetooth/host/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml \
SEARCH_PATH=tests/bsim/bluetooth/host/adv tests/bsim/run_parallel.sh
```

Check the run_parallel.sh help for more options and examples on how to use this script to run these
tests in batch.

After building the tests' required binaries you can run a test directly with its individual
test scripts.

For example you can build the required binaries for the networking tests with

```
WORK_DIR=${ZEPHYR_BASE}/bsim_out tests/bsim/net/compile.sh
```
and then directly run one of the tests:

```
tests/bsim/net/sockets/echo_test/tests_scripts/echo_test_802154.sh
```


Conventions about these tests scripts
-------------------------------------

Each test is defined by a shell script with the extension `.sh`.
Scripts starting with an underscore (`_`) are ignored.

Each of these test scripts expects the binaries their require already built, and will execute
the required simulated devices and physical layer simulation with the required command line
options.

Tests must return 0 to the invoking shell if the test passes, and not 0 if the test
fails.

It is recommended to have a single test for each test script.

Each test must have a unique simulation id, to enable running different tests in parallel.

These scripts should not compile the images on their own.

Neither the scripts nor the images should modify the workstation filesystem content beyond the
`${BSIM_OUT_PATH}/results/<simulation_id>/` or `/tmp/` folders.
That is, they should not leave straneous files behind.

If these scripts or the test binaries create temporary files, they should preferably do so by
placing them in the `${BSIM_OUT_PATH}/results/<simulation_id>/` folder.
Otherwise they should be named as to avoid conflicts with other test scripts which may be running
in parallel.

When running tests that require several consecutive simulations, for ex. if simulating a device
pairing, powering off, and powering up after as a new simulation,
they should strive for using separate simulation ids for each simulation part,
in that way ensuring that the simulation radio activity of each segment can be inspected a
posteriori.


Conventions about the build scripts
-----------------------------------

The build scripts (compile.sh) simply build all the required test and sample applications
for the tests' scripts placed in the subfolders below.

This build scripts use the common compile.source which provide a function (compile) which calls
cmake and ninja with the provided application, configuration and overlay files.

To speed up compilation for users interested only in a subset of tests, several compile scripts
exist in several subfolders, where the upper ones call into the lower ones.

Note that cmake and ninja are used directly instead of the `west build` wrapper as west is not
required, and some Zephyr users do not use or have west, but still use this build and tests scripts.
