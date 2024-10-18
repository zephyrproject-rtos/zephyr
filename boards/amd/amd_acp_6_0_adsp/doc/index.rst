.. _amd_acp_6_0_adsp:

AMD ACP_6_0
############

Overview
********

ACP_6_0 ADSP board is based on AMD ACP_6_0 platform,
Zephyr OS is ported to run on the HiFi5 DSP .

- Board features:

  - RAM & Storage: 1.75 MB HP SRAM / 512 KB IRAM/DRAM
  - Audio Interfaces:
      - 1 x SP (I2S, PCM),
      - 1 x BT (I2S, PCM),
      - 1 x HS(I2S, PCM),
      - DMIC

Supported Features
==================

The following hardware features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| I2S       | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+

System Clock
------------

This board configuration uses a system clock frequency of @ 200 - 800MHz.
