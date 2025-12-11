# Zephyr devmem load command
This module add a `devmem load` command that allows data to be loaded into device memory.
The `devmem load` command is supported by every transport the shell can run on.

After using a command in the Zephyr shell, the device reads all transferred data and writes to an address in the memory.
The transfer ends when the user presses `ctrl-x + ctrl-q`.

## Usage
Note: when using the devmem load command over UART it is recommended to use interrupts whenever possible.
If this is not possible, reduce the baud rate to 9600.

If you use poll you should also use `prj_poll.conf` instead of `prj.conf`.
## Building

The sample can be built for several platforms, the following commands build and run the application with a shell for the FRDM-K64F board.
```bash
west build -b frdm_k64f samples/subsys/shell/devmem_load
west flash
```

Building for boards without UART interrupt support:
```bash
west build -b native_sim -- -DCONF_FILE=prj_poll.conf  samples/subsys/shell/devmem_load
```
## Running
After connecting to the UART console you should see the following output:
```bash
uart:~$
```
The `devmem load` command can now be used (`devmem load [option] [address]`):
```bash
uart:~$ devmem load 0x20020000
Loading...
press ctrl-x ctrl-q to escape
```

Now, the `devmem load` is waiting for data.
You can either type it directly from the console or send it from the host PC (replace `ttyX` with the appropriate one for your UART console):
```bash
xxd -p data > /dev/ttyX
```
(It is important to use plain-style hex dump)
Once the data is transferred, use `ctrl-x + ctrl-q` to quit the loader.
It will print the number of the bytes read and return to the shell:
```bash
Number of bytes read: 3442
uart:~$
```

## Options
Currently, the `devmem load` command supports the following argument:
* `-e` little endian parse e.g. `0xDEADBEFF -> 0xFFBEADDE`
