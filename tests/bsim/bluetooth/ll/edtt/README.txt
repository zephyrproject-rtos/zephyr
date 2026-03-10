Intro
#####

This is the embedded side of the Bluetooth conformance tests which are part of the
EDTT (Embedded Device Test Tool).

Much more info about the tool can be found in https://github.com/EDTTool/EDTT
and in its `doc/` folder.

In very short, there is 2 applications in this folder:
1.) A controller only build of the Bluetooth LE stack, where the HCI, and a few extra
    interfaces are exposed to the EDTT.
2.) An application which implements the test GATT services specified by BT SIG
    in GATT_Test_Databases.xlsm, with an EDTT interface which allows the EDTT
    tests to switch between theses.

The first application is used for LL and HCI conformance tests of the
controller.
The second application for GATT tests.

These 2 particular applications are meant to be used in a simulated environment,
for regression, either as part of a CI system, or in workstation during
development.
This is due to this particular application only containing an EDTT transport
driver for simulated targets, meant to connect to the `bsim` EDTT transport
driver thru the EDTT bridge.

How to use it
#############

Assuming you have already
`installed BabbleSim <https://babblesim.github.io/fetching.html>`_.

Add to your environment the variable EDTT_PATH pointing to the
EDTT folder. You can do this by adding it to your `~/.bashrc`, `~/.zephyrrc`,
or similar something like:
```
export EDTT_PATH=${ZEPHYR_BASE}/../tools/edtt/
```
(if you add it to your .bashrc you probably won't be able to refer to
ZEPHYR_BASE)

To run these sets of tests you need to compile both of these applications as any
other Zephyr app, targeting the nrf52_bsim.
To compile both in an automated way you can just use the `compile.sh` (see
below).

To run the tests you can either run one of the provided scripts, or run by
hand the BabbleSim 2G4 Phy, EDTT bridge, EDTT and needed simulated devices.
The shortest path is to use the provided scripts.

In short the whole process being:
```
cd ${ZEPHYR_BASE} && source zephyr-env.sh
#Compile all apps:
WORK_DIR=${ZEPHYR_BASE}/bsim_out tests/bsim/bluetooth/ll/compile.sh

#run all tests
RESULTS_FILE=${ZEPHYR_BASE}/banana.xml SEARCH_PATH=tests/bsim/bluetooth/ll/edtt/ tests/bsim/run_parallel.sh

#or just run one set:
tests/bsim/bluetooth/ll/edtt/tests_scripts/hci.sh
```
