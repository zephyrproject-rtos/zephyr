ZEPHYR TASK PROFILER
####################

This project permits to enable profiling in zephyr application
Profiling trace can be retrived over JTAG or UART
- JTAG: slow but no impact on CPU execution
- UART: much faster but impacting CPU

On QEMU, profiler trace can be retrieved using gdb (following JTAG guidelines)
or on virtual serial port (when enabling UART flush)

Profiler consists in:
1) Enabling KERNEL_EVENT_LOGGER in zephyr
  - Various config switches allows selecting events to capture
2) Extracting profiler events via JTAG (requires debugger) or UART (requires
   profiler background task)
  - for UART: profiler background task must be added to the application
3) Post-processing tools for getting profiler binary data and get ASCII/graphic
   output

---
NOTE: KERNEL_TASK_MONITOR logs have been merged in KERNEL_EVENT_LOGGER ring
buffer since Zephyr 1.1
If using Zephyr version inferior to 1.1, monitor buffer can be dumped using
JTAG and post-processed using profile_monitor.py
The command to dump the buffer is:
(gdb) dump binary value <monitor_file> k_monitor_buff
Then post-processing:
./profile_monitor.py (--qemu) <monitor_file> <zephyr_map_file>

Project organisation
====================

This project includes:

|_ profiler: profiler code (kernel event logger flush) and post-processing
|  |         scripts
|  |_ src: profiler code to integrate into zephyr application to enable
|  |       flushing of kernel events
|  |_ scripts: post-processing scripts
|  |  |_ term: terminal to use to get zephyr console logs and retrieve profiler
|  |           data into a separate file for post-processing (linux only)
|_ microkernel: sample code to exercise profiler in microkernel
|                          mode
|_ nanokernel: sample code to exercise profiler in nanokernel mode

Following guide explains how to enable profiling in your application and
how to use profiling data

Requirements
============

To be installed on host PC:
- python and matplotlib

For JTAG only
- gdb
- For galileo
  - JTAG dongle (e.g. OLIMEX ARM-USB-OCD-H)
  - openocd

How to enable profiler in your application
==========================================

Conventions:
$ZEPHYR_BASE = Zephyr base folder
$APP_BASE = Application base folder
$PROFILER_BASE = Profiler base folder i.e. $ZEPHYR_BASE/samples/task_profiler/profiler
prof.log = profiler output file generated from Zephyr target (binary)

1) Enable KERNEL_EVENT_LOGGER
-----------------------------

  1.1) Kernel event logger configuration
  --------------------------------------

Edit project configuration file (e.g. $APP_BASE/prj.conf) and add the following
flags:

<-- snippet
CONFIG_RING_BUFFER=y
CONFIG_KERNEL_EVENT_LOGGER=y
CONFIG_NANO_TIMEOUTS=y
CONFIG_KERNEL_EVENT_LOGGER_BUFFER_SIZE=<BUFFER_SIZE>
-->

<BUFFER_SIZE> may depend on use case. Note that
- on JTAG, buffer is no more populated when full
- on UART, buffer is flushed at UART speed every 5s (can be modified in
  src/profiler.c)

Events have to be enabled on top of previous flags:
- Task/fiber switches
CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH=y
- Interrupts logging
CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT=y
- Sleep logging
CONFIG_KERNEL_EVENT_LOGGER_SLEEP=y
- Task monitor (microkernel only)
CONFIG_TASK_MONITOR=y
CONFIG_TASK_MONITOR_MASK=<MASK>

<MASK> bits are defined in kernel/microkernel/include/micro_private.h
For information:
- Task swap event: 1
- Task state bit change: 2
- Kserver commands: 4
- Kevents: 8
If wanting to understand microkernel object usage and task bit handling,
recommended value is 6
Otherwise, task monitor can be disabled (to decrease logger ring buffer usage)

  1.2) Kernel event logger timestamp
  ----------------------------------

By default, the kernel event logger is using the system timer for timestamping
events. As system timer may not work properly depending on platform, this timer
can be set at runtime using following flag:

$APP_BASE/prj.conf:
CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP=y

In that case, the profiler will set the RTC as timestamp so RTC must be enabled
$APP_BASE/prj.conf:
CONFIG_RTC=y

In case the RTC cannot be used, profiler can support counters as well by
building application with the following way:

$APP_BASE/prj.conf:
CONFIG_COUNTER=y

Build command:
make BOARD=... PROFILER_USE_COUNTER=y

2) Enable UART flush [OPTIONAL]
-------------------------------

  2.1) Enable profiler UART flush function
  ----------------------------------------

    2.1.1) MICROKERNEL mode
    -----------------------

Add to $APP_BASE/prj.mdef

% TASK NAME  PRIO ENTRY STACK GROUPS
% ==================================
<--snippet
  TASK PROF    10 prof    512 [EXE]
-->

Note:
Task priority must be the lowest (depending on application code and zephyr
lowest supported priority). Anyway, an application never letting lower priority
task execute (so never going to background task) may avoid the profiler task
to execute. In this case, NANOKERNEL below method can be used (calling
prof_flush in the main loop, where processing will be the least impacted)

    2.1.2) NANOKERNEL mode
    ----------------------

prof_flush function must be called from main task before going in idle mode

<--snippet
#include "profiler.h"

...

void main(void)
{
...
	while(1) {
...
		prof_flush();
		// Going to idle e.g. calling nano_cpu_idle()
-->

Note that profiler src folder must be added to the Makefile:
$APP_BASE/src/Makefile:

<--snippet
ccflags-y += -I${ZEPHYR_BASE}/samples/task_profiler/profiler/src
-->

  2.2) Add path to $PROFILER_BASE folder in application Makefile
  --------------------------------------------------------------

For example in $APP_BASE/src/Makefile:

<--snippet
obj-y += $PROFILER_BASE/
-->

Note:
- the final "/" after $PROFILER_BASE is important
- the path must be relative to application src folder

  2.3) Increase console UART baud rate (depending on your board)
  ------------------------------------

On Quark X1000, maximum standard baud rate is 921600
This is done by adding following line in $APP_BASE/prj.conf:

<--snippet
CONFIG_UART_NS16550_PORT_1_BAUD_RATE=921600
-->

Profiler has been tested at 2 Mbauds on Quark SE.

  2.4) (Galileo only) disable UART 0
  ----------------------------------

Due to an issue with Quark X1000 UART_0 IRQ mapping, UART0 must be disabled in
$APP_BASE/prj.conf:

<--snippet
CONFIG_UART_NS16550_PORT_0=n
-->

3) Enable profiler console for dynamic control
----------------------------------------------

For having the capability to dynamically enable/disable/configure the profiler
at runtime, shell command is supported.

For that purpose, following flags must be enabled in project configuration file
(e.g. $APP_BASE/prj.conf):

<-- snippet
CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC=y
CONFIG_CONSOLE_HANDLER=y
CONFIG_CONSOLE_HANDLER_SHELL=y
CONFIG_MINIMAL_LIBC_EXTENDED=y
-->

CONFIG_MINIMAL_LIBC_EXTENDED is required for atoi() function used in profiler shell
command implementation.

If done, the profiler will automatically enable a shell over UART allowing to
use commands to start/stop/configure the profiler:

- Start profiling:
prof start
- Stop profiling:
prof stop
- Configure profiler:
prof cfg <cfg1> <cfg2>
This sets the kernel event logger configuration (cfg1) and kernel monitor
configuration (cfg2)
By default cfg1 is set to log all events enabled at build time and cfg2 is set
to CONFIG_TASK_MONITOR_MASK
The new configuration is applied for the following start of the profiler.
Any on-going profiling session isn't altered

- Read profiler configuration
prof cfg
Get return like X(Y) A(B)
X being currently used kernel event logger configuration / Y being the next
  value (Set by cfg command).
A being the currently used kernel monitor configuration / B being the next
  value.

WARNING: if the application has already defined its shell, then the profiler
command can be added to the list of application commands this way (apps_cmd
being the array of shell commands defined by the app)

<-- snippet
#include "profiler.h"

const struct shell_cmd apps_cmd[] = {
	...
	<apps commands defined here>
	...
	PROF_CMD,
	{ NULL, NULL}
}
-->

Note that profiler src folder must be added to the Makefile:
$APP_BASE/src/Makefile:

<--snippet
ccflags-y += -I${ZEPHYR_BASE}/samples/task_profiler/profiler/src
-->

Additionally, the profiler must not register its console so
PROFILER_NO_SHELL_REGISTER must be set to 1 while building:
export PROFILER_NO_SHELL_REGISTER=1
or
make BOARD=... PROFILER_NO_SHELL_REGISTER=1

4) Build and flash project
--------------------------

Flags that can be used by the profiler
PROFILER_NO_SHELL_REGISTER: when profiler dynamic control is set, the profiler
                            won't set its own console handler. In that case the
                            profiler commands must be added to the application
                            console handler

PROFILER_USECOUNTER: in case custom timestamp is set for the kernel event
                     logger, the profiler will use RTC. This flag allows to
                     use a counter instead

5) Get prepared for collecting profiler data
--------------------------------------------

  5.1) for using JTAG on galileo with OpenOCD
  -------------------------------------------

Connect OpenOCD JTAG to PC

<--snippet
cd $OPENOCD_FOLDER
openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg -f board/quark_x10xx_board.cfg
cd $APP_BASE
gdb outdir/$BOARD/zephyr.elf
(gdb) target remote localhost:3333
(gdb) c
-->

  5.2) for using UART on any board
  --------------------------------

- connect console UART to host PC
- start "profterm"

<--snippet
cd $PROFILER_BASE/scripts/term/
make
cd $PROFILER_BASE/scripts
./profterm /dev/<TTY_USB> prof.log console.log
-->

<TTY_USB> being the board UART console device

Note: profterm UART rate can be set using -s option. Otherwise default speed is
921600.

  5.3) for using with QEMU (UART flushing mode)
  ---------------------------------------------

Two terminals are required
- one for running QEMU, basically redirecting serial to a virtual device
- one for running profterm (as it would be done with a board)

<--snippet
cd $PROFILER_BASE/scripts/term/
make
-->

Term1: QEMU must be launched the following way

<--snippet
cd $APP_BASE
make BOARD=... QEMU_PTY=1 qemu
-->

QEMU will display virtually created TTY
"char device redicected to /dev/pts/XY"

Term2:
<--snippet
./profterm /dev/pts/XY prof.log console.log
-->

6) Run program
--------------

7) Get log (JTAG only)
----------------------

Break execution and dump buffer
<--snippet
(gdb) ** break <CTRL-C> **
(gdb) dump binary value prof.log *(int[<BUFFER_SIZE>] *)sys_k_event_logger.ring_buf.buf
-->

<BUFFER_SIZE> being the buffer size defined in $APP_BASE/prj.conf file

8) Post-processing
------------------

WARNING: zephyr ELF file is required for post-processing. This file must be
kept together with dump files in order to be able to run the post-processing.

profile script under $PROFILER_BASE/scripts must be called in its folder for the
moment
Run "./profile.sh -h" for usage

Note that if using JTAG, HW cycles per sec is not part of the dump and so HW
cycles per sec must be provided to "profile" script using -c option (see
profile.sh help)

Notes:
- python is required for the post processing. matplotlib shall be installed for
graphical output

