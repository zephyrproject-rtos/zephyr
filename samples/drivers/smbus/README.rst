.. zephyr:code-sample:: smbus-shell
   :name: SMBus shell
   :relevant-api: smbus_interface

   Interact with SMBus peripherals using shell commands.

Overview
********

This is a simple SMBus shell sample that allows arbitrary boards with SMBus
driver supported exploring the SMBus communication with peripheral devices.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/drivers/smbus` in the
Zephyr tree.
The sample can be built and run as follows for the ``qemu_x86_64`` board:

.. zephyr-app-commands::
   :zephyr-app: zephyr/samples/drivers/smbus
   :host-os: unix
   :board: qemu_x86_64
   :goals: run
   :compact:

Sample Output
*************

Output from console when application started:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-804-gfedd72615e82  ***
   Start SMBUS shell sample qemu_x86_64
   uart:~$

List available SMBus shell commands with:

.. code-block:: console

   uart:~$ smbus
   smbus - smbus commands
   Subcommands:
     quick            :SMBus Quick command
                       Usage: quick <device> <addr>
     scan             :Scan SMBus peripheral devices command
                       Usage: scan <device>
     byte_read        :SMBus: byte read command
                       Usage: byte_read <device> <addr>
     byte_write       :SMBus: byte write command
                       Usage: byte_write <device> <addr> <value>
     byte_data_read   :SMBus: byte data read command
                       Usage: byte_data_read <device> <addr> <cmd>
     byte_data_write  :SMBus: byte data write command
                       Usage: byte_data_write <device> <addr> <cmd> <value>
     word_data_read   :SMBus: word data read command
                       Usage: word_data_read <device> <addr> <cmd>
     word_data_write  :SMBus: word data write command
                       Usage: word_data_write <device> <addr> <cmd> <value>
     block_write      :SMBus: Block Write command
                       Usage: block_write <device> <addr> <cmd> [<byte1>, ...]
     block_read       :SMBus: Block Read command
                       Usage: block_read <device> <addr> <cmd>

Scan for available SMBus devices with command:

.. code-block:: console

   uart:~$ smbus scan smbus@fb00
        0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
   00:             -- -- -- -- -- -- -- -- -- -- -- --
   10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
   20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
   30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
   40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
   50: 50 51 52 53 54 55 56 57 -- -- -- -- -- -- -- --
   60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
   70: -- -- -- -- -- -- -- --
   8 devices found on smbus@fb00
