.. _cy8ckit_062_wifi_bt_m4:

PSoC6 WiFi-BT Pioneer Kit (CM4 Core)
####################################

Overview
********

See :ref:`cy8ckit_062_wifi_bt` for a general overview of the
CY8CKIT_062_WIFI_BT board.
The Cortex-M4 is a secondary core on the board's SoC. It should be started from
the CM0+ core.

Programming and Debugging
*************************

Only the CM0+ core starts by default after the MCU reset.
In order to have CM4 core working FW for both cores should be written into
Flash. CM0+ FW should starts the CM4 core at one point using
Cy_SysEnableCM4(CM4_START_ADDRESS); call. CM4_START_ADDRESS is 0x10060000 in
the current configuration. The CM0+/CM4 Flash/SRAM areas are defined in
:zephyr_file:`dts/arm/cypress/psoc6.dtsi`.
