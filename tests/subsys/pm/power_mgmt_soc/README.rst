.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

SoC power management tests
##########################

The default ``pm.soc`` scenario exercises the board's configured power states.

Suspend-to-RAM
**************

The opt-in ``pm.soc.s2ram`` and ``pm.soc.s2ram.no_fpu`` scenarios exercise
retained execution on ``frdm_mcxw71/mcxw716c`` and
``frdm_mcxw72/mcxw727c``. They select the S2RAM multithread test instead of the
ordinary tests; default board configuration and ordinary test discovery are
unchanged.

For example, build the FP-enabled scenario with:

.. code-block:: console

   west build -b frdm_mcxw71/mcxw716c tests/subsys/pm/power_mgmt_soc \
     -d build/s2ram-w71 -- -DTEST_S2RAM=ON -DEXTRA_CONF_FILE=s2ram.conf \
     -DDTC_OVERLAY_FILE=mcxw7x_s2ram.overlay \
     -DCONFIG_FPU=y -DCONFIG_FPU_SHARING=y

Use the W72 board target for the other board. Set both ``CONFIG_FPU`` and
``CONFIG_FPU_SHARING`` to ``n`` for the no-FP scenario.

The overlay places kernel/application RAM in 32 KiB at ``0x30008000``, outside
both targets' ROM exclusions. Use standalone secure application
core0, internal-flash XIP, no NBU firmware and software CRC dispatch. Factory
ROM must permit fast wake without NPX/FLW reconstruction.

The ``powerdown`` DT node maps hardware Power Down to Zephyr Suspend-to-RAM.
The target-specific ``s2ram_reserved`` node describes both excluded aliases.
W71 excludes the first 16 KiB of STCM (MCXW71 RM Rev. 3, 14.3.2/15.2);
its documented ROM API workspace in 17.1 is within that area. W72 excludes
the first 32 KiB (MCXW72 RM Rev. 5, 15.2), not only the 16 KiB initialized
on wake in 14.4.2. These bounds are independent of the SDK demo's layout.
The cold ECC hook's clearing extent does not define the warm ROM workspace.

Static RAM policy
=================

``CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM=y`` powers off unused CMC
banks once at initialization, in all modes. Geometry comes from ``cmc``'s
``sram-banks`` property. A bank remains powered if it overlaps the entire
chosen SRAM, chosen on-chip flash, the secure ROM workspace, or an enabled
``zephyr,memory-region``. Shrink chosen SRAM and declare any additional
live regions; an enabled named region keeps its whole intersecting bank,
not only its used bytes. For example, an additional W71 STCM region is:

.. code-block:: devicetree

   &stcm {
       retained_data: memory@18000 {
           compatible = "zephyr,memory-region", "mmio-sram";
           reg = <0x18000 DT_SIZE_K(8)>;
           zephyr,memory-region = "APP_RETAINED";
       };
   };

Use the resulting memory region for the corresponding linker placement.
DT keep regions must use the apertures described by ``sram-banks``; a
non-secure alias does not independently cause the existing secure-addressed
bank calculation to keep a bank. With unused-bank shutdown disabled, this
policy does not program SRAMDIS. S2RAM retains every powered bank and supplies
the independent upper 8 KiB through VBAT. It restores the original SRAMDIS
mask after wake or an entry abort.

The final-ELF checker uses the generated DT and configuration, not
``_image_ram_end``. Every nonempty allocated section, including NOBITS,
must fit local CTCM/STCM or chosen flash. Both TCM aliases are checked
against ROM exclusions and disabled banks. Unsupported memory fails rather
than being assumed retained. The standalone overlay disables unused radio
SMU2 because the ordinary linker otherwise allocates a shared-memory
placeholder there even without NBU. Do not put live data in that region.
These checks apply only with S2RAM enabled; ordinary PM/default layouts
are unchanged.

This is a static policy, not permission to discard live memory or borrow
the ROM area temporarily. Allocation draining, DMA quiescence, suspend veto,
abort rollback and wake reconstruction would need a separate lifecycle
contract; the current retained-memory access APIs do not supply one.

Timing and retained execution
=============================

The shared exit-latency budget is 3 ms, including SoC/PM restoration. The
`MCXW71 data sheet <https://www.nxp.com/docs/en/data-sheet/MCXW71.pdf>`_
(Rev. 3, Table 19) specifies a 234.59 us maximum hardware Power Down-to-Active
delay; the `MCXW72 data sheet <https://www.nxp.com/docs/en/data-sheet/MCXW72.pdf>`_
(Rev. 5, Table 19) gives 158.20 us typical without a maximum. These hardware
figures do not cover the complete Zephyr restore path.

Separate characterization at 96 MHz with the 32768 Hz LPTMR observed upper
exit estimates of 550 us on W71 and 1069 us on W72 over ten genuine recoveries
per FP/no-FP profile. Each estimate includes one counter quantum. The 3 ms
budget rounds twice the larger observation up to a whole millisecond; it is
not an all-voltage/temperature worst-case guarantee. Applications must increase
it for slower device-resume paths. The one-second minimum residency remains
a conservative admission choice, not a measured energy break-even point.
The forced sleeps below do not test default-policy admission.

Ten forced 500 ms sleeps must each invoke the real architecture suspend once
and return zero. The test checks retained data, BSS, heap and main/worker stack
patterns, completed sleep time and worker progress after wake. Each successful
iteration prints ``PM S2RAM cycle N PASS`` before the final ztest result.
Skipped or aborted entries fail rather than counting as successful PM events.
Board support also captures SRAM bank-disable policy, sleep-debug control and
actual core frequency before console reconstruction, then checks them against
entry values. The shutdown-disabled variant also requires a zero disable mask.

A custom policy stays active except for the forced transition. MCXW board
support drains the default UART1 console before entry and restores its pinctrl
and cached UART configuration from the test thread after recovery. No extra
UART connection or button is required; LPTMR0 supplies timed wake.

State ownership and restoration
===============================

MAIN core/cache and peripheral operating state requires recovery. WAKE
clock configuration is guarded against warm-boot changes: unchanged
configuration is not reinitialized, but oscillator readiness is still checked.
The existing clock driver owns SCG/MRCC/flash-clock snapshots and early
restoration. Its context is placed in ``noinit`` within retained SRAM; verify
the actual final-ELF section, not only the variable's address or compilation.
Two SoC-internal calls supply the CPU-before-device-resume ordering
that ordinary clock device PM cannot provide; public clock_control and WUC
interfaces are unchanged. MRCC cells use per-register writable masks, not
whole-register snapshots or an assumed consumer-domain retention map.
Clock-only SRAM/timer cells have no reset replay. SECSUBSYS is gated through
clock_control and both its clock and reset-negation fields are restored.
Temporary entry controls, including sleep-debug gating, are rolled back on
success or abort. A consumer's MAIN/WAKE domain does not establish the physical
retention of each MRCC control cell. The native domain references are W71 RM
24.3.2/30.2/30.4 and W72 RM 22.3.2/28.2/28.4; they are not a complete
per-register-cell retention map. Generic SDK reinitialization and observing
reset-default values are not proof that a saved field loses power.

.. list-table:: Reasons for the saved state
   :header-rows: 1
   :widths: 32 40 28

   * - State
     - Reason
     - Owner
   * - CPU, SCB, NVIC, MPU, FP and cache operating state
     - MAIN/core recovery, not a retained clock configuration claim
     - Architecture helpers and SoC
   * - SCG RCCR, FIRC/SOSC controls, configuration/trim and original locks;
       W71 SIRC controls
     - Warm-change guards; compare requested and actual source/dividers and
       retain only target-defined writable configuration
     - Clock driver; mismatch-only reconstruction with bounded readiness checks
   * - MRCC CC; CC/RSTB; CC/RSTB/MUX/DIV groups
     - Guard uncertain register-cell retention; exclude PR/status, reserved
       fields and RF BIST. Clock-only cells receive no invented reset replay
     - Clock driver's target-specific masks (36 W71 entries, 44 W72 entries)
   * - MRCC SECSUBSYS CC/RSTB
     - Entry explicitly gates the clock and asserts reset; both original
       fields must return on genuine wake and real-WFI abort
     - Normal clock_control gate operation and the same early clock owner
   * - FMU FCTRL.RWSC and SystemCoreClock
     - Flash timing must be safe before a faster clock; reconcile the
       configuration-derived core rate with the saved profile
     - Clock driver
   * - CMC SRAMDIS
     - Preserve the configured static policy against observed warm-path changes
     - SoC, restoring only implemented bits
   * - CMC SRAMRET/CKCTRL/PMCTRL/DBGCTL.SOD, SPC LP_CFG,
       VBAT controls and RFMC host request
     - Temporary entry-owned retention, debug and power controls
     - SoC success/abort rollback; preserve pre-existing requests
   * - SPC WAKEUP, record CRC, marker and diagnostic
     - ROM recovery protocol and attempt lifetime
     - SoC and existing architecture marking contract

This is a retained-execution regression, not full SoC acceptance. It does not
inject entry aborts, verify every FP/core register, probe every memory bank or
qualify actual power-on-reset/ECC behavior. FP/no-FP builds exercise their
respective production paths, not an exact FP-register preservation assertion.
Other MAIN-domain peripherals require their own context restoration.
