.. _debugmon:

Cortex-M Debug Monitor
######################

Monitor mode debugging is a Cortex-M feature, that provides a non-halting approach to
debugging. With this it's possible to continue the execution of high-priority interrupts,
even when waiting on a breakpoint.
This strategy makes it possible to debug time-sensitive software, that would
otherwise crash when the core halts (e.g. applications that need to keep
communication links alive).

Zephyr provides support for enabling and configuring the Debug Monitor exception.
It also contains a ready implementation of the interrupt, which can be used with
SEGGER J-Link debuggers.

Configuration
*************

Configure this module using the following options.

* :kconfig:option:`CONFIG_CORTEX_M_DEBUG_MONITOR_HOOK`: enable the module. This option, by itself,
  requires an implementation of debug monitor interrupt that will be executed
  every time the program enters a breakpoint.

With a SEGGER debug probe, it's possible to use a ready, SEGGER-provided implementation
of the interrupt.

* :kconfig:option:`CONFIG_SEGGER_DEBUGMON`: enables SEGGER debug monitor interrupt. Can be
  used with SEGGER JLinkGDBServer and a SEGGER debug probe.


Usage
*****

When monitor mode debugging is enabled, entering a breakpoint will not halt the
processor, but rather generate an interrupt with ISR implemented under
``z_arm_debug_monitor`` symbol.  :kconfig:option:`CONFIG_CORTEX_M_DEBUG_MONITOR_HOOK` config configures this interrupt
to be the lowest available priority, which will allow other interrupts to execute
while processor spins on a breakpoint.

Using SEGGER-provided ISR
=========================

The ready implementation provided with :kconfig:option:`CONFIG_SEGGER_DEBUGMON` provides functionality
required to debug in the monitor mode using regular GDB commands.
Steps to configure SEGGER debug monitor:

1. Build a sample with :kconfig:option:`CONFIG_CORTEX_M_DEBUG_MONITOR_HOOK`` and :kconfig:option:`CONFIG_SEGGER_DEBUGMON`
   configs enabled.

2. Attach JLink GDB server to the target.
   Example linux command: ``JLinkGDBServerCLExe -device <device> -if swd``.

3. Connect to the server with your GDB installation.
   Example linux command: ``arm-none-eabi-gdb --ex="file build/zephyr.elf" --ex="target remote localhost:2331"``.

4. Enable monitor mode debugging in GDB using command: ``monitor exec SetMonModeDebug=1``.

After these steps use regular gdb commands to debug your program.


Using other custom ISR
======================
In order to provide a custom debug monitor interrupt, override ``z_arm_debug_monitor``
symbol. Additionally, manual configuration of some registers is required
(see :zephyr:code-sample:`debugmon` sample).
