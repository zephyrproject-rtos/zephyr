Title: USB Console Hello World

Description:

A simple Hello World example, with console output coming to USB UART. Primarily
intended to show the required config options.

Usage
-----
Plug the board into a host device, for example, a PC running Linux.
The board will be detected as a CDC_ACM serial device. To see the console output
from the zephyr board, use a command similar to "minicom -D /dev/ttyACM0".
You may need to stop modemmanager via "sudo stop modemmanager", if it is
trying to access the device in the background.

--------------------------------------------------------------------------------

Building and Running Project:

$ west build -b reel_board

Sample Output:

Hello World! x86
Hello World! x86
Hello World! x86
Hello World! x86
...
...
