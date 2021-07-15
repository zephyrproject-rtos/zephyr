# Complex test scenarios and MCUboot tests

# Why:
We wanted to fully automatize MCUboot tests. All those tests require building multiple images and executing mutliple stages (e.g. signing images, flashing them consecutevly and checking the output after each flash) within one test scenario. The current Twister workflow doesn't allow for running this kind of tests. Only a single image can be built and flashed during a single test. I wanted to provide means for testers/developers to easily add more complex tests, whitout the need of going into the Twister's backend.
# What:
This PR proposes an extension to Twister, allowing for building multiple images and having several on-target stages in a single test. It also adds the MCUboot tests which utilitize the proposed solution. The PR can be splited: Twister part added to Zephyr's and the tests themselves  to the MCUboot's repository.
# How:
My PR proposes an easy way for adding more complex test scenarios for Twister. It is based on "keword-driven" approch for test design. A developer/tester should be able to easily design such scenarios within the testcase/sample.yamls, using simple and functional keywords. As a main feature I added "images" and "stages" keywords which tell Twister which images to build and which stages to run. The most new functionality is added in `stageslib.py`. I also had to refactor some parts of `twisterlib.py`, however I tried not to change the original workflow.


To run tests with twister one needs to:
- checkout this PR
- has nrf52840dk connected (e.g. to /dev/ttyACM0)
- run in zephyr's dir:`scripts/twister -T /tests/subsys/mcuboot/ -p nrf52840dk_nrf52840 --device-testing --device-serial /dev/ttyACM0 -v -v --inline-logs`


# MCUboot tests:

Added tests are based on the tests from: [https://github.com/mcu-tools/mcuboot/tree/main/samples/zephyr](https://github.com/mcu-tools/mcuboot/tree/main/samples/zephyr) `run-tests.sh` and `Makefile`. I used a version configured for nrf52840dk https://github.com/nvlsianpu/mcuboot/tree/mod-for-nrf52840-testing/samples/zephyr

All tests have the same workflow:
  -   build (with overlay) bootloader, hello1, hello2,
  -   sign hello1 and hello2 with varying args used,
  -   flash bootloader an read output, flash hello1 and read output, flash hello2 and read output, reset and read output.

The idea is that testcase.yamls should contain platform-independant configurations and regex checks. Platform specific arguments needed for imgtool are stored in `hello-word/ingtool_conf.yaml`

I added args required by imgtool to `hello-word/ingtool_conf.yaml`.  Lines to be checked in `testcase.yaml` are based on the output I've seen when running run-tests.sh with https://github.com/nvlsianpu/mcuboot/tree/mod-for-nrf52840-testing/samples/zephyr It is possible that not all lines are needed and some has to be modified to be platform-independant. In addition, `hello-word/ingtool_conf.yaml`has to be expanded adding args for other platforms.

# TODO:
- twisterlib.py  needs extra refactoring by extracting command-building method for flashing binaries and adding other options (e.g. reseting) to it. I used a temp hack where `pyocd commander -c reset` is called to reset a board instead a regular flashing command. This won't work when more boards are connected. In addition I tested my PR with nrf52840 and nrfjprog which recognises where to flash thanks to the data from Intel hexes. For other boards/runners we might need to expand command building with these information. E.g.
`pyocd flash -a 0x20000 signed-hello1.bin` vs `pyocd flash -a 0x80000 signed-hello2.bin`
