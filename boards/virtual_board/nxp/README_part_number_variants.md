<!--
SPDX-FileCopyrightText: 2026 NXP
SPDX-License-Identifier: Apache-2.0
-->

# NXP virtual-board generator: SoC part-number variants

The NXP virtual-board generator can optionally create **one virtual-board target per
SoC part number** defined in NXP `Kconfig.soc` files.

This is useful for families like **i.MX RT118x** where multiple `SOC_PART_NUMBER_*`
Kconfig symbols exist (e.g. `SOC_PART_NUMBER_MIMXRT1187CVM8C`) and some build flows
require selecting a concrete part-number symbol.

## Enable the mode

In the generator configuration JSON (for example
`boards/virtual_board/nxp/defs/nxp_virtual.json`), set:

```json
{
  "include_part_number_variants": true
}
```

> The repo default is `false` to preserve the historical behavior.

## Run the generator

```sh
cd $ZEPHYR_BASE

python3 boards/virtual_board/tools/gen_nxp_virtual_board.py \
  --configuration boards/virtual_board/nxp/defs/nxp_virtual.json
```

## What gets generated

Output directory (per config):

- `boards/virtual_board/generated/boards/nxp/nxp_virtual/`

### Qualifiers (YAML `identifier:`)

Targets become a 3-segment qualifier when a part-number is present:

- `nxp_virtual/<soc>/<cpucluster>/<part-number>`

Example:

- `nxp_virtual/mimxrt1189/cm33/MIMXRT1187CVM8C`

### Files

For each generated target, the generator emits:

- `nxp_virtual_<soc>_<cpucluster>_<part>.[dts|yaml]`
- `nxp_virtual_<soc>_<cpucluster>_<part>_defconfig`

Example:

- `nxp_virtual_mimxrt1189_cm33_mimxrt1187cvm8c.dts`
- `nxp_virtual_mimxrt1189_cm33_mimxrt1187cvm8c.yaml`
- `nxp_virtual_mimxrt1189_cm33_mimxrt1187cvm8c_defconfig`

### Kconfig behavior

The generated board Kconfig symbol for that target selects both:

- the SoC symbol, e.g. `SOC_MIMXRT1189_CM33`
- the part number symbol, e.g. `SOC_PART_NUMBER_MIMXRT1187CVM8C`

Note: `SOC_PART_NUMBER_*` symbols are typically **not directly user-configurable**
(no prompt). Assigning `CONFIG_SOC_PART_NUMBER_*` in defconfig is treated as a
fatal Kconfig warning. Therefore the generator enables the part number via the
generated board Kconfig `select SOC_PART_NUMBER_<...>`.

## Notes

- `SOC_PART_NUMBER_*` symbols are discovered by scanning `soc/**/Kconfig.soc`.
- DTSI overrides and patch overrides still key off the *base* qualifier:
  - `soc` or `soc/cpucluster` (variant is ignored for override lookups)
  so you do not need to duplicate JSON overrides per part-number.
