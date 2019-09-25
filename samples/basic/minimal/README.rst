.. _minimal_sample:

Minimal sample
##############

Overview
********

This sample defines An empty ``main()`` and a set of minimal configurations
that provide tests for the smallest ROM sizes possible with the Zephyr kernel.

The following configuration files are available:

* :file:`mt.conf`: Enable multithreading
* :file:`no-mt.conf`: Disable multithreading
* :file:`no-preempt.conf`: Disable preemption
* :file:`no-timers.conf`:: Disable timers
* :file:`arm.conf`: Arm-specific disabling of features

Building and measuring ROM size
*******************************

In order to compare ROM sizes with different minimal configurations, the
following combinations are suggested:

* Reel board (Arm architecture)

  * Multithreading enabled

    * Reference ROM size: 7-8KB

    .. zephyr-app-commands::
       :zephyr-app: samples/basic/minimal
       :host-os: unix
       :board: reel_board
       :build-dir: reel_board/mt/
       :conf: "common.conf mt.conf arm.conf"
       :goals: rom_report
       :compact:

  * Multithreading enabled, no preemption

    * Reference ROM size: 7-8KB

    .. zephyr-app-commands::
       :zephyr-app: samples/basic/minimal
       :host-os: unix
       :board: reel_board
       :build-dir: reel_board/mt-no-preempt/
       :conf: "common.conf mt.conf no-preempt.conf arm.conf"
       :goals: rom_report
       :compact:

  * Multithreading enabled, no preemption, timers disabled

    * Reference ROM size: 3-4KB

    .. zephyr-app-commands::
       :zephyr-app: samples/basic/minimal
       :host-os: unix
       :board: reel_board
       :build-dir: reel_board/mt-no-preempt-no-timers/
       :conf: "common.conf mt.conf no-preempt.conf no-timers.conf arm.conf"
       :goals: rom_report
       :compact:

  * Multithreading disabled, timers enabled

    * Reference ROM size: 4-5KB

    .. zephyr-app-commands::
       :zephyr-app: samples/basic/minimal
       :host-os: unix
       :board: reel_board
       :build-dir: reel_board/no-mt/
       :conf: "common.conf no-mt.conf arm.conf"
       :goals: rom_report
       :compact:

  * Multithreading disabled, timers disabled

    * Reference ROM size: 2-3KB

    .. zephyr-app-commands::
       :zephyr-app: samples/basic/minimal
       :host-os: unix
       :board: reel_board
       :build-dir: reel_board/no-mt-no-timers/
       :conf: "common.conf no-mt.conf no-timers.conf arm.conf"
       :goals: rom_report
       :compact:

