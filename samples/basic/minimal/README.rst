.. zephyr:code-sample:: minimal
   :name: Minimal footprint

   Measure Zephyr's minimal ROM footprint in different configurations.

Overview
********

This sample provides an empty ``main()`` and various configuration files that
can be used to measure Zephyr's minimal ROM footprint in different
configurations.

The following configuration files are available:

* :file:`mt.conf`: Enable multithreading
* :file:`no-mt.conf`: Disable multithreading
* :file:`no-preempt.conf`: Disable preemption
* :file:`no-timers.conf`: Disable timers
* :file:`arm.conf`: Arm-specific disabling of features

Building and measuring ROM size
*******************************

The following combinations are suggested for comparing ROM sizes in different
configurations. They all target the :ref:`reel_board` (Arm Aarch32 architecture).

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
