# nRF IEEE 802.15.4 radio driver

[![Build Status][travis-svg]][travis]

[travis]: https://travis-ci.org/NordicSemiconductor/nRF-IEEE-802.15.4-radio-driver
[travis-svg]: https://travis-ci.org/NordicSemiconductor/nRF-IEEE-802.15.4-radio-driver.svg?branch=master

The nRF IEEE 802.15.4 radio driver implements the IEEE 802.15.4 PHY layer on the Nordic Semiconductor nRF52840 SoC.

The architecture of the nRF IEEE 802.15.4 radio driver is independent of the OS and IEEE 802.15.4 MAC layer.
It allows to use the driver in any IEEE 802.15.4 based stacks that implement protocols such as Thread, ZigBee, or RF4CE.

In addition, it was designed to work with multiprotocol applications. The driver allows to share the RADIO peripheral with other PHY protocol drivers, for example, Bluetooth LE.

For more information and a detailed description of the driver, see the [Wiki](https://github.com/NordicSemiconductor/nRF-IEEE-802.15.4-radio-driver/wiki).

Note that the *nRF-IEEE-802.15.4-radio-driver.packsc* file is a project building description used for internal testing in Nordic Semiconductor. This file is NOT needed to build the driver with any other tool.
