Description
===========

zperf is a network traffic generator for Zephyr that may be used to
evaluate network bandwidth.

Features
=========

- Compatible with iPerf_2.0.5.
- Support for micro and nano kernel modes.
- Client or server mode allowed without need to modify the source code.
- Working with task profiler (PROFILER=1 to be set when building zperf)

Supported Boards
================

zperf is board-agnostic. However, zperf requires a network interface.
So far, zperf has been tested only on the Intel Galileo Development Board.
