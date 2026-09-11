# PSE84 (KIT_PSE84_EVAL) Memory Map & Protection Domains in the TF-M case

Target: `pse846gps2dbzc4a` — Infineon PSOC Edge E84, dual-core Arm (Cortex‑M33 "SYSCPU" + Cortex‑M55 "APPCPU") with an Ethos‑U55 NPU.

This document summarizes **what memories exist**, **how they are
partitioned**, and **which protection domain owns each region**.

---

## 1. Physical memories

| Memory type (`IFX_MEMORY_TYPE_*`) | Kind | Notes |
|---|---|---|
| `ITCM` (1) | Tightly-coupled code RAM | CM55 instruction TCM |
| `DTCM` (0) | Tightly-coupled data RAM | CM55 data TCM |
| `RRAM` (2) | Non-volatile (on-chip) | Bootloader, user NVM, TF-M ITS/PS/NV counters |
| `SRAM` (5) | Volatile system RAM | CM33 ram code/data + shared regions |
| `SOCMEM_RAM` (4) | Volatile system-on-chip RAM | CM55 ram code/data, GFX, shared |
| `SMIF0MEM1` (3) | External QSPI NOR flash | MCUboot slots (primary/upgrade/trailer) |

---

## 2. Memory regions

Addresses below use the primary non-cached alias. Sizes are exact.

### 2.1 RRAM

| Region | Start | Size | Owner / purpose |
|---|---|---|---|
| `bootloader_nvm`    | `0x22011000` | `0x28000` (160 KB) | MCUboot / boot code (secure) |
| `user_nvm`          | `0x22039000` | `0x2A000` (168 KB) | Shared user NVM (non-secure) |
| `TFM_ITS`           | `0x22063000` | `0x02000` (8 KB)   | TF-M Internal Trusted Storage |
| `TFM_PS`            | `0x22065000` | `0x04000` (16 KB)  | TF-M Protected Storage |
| `TFM_NV_COUNTERS`   | `0x22069000` | `0x01000` (4 KB)   | TF-M rollback/NV counters |

### 2.2 SRAM

| Region | Start | Size | Owner / purpose |
|---|---|---|---|
| `m33s_shared`             | `0x24001000` | `0x01000` (4 KB)   | Secure↔NS shared (M33 secure) |
| `m33s_data`               | `0x24037000` | `0x21000` (132 KB) | M33 secure (TF-M) data |
| `m33_code`                | `0x24058000` | `0x65000` (404 KB) | M33 non-secure code |
| `m33_data`                | `0x240BD000` | `0x40000` (256 KB) | M33 non-secure data |
| `m33s_allocatable_shared` | `0x240FD000` | `0x01000` (4 KB)   | Allocatable shared (M33 secure) |
| `m33_allocatable_shared`  | `0x240FE000` | `0x01000` (4 KB)   | Allocatable shared (M33 NS) |
| `m55_allocatable_shared`  | `0x240FF000` | `0x01000` (4 KB)   | Allocatable shared (M55) |

### 2.3 CITCM/DTCM

| Region | NS start | Secure start | Size | Purpose |
|---|---|---|---|---|
| `m55_code` | `0x48000000` | `0x58000000` | `0x40000` (256 KB) | CM55 ITCM (code) |
| `m55_data` | `0x48040000` | `0x58040000` | `0x40000` (256 KB) | CM55 DTCM (data) |

### 2.4 SOCMEM RAM

| Region | Start | Size | Owner / purpose |
|---|---|---|---|
| `m55_code_secondary` | `0x26000000` | `0x040000` (256 KB)   | CM55 code (RAM-boot secondary) |
| `m55_data_secondary` | `0x26040000` | `0x2BC000` (~2.73 MB) | CM55 data |
| `m33_m55_shared`     | `0x262FC000` | `0x040000` (256 KB)   | CM33↔CM55 shared IPC buffers |
| `gfx_mem`            | `0x2633C000` | `0x1C4000` (~1.77 MB) | Graphics (GFXSS) framebuffer |

### 2.5 External QSPI NOR flash (`SMIF0MEM1`)

| Region | Start | Size | Purpose |
|---|---|---|---|
| `m33s_nvm`          | `0x60100000` | `0x200000` (2 MB)    | M33 secure primary slot (code XIP) |
| `m33s_trailer`      | `0x60300000` | `0x040000` (256 KB)  | M33 secure image trailer |
| `m33_nvm`           | `0x60340000` | `0x200000` (2 MB)    | M33 non-secure primary slot |
| `m33_trailer`       | `0x60540000` | `0x040000` (256 KB)  | M33 NS image trailer |
| `m55_nvm`           | `0x60580000` | `0x2C0000` (2.75 MB) | M55 primary slot |
| `m55_trailer`       | `0x60840000` | `0x040000` (256 KB)  | M55 image trailer |
| `m33s_upgrade_slot` | `0x60880000` | `0x240000` (2.25 MB) | M33 secure upgrade (secondary) |
| `m33_upgrade_slot`  | `0x60AC0000` | `0x240000` (2.25 MB) | M33 NS upgrade (secondary) |
| `m55_upgrade_slot`  | `0x60D00000` | `0x300000` (3 MB)    | M55 upgrade (secondary) |

MCUboot flash-area IDs (`memorymap.h`): `FLASH_AREA_BOOTLOADER` and image
primary/secondary pairs `IMG_1..IMG_5`, plus swap-status and scratch areas.

---

## 3. Protection domains

Two independent protection mechanisms are configured on this device:

- **MPC (Memory Protection Controller)** — enforces security/access on *memory* per
  Protection Context (PC).
- **PPC (Peripheral Protection Controller)** — enforces security/access on *peripherals*.

Both are driven from `cycfg_system.c` and only take effect on the secure CM33 build
(`CY_PDL_TZ_ENABLED` / `COMPONENT_SECURE_DEVICE`), i.e. under TF-M.

### 3.1 Protection Contexts (PC)

Access is granted per bus-master Protection Context. The masks used in this
configuration map to:

| PC | Role (as used here) |
|---|---|
| PC1 | Boot / CM33 secure privileged (ROT) |
| PC2 | CM33 secure — TF-M SPE |
| PC5 | CM33 non-secure — NSPE |
| PC6 | CM55 non-secure — application core |
| PC7 | Auxiliary masters (DMA / debug / SE) |

`pcMask` examples from the PPC config: `0x86` = PC1+PC2+PC7 (secure domains);
`0xE6` = PC1+PC2+PC5+PC6+PC7 (shared secure+non-secure).

### 3.2 MPC memory domains (`unified_mpc_domains`)

Each domain lists the PCs allowed and the memory ranges it covers.

| Domain | Security / access (PC → attr) | Covered memory |
|---|---|---|
| **M33S** | PC2, PC7 → Secure RW | RRAM `bootloader_nvm`, RRAM `TFM_NV_COUNTERS`, QSPI `m33s_trailer`, SRAM `m33s_shared` + `m33s_data` |
| **M33_M55** | PC2, PC5, PC6, PC7 → NS RW | RRAM `user_nvm`, QSPI `m33_nvm`…`m55_upgrade_slot` block, all SOCMEM, SRAM `m33_code`/`m33_data`, RAMC1 |
| **M33S_CODE** | PC2, PC7 → Secure R | QSPI `m33s_nvm` (secure XIP code) |
| **TFM_SP_INITIAL_ATTESTATION** | PC2, PC7 → Secure RW | (SRSS peripherals via PPC) |
| **TFM_SP_CRYPTO** | PC2, PC7 → Secure RW | (Crypto peripheral via PPC) |
| **TFM_SP_ITS** | PC2, PC7 → Secure RW | RRAM `TFM_ITS` |
| **TFM_SP_PS** | PC2, PC7 → Secure RW | RRAM `TFM_PS` |
| **M33NSC** | PC2, PC5, PC6, PC7 → Secure R | (non-secure-callable veneer) |
| **M33** | PC2, PC5, PC7 → NS RW | (no explicit regions) |
| **M55** | PC2, PC6, PC7 → NS RW | (no explicit regions) |

On MPC violation the response is `CY_MPC_BUS_ERR` for the configured blocks.

### 3.3 PPC peripheral domains

**PPC0 (PERI0)** — `cycfg_ppc_0_domains_config`:

| Domain | Attr | Representative peripherals |
|---|---|---|
| **M33S** | Secure, privileged (`pcMask 0x86`) | SYSCPUSS, RAMC0/1 controllers + MPC regs, MXCM33 secure, IPC0 struct0, fault structs, SRSS HIB/power, DFT |
| **M33_M55** | Non-secure, non-priv (`pcMask 0xE6`) | GPIO/HSIOM (all ports), SCB0–11, TCPWM0, CANFD0, I3C, ETH0, DW0/DW1 DMA channels, IPC0 struct1–15, LPCOMP, NNLITE |
| **TFM_SP_INITIAL_ATTESTATION** | Secure, priv | SRSS general/main |
| **TFM_SP_CRYPTO** | Secure, priv | CRYPTO (main/boot/key/buf) |

**PPC1 (PERI1)** — `cycfg_ppc_1_domains_config`:

| Domain | Attr | Representative peripherals |
|---|---|---|
| **M33S** | Secure, privileged | M55 APPCPUSS boot, MXCM55 secure/boot, SMIF0/1 cache+AXI MPC config, SMIF GPIO ports, SOCMEM boot/MPC |
| **M33_M55** | Non-secure, non-priv | MXCM55 core, SAXI DMAC, PDM0, TDM0, SMIF0/1 core, U55 NPU, SOCMEM main, GFXSS (GPU/DC/MIPIDSI), SDHC0/1, USBHS, ITCM/DTCM |

On PPC violation the response is `CY_PPC_BUS_ERR`.

---

## 4. Summary

Effectively, the default memory allocation for PSE84_EVAL boils down to

- RRAM: bootloader and the TFM specific regions (ITS, PS, NV_COUNTERS) are marked SECURE, everything else (range between 0x39000 and 0x63000) is non-secure
- SRAM: up until offset 0x58000 is marked secure, afterwards is non-secure
- SMIF0MEM1: up until offset 0x340000 is marked as secure, afterwards is non-secure.

## 5. Relocation

With the strategy using the secure/non-secure protection split, the user can have the flexibility of moving the SRAM, RRAM and EXTFLASH memory regions with some limitations.
The non-secure images can be moved and resized freely as long as they are not moved before the cut-off boundary for SRAM and EXTFLASH.

Therefore users can freely update the kit_pse84_eval_memory_map.dtsi and or the per core files (e.g. kit_pse84_eval_m33_ns.dtsi and kit_pse84_eval_m55.dtsi) and in some instance reflect the changes in the generated files located in hal/infineon/zephyr-ifx-cycfg/kit_pse84_eval .
Here's a case by case description:

### 5.1 m55_xip
1. Update kit_pse84_eval_memory_map.dtsi entry for m55_xip (if moving earlier, resizing m33_xip is also needed)
2. Update kit_pse84_eval_m55.dtsi slot0_partition address
3. Rebuild cm55 image AND cm33ns image (it needs to know the new start address for cm55)

### 5.2 m33_xip
1. Update kit_pse84_eval_memory_map.dtsi entry for m33_xip (if resizing to make it bigger, moving m55_xip might be needed)
2. Update kit_pse84_eval_m33_ns.dtsi slot0_partition address
3. Update cymem_cm33_0.h file in hal/infineon/zephyr-ifx-cycfg/kit_pse84_eval with the new offset/address/size of m33_nvm defines.
4. Rebuild cm33ns image

### 5.3 m33_data
1. Update kit_pse84_eval_memory_map.dtsi entry for m33_data (neighboring regions could be impacted)
3. Update cymem_cm33_0.h file in hal/infineon/zephyr-ifx-cycfg/kit_pse84_eval with the new offset/address/size of m33_data defines.
4. Rebuild cm33ns image

### 5.4 ITS / PS / NV_COUNTERS
1. Update kit_pse84_eval_memory_map.dtsi entry for RRAM reserved_security address of where security regions start (neighboring regions could be impacted)
2. Update cymem_cm33_S_0.h file in hal/infineon/zephyr-ifx-cycfg/kit_pse84_eval with the new offset/address/size of the ITS/PS/NV_COUNTERS regions
3. Update the addresses/sizes of TFM_SP_xx_mpc_regions (xx = ITS, PS, NV_COUNTERS)
4. Update M33_M55_mpc_regions with the new size of the non-secure RRAM area
5. Update mxrramc_0_mpc_0_srf_protection_range_s with the new size of the non secure area and the new offset and size of the secure area.
6. Rebuild cm33ns image (it will trigger a rebuild of the TF-M SPE)

---

# PSE84 (KIT_PSE84_EVAL) Memory Map & Protection Domains in non TF-M case

The only source of truth for the non TF-M case, is kit_pse84_eval_memory_map.dtsi all details of the memory layout are already kept up to date in that file.

## 1. CM33s
When building a standalone application for the cm33s the memory map can be updated as desired without need to update any other files

## 2. CM55 in --sysbuild
When building the CM55 (with the --sysbuild configuration - which is needed cause the core that boots is always the cm33 and it then enables the cm55 one) the user can update the memory map but must take into consideration also the MPC configuration which is statically set in the cm33s minimal app that boots the cm55.

The mpc regions are defined in soc/infineon/edge/pse84/security_config/pse84_s_protection.c.

### 2.1 Protection state in `pse84_s_protection.c`

Unlike the TF-M case, this minimal secure boot app applies a **much simpler, mostly
non-secure** protection setup. It configures only the MPC (memory) domains and
effectively **opens all peripherals** via the PPC.

**MPC memory domains** (`unified_mpc_domains`). Offsets are relative to each memory
base: RRAM `0x22000000`, SRAM/RAMC0 `0x24000000`, RAMC1 `0x24080000`,
SOCMEM `0x26000000`, QSPI/SMIF0 `0x60000000`.

| Domain | Security / access | Regions (base @ offset, size → absolute) |
|---|---|---|
| **m33s** | PC2, PC7 → Secure RW | RRAM `@0x11000`, `0x4A000` (bootloader + secure)<br>SMIF0 `@0x100000`, `0x200000` (m33s XIP code, 2 MB)<br>RAMC0 `@0x1000`, `0x57000` (m33s SRAM) |
| **m33** | PC2, PC5, PC7 → NS RW | SMIF0 `@0x300000`, `0x240000` (m33 NS code)<br>RAMC0 `@0x58000`, `0x28000` (m33 NS SRAM)<br>RAMC1 `@0x0`, `0x3D000` |
| **m55** | PC2, PC6, PC7 → NS RW | SMIF0 `@0x500000`, `0x300000` (m55 XIP code, 3 MB)<br>SOCMEM `@0x0`, `0x40000` → `0x26000000` (m55 code secondary) |
| **m33_m55** | PC2, PC5, PC6, PC7 → NS RW | RRAM `@0x5B000`, `0x8000` <br>SOCMEM `@0x40000`, `0x4C0000` → `0x26040000` (m55 data + shared)<br>RAMC1 `@0x3D000`, `0x43000` → `0x240BD000` |

MPC violation response is `CY_MPC_BUS_ERR` on all memories.

The CM55 therefore runs in PC6, non-secure, and can only reach memory covered by the
**m55** and **m33_m55** MPC domains — anything outside those ranges triggers a bus
fault.

### 2.2 Steps to relocate / resize CM55 regions

The CM55 footprint is described by the **m55** domain (its own XIP code in SMIF0 +
code-secondary in SOCMEM) and the **m33_m55** domain (its data/shared in SOCMEM).
Both are static in `pse84_s_protection.c`, so any dtsi change must be mirrored
there or the CM55 will fault on first access.

1. Update `kit_pse84_eval_memory_map.dtsi` with the new address/size of the CM55
   region(s), and `kit_pse84_eval_m55.dtsi` (`slot0_partition`) for the XIP code slot.
   Adjust neighboring `m33` regions if the move creates a gap/overlap.
2. In `pse84_s_protection.c`, update `m55_mpc_regions`:
   - `SMIF0_CACHE_BLOCK_CACHEBLK_AHB_MPC0` and `SMIF0_CORE_AXI_MPC0` `offset`/`size`
     for the CM55 XIP code (both entries must match).
   - `SOCMEM_SRAM_MPC0` `offset`/`size` for the CM55 code-secondary.
3. If the CM55 **data / shared** SOCMEM window moves or resizes, update the
   `SOCMEM_SRAM_MPC0` entry in `m33_m55_mpc_regions` (and the RAMC1 entry if RAM is
   affected) so the shared area stays contiguous with `m55_mpc_regions`.
4. Rebuild the cm55 app with sysbuild (it will trigger the cm33s rebuild with updated MPC settings)

