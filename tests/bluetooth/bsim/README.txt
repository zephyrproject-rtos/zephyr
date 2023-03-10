This folder contains tests meant to be run with BabbleSim's physical layer
simulation, and therefore cannot be run directly from twister

The compile.sh and run_parallel.sh scripts are used by the CI system to build
the needed images and execute these tests in batch.

You can also run them manually if desired, but be sure to call them setting
the variables they expect. For example, from Zephyr's root folder, you can run:

WORK_DIR=${ZEPHYR_BASE}/bsim_bt_out tests/bluetooth/bsim/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml SEARCH_PATH=tests/bluetooth/bsim tests/bluetooth/bsim/run_parallel.sh

Or to run only a specific subset, e.g. host advertising tests:
WORK_DIR=${ZEPHYR_BASE}/bsim_bt_out tests/bluetooth/bsim/host/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/myresults.xml SEARCH_PATH=tests/bluetooth/bsim/host/adv tests/bluetooth/bsim/run_parallel.sh
