.. zephyr:code-sample:: display_console
   :name: Terminal on a display
   :relevant-api: uart_interface log_api shell_api

   Show the console, log messages and a shell on a display.

Overview
********

This sample uses a display as the serial console. An emulated UART is the
console device; the display console shows the data it transmits as text on the
display selected by the ``zephyr,display`` chosen node, and feeds it with the
keys reported by any keyboard driver of the input subsystem.

:file:`app.overlay` adds the emulated UART and selects it as ``zephyr,console``
and ``zephyr,shell-uart``, so the boot banner, ``printk()``, the log messages
and the shell appear on the display with no code of their own. The application
prints one line and leaves the shell to the keyboard. Log warnings and errors
are shown in colour where the display allows it, and long lines wrap.

The rows are drawn by the renderer selected in Kconfig. By default the
character framebuffer fonts are used, in the pixel format the display is
configured for, so monochrome, greyscale and colour displays are all
supported. With :file:`lvgl.conf` the rows are drawn by LVGL in its default
font instead, which must be monospaced; the rows appear once LVGL is
initialized.

Log messages reach the display through the shell, so the log mode is set to
deferred: with immediate logging, everything logged before the shell starts,
including the boot banner, is dropped.

Building and Running
********************

On :zephyr:board:`native_sim` the SDL display shows the result. The board
overlay adds an emulated UART bridge in the forward direction from ``uart0``,
a pseudo terminal whose path is printed at startup, to the emulated UART, so
what is typed in a terminal attached to it drives the shell on the display
while the output stays on the display. The native log backend is disabled so
that the log messages reach the display.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/terminal
   :board: native_sim
   :goals: build run
   :compact:

With the rows drawn by LVGL:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/terminal
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=lvgl.conf
   :goals: build run
   :compact:

On :zephyr:board:`uedx32480035e_wb_a` the board overlay bridges the USB serial
port instead, so the board is a display console as soon as it is flashed:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/terminal
   :board: uedx32480035e_wb_a/esp32s3/procpu
   :goals: build flash
   :compact:

On other boards, enable the input driver of the keyboard, whose key events the
console turns into terminal input, or bridge a serial port as the board
overlays do. Set the terminal size of the shell to the text grid of
the display, which is the display size divided by the font size:

.. code-block:: cfg

   CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH=32
   CONFIG_SHELL_DEFAULT_TERMINAL_HEIGHT=15
