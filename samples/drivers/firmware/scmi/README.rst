.. zephyr:code-sample:: scmi
   :name: SCMI Platform Interaction

   Interact with the SCMI platform using various protocols.

Overview
********

This is a simple demo, which allows an SCMI agent running Zephyr to print information about and
configure parts of the system by interacting with the SCMI platform using various protocols.

Building
********

This demo can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/firmware/scmi
   :board: imx95_evk/mimx9596/m7
   :goals: build

Sample Output
=============

.. code-block:: console

   uart:~$ scmi
   scmi - ARM SCMI commands
   Subcommands:
     clk  : Clock protocol commands
   uart:~$ scmi clk
   clk - Clock protocol commands
   Subcommands:
     version      : get protocol version
                    Usage: version
     summary      : get clock tree summary
                    Usage: summary
     info         : get detailed clock information
                    Usage: info <clock_id>
     set-enabled  : enable/disable a clock
                    Usage: set-enabled <clock_id> on|off
     set-rate     : set a clock's rate (in Hz)
                    Usage: set-rate <clock_id> <rate>
     set-parent   : set a clock's parent
                    Usage: set-parent <clock_id> <parent_id>
   uart:~$ scmi clk summary
   +----------------------------------------------------------------------+
   | ID |        Name        | Enabled |   Rate(Hz)  |        Parent      |
   +----------------------------------------------------------------------+
   |  0 |               ext  |    Y    |    25000000 |       ext_gpr_sel  |
   +----------------------------------------------------------------------+
   |  1 |            osc32k  |    Y    |       32768 |               N/A  |
   +----------------------------------------------------------------------+
   |  2 |            osc24m  |    Y    |    24000000 |               N/A  |
   +----------------------------------------------------------------------+
   |  3 |               fro  |    Y    |   256000000 |               N/A  |
   +----------------------------------------------------------------------+
   |  4 |       syspll1_vco  |    Y    |  4000000000 |               N/A  |
   +----------------------------------------------------------------------+
   |  5 |   syspll1_pfd0_un  |    Y    |  1000000000 |       syspll1_vco  |
   +----------------------------------------------------------------------+
   |  6 |      syspll1_pfd0  |    Y    |  1000000000 |   syspll1_pfd0_un  |
   +----------------------------------------------------------------------+
   |  7 |   syspll1_pfd0_di  |    Y    |   500000000 |   syspll1_pfd0_un  |
   +----------------------------------------------------------------------+
   uart:~$ scmi clk info 50
   Name: lpspi2
   Enabled status: Y
   Rate (Hz): 50000000
   Parent: syspll1_pfd1_di [10]
