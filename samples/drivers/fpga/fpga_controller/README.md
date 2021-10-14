# Zephyr FPGA controller
This module is an FPGA driver that can easily load a bitstream, reset it, check its status, enable or disable the FPGA.
This sample demonstrates how to use the FPGA driver API.
Currently the sample works with [Quicklogic Quickfeather board](https://github.com/QuickLogic-Corp/quick-feather-dev-board).

## Requirements
* Zephyr RTOS
* [Quicklogic Quickfeather board](https://github.com/QuickLogic-Corp/quick-feather-dev-board)

## Building

For the QuickLogic QuickFeather board:
```bash
west build -b quick_feather samples/drivers/fpga/fpga_controller
```

## Running
See [QuickFeather programming and debugging](https://docs.zephyrproject.org/latest/boards/arm/quick_feather/doc/index.html#programming-and-debugging) on how to load an image to the board.

## Sample output
Once the board is programmed, the LED should alternately flash red and green.
