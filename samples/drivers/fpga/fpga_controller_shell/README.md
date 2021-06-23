# Zephyr FPGA controller in shell
This module is an FPGA driver that can easily load a bitstream, reset it, check its status, enable or disable the FPGA.
This sample demonstrates how to use the FPGA controller shell subsystem.
Currently, the sample works with the [QuickLogic QuickFeather board](https://github.com/QuickLogic-Corp/quick-feather-dev-board).

## Requirements
* Zephyr RTOS with shell subsystem enabled
* [QuickLogic QuickFeather board](https://github.com/QuickLogic-Corp/quick-feather-dev-board)

## Building

For the QuickLogic QuickFeather board:
```bash
west build -b quick_feather samples/drivers/fpga/fpga_controller_shell
```
See [QuickFeather programming and debugging](https://docs.zephyrproject.org/latest/boards/arm/quick_feather/doc/index.html#programming-and-debugging) on how to load an image to the board.

## Running
After connecting to the shell console you should see the following output:

```bash
Address of the bitstream (red): 0xADDR
Address of the bitstream (green): 0xADDR
Size of the bitstream (red): 75960
Size of the bitstream (green): 75960

uart:~$
```
This sample is already prepared with bitstreams.
After executing the sample, you can see at what address it is stored and its size in bytes.

The FPGA controller command can now be used (`fpga load <device> <address> <size in bytes>`):
```bash
uart:~$ fpga load FPGA 0x2001a46c 75960
FPGA: loading bitstream
```
The LED should start blinking (color depending on the selected bitstream).
To upload the bitstream again you need to reset the FPGA:

```bash
uart:~$ fpga reset FPGA
FPGA: resetting FPGA
```
You can also use your own bitstream.
To load a bitstream into device memory, use `devmem load` command.
It is important to use the -e option when sending a bistream via `xxd`:
```bash
uart:~$ devmem load -e 0x10000
Loading...
Press ctrl-x + ctrl-q to stop
```
Now, the loader is waiting for data.
You can either type it directly from the console or send it from the host PC (replace `ttyX` with the appropriate one for your shell console):
```bash
xxd -p data > /dev/ttyX
```
(It is important to use plain-style hex dump)
Once the data is transferred, use `ctrl-x + ctrl-q` to quit loader.
It will print the sum of the read bytes and return to the shell:
```bash
Number of bytes read: 75960
uart:~$
```
Now the bitstream can be uploaded again.
```bash
uart:~$ fpga load FPGA 0x10000 75960
FPGA: loading bitstream
```
