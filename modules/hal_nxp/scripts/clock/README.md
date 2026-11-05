<!--
SPDX-FileCopyrightText: (C) 2026 NXP

SPDX-License-Identifier: Apache-2.0
-->

# MCUXpresso rev3 Clock Identifier Generation Scripts

These scripts generate the Zephyr `nxp,imx-ccm-rev3` clock-identifier binding
header for a SoC directly from that SoC's MCUXpresso SDK `fsl_clock.h`. The
identifiers a peripheral devicetree node passes to the rev3 clock controller
(`IMX_CCM_LPCG_*`, `IMX_CCM_ROOT_*`, `IMX_CCM_MUX_*`) must equal the HAL
enumerations (`clock_lpcg_t`, `clock_root_t`, `clock_root_mux_source_t`) the
controller forwards them to. Generating the header from those enums is what
makes "the values cannot drift from the HAL" a guarantee rather than a comment.

| Script                  | Functionality                                              |
| ----------------------- | --------------------------------------------------------- |
| `gen_ccm_rev3_ids.py`   | Generates the `imx_ccm_rev3_<soc>.h` binding header from `fsl_clock.h` |

## Data source

Unlike the pinctrl generators, this tool takes no MCUXpresso Config Tools data
pack. Its only input is the SoC's `fsl_clock.h`, shipped in `hal_nxp` under
`mcux/mcux-sdk-ng/devices/<family>/<soc>/<part>/drivers/fsl_clock.h`. The
enumerator *values* are read from the explicit `= <n>` assignments in the enum
bodies — never from positional counting — because a root that appears in more
than one enum group (e.g. NPU, QTPM0, USB1 on RT266x) numbers its mux sources
from zero in each group. Range markers (`*_START` / `*_END`) and invalid
sentinels (`kCLOCK_Root_Invalid`) are intentionally not exported.

## Supported SoCs

The parsing framework is shared across every rev3-family SoC. Each SoC
contributes a small configuration under `socs/` describing its output header
name, display name, and any naming exceptions.

| SoC config (`--soc`) | Header generated (in Zephyr)                         |
| -------------------- | ---------------------------------------------------- |
| `rt266x`             | `include/zephyr/dt-bindings/clock/imx_ccm_rev3_rt266x.h` |

To add another rev3 SoC, add `socs/<soc>.py` exporting a `CONFIG` object with
the same interface as `socs/rt266x.py` and list it in the table above. A config
provides:

- `soc` / `display` -- names used in the guard and file comment.
- `id_space_doc` -- the SoC-specific bullet list folded into the header comment.
- `lpcg_tail(enumerator)` / `root_tail(enumerator)` / `mux_name(enumerator)` --
  turn a HAL enumerator into the macro tail. `mux_name` returns `None` to skip an
  entry that is not a real per-root source.

The framework already absorbs the variations seen across rev3 SoCs, so a config
only overrides what its SDK does differently:

- **Missing driver version.** Some SoCs (e.g. i.MX95) ship `fsl_clock.h` without
  `FSL_CLOCK_DRIVER_VERSION`; the header records `n/a`. No config change needed.
- **Missing `clock_lpcg` enum.** i.MX95 has no LPCG enum at all; the LPCG section
  is simply omitted. No config change needed.
- **`Mux`-prefixed sources.** RT1180/i.MX95 name mux sources
  `kCLOCK_M7_ClockRoot_MuxOscRc24M` (with a `Mux` infix) where RT266x does not.
  Strip that prefix in the SoC's `mux_name` -- it is deliberately not stripped in
  the framework, because a source could legitimately contain the letters `Mux`.
- **Instance-prefixed vs bare LPCG names.** Both forms flow through `lpcg_tail`
  unchanged; override it only if a SoC needs a different tail.

The range markers (`*_START` / `*_END`) and invalid sentinels are filtered by
the framework for every SoC.

## Generating a header

```
cd /path/to/zephyrproject
python zephyr/modules/hal_nxp/scripts/clock/gen_ccm_rev3_ids.py --soc rt266x \
    --fsl-clock modules/hal/nxp/mcux/mcux-sdk-ng/devices/RT/RT2660/MIMXRT2663/drivers/fsl_clock.h \
    --source-version "hal_nxp <commit> / RT2660 MIMXRT2663 public SDK"
```

The tool is explicitly invoked for maintenance. Normal `west build` consumes
the committed header and does not run the generator.

Use complete long option names; abbreviated options are rejected, following
Zephyr's command-line convention.

`--output` defaults to `include/zephyr/dt-bindings/clock/imx_ccm_rev3_<soc>.h`
inside the Zephyr repository, independent of the current working directory.

`--source-version` is stamped verbatim into the header's file comment so the
generated artifact records which HAL revision it was produced from. Omitting it
is allowed but leaves the header unversioned.

## Checking a header is up to date (CI)

`--check` regenerates in memory and compares byte-for-byte against the target
header, exiting 1 on any difference without writing. This is the mechanism that
keeps the checked-in header honest:

```
python gen_ccm_rev3_ids.py --soc rt266x \
    --fsl-clock /path/to/hal_nxp/mcux/mcux-sdk-ng/devices/RT/RT2660/MIMXRT2663/drivers/fsl_clock.h \
    --source-version "hal_nxp <commit> / RT2660 MIMXRT2663 public SDK" \
    --check
```

Verified behaviour (hold `--source-version` constant, or the stamp alone makes the
bytes differ): unmodified header exits 0, changing a single macro exits 1,
restoring it exits 0 again.

The generator and output header live in Zephyr; the `fsl_clock.h` input remains
in `hal_nxp`. A CI job running `--check` therefore needs both trees and must pass
the same `--source-version` used to generate the header, because the stamp is
part of the compared bytes. Devicetree includes the generated header as
`<zephyr/dt-bindings/clock/imx_ccm_rev3_rt266x.h>`, alongside the SoC-agnostic
packed-cell header `<zephyr/dt-bindings/clock/imx_ccm_rev3.h>`.

### The driver version is a signal, not the gate

The header records the `FSL_CLOCK_DRIVER_VERSION` it was generated from (parsed
from `MAKE_VERSION(a, b, c)`, or `n/a` when the SoC omits the macro). This lets a
CI job report *why* a header is stale -- "HAL clock driver bumped 1.1.0 -> 1.2.0"
reads better than a raw byte diff -- and lets a human see at a glance whether the
HAL moved.

It is deliberately **not** used to skip the byte-exact `--check`. The version
tags the driver *code*, not the three enums; NXP can (and has) changed an
enumerator without bumping it. Gating regeneration on the version alone would let
exactly the drift this tool exists to catch pass silently. So CI always runs the
full `--check`, even when the version is unchanged -- regenerating ~800 lines and
comparing is cheap, and it is the only check that cannot be fooled.

## Do not edit the generated header by hand

Edit `fsl_clock.h` (i.e. take a newer HAL revision) or the SoC config, then
regenerate. A hand edit to the header will be reverted the next time the header
is regenerated, and `--check` will flag it in the meantime.
