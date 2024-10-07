.. amd_acp_6_0_adsp:
AMD ACP_6_0 ADSP
###############

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
More information about the board can be found at the
`AMD website`_.
  https://www.amd.com/en/products/processors/laptop/ryzen.html#tabs-1181ea0b44-item-a350828a92-tab

Supported Features
==================

The Zephyr AMD_RMB_ADSP board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| I2S       | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+

System Clock
------------

This board configuration uses a system clock frequency of @ 200 - 800MHz.
