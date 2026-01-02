# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# RISC-V AIA (APLIC + IMSIC) Driver Tests

This test suite validates the RISC-V Advanced Interrupt Architecture (AIA) drivers,
including the APLIC (Advanced Platform-Level Interrupt Controller) and IMSIC
(Incoming Message-Signaled Interrupt Controller).

## Test Coverage

### APLIC Tests

1. **Register Offset Calculations**
   - `test_aplic_sourcecfg_offset`: Validates SOURCECFG register offset calculation
   - `test_aplic_target_offset`: Validates TARGET register offset calculation

2. **Register Address Constants**
   - `test_aplic_register_addresses`: Verifies critical APLIC register offsets per AIA spec

3. **Configuration Bits and Modes**
   - `test_aplic_domaincfg_bits`: Tests DOMAINCFG register bit field definitions
   - `test_aplic_source_modes`: Validates source mode constants (inactive, edge, level, etc.)

4. **MSI Routing Encoding**
   - `test_aplic_target_encoding`: Tests TARGET register field encoding (hart, MSI mode, EIID)
   - `test_aplic_genmsi_encoding`: Tests GENMSI register field encoding for software-triggered MSI
   - `test_aplic_msi_geometry_fields`: Validates MSIADDRCFGH geometry field encoding

### IMSIC Tests

1. **CSR Address Definitions**
   - `test_imsic_csr_addresses`: Validates direct and indirect CSR addresses

2. **Register Field Decoding**
   - `test_imsic_mtopei_fields`: Tests MTOPEI register field masks (EIID, priority)
   - `test_imsic_eidelivery_modes`: Validates EIDELIVERY mode constants (MMSI, DMSI, DDI)

3. **EIE Register Indexing**
   - `test_imsic_eie_indexing`: Tests EIID to EIE register mapping (8 registers, 32 IDs each)
   - `test_imsic_eie_bit_operations`: Validates bit manipulation for enabling/disabling EIIDs
   - `test_imsic_indirect_csr_addressing`: Tests CSR address calculation for indirect access

### Integration Tests

1. **MSI Routing**
   - `test_aia_msi_routing_encoding`: Tests APLIC TARGET + IMSIC EIE encoding for complete MSI route

2. **Boundary Conditions**
   - `test_eiid_range_boundaries`: Tests EIID encoding from 0 to 2047 (11-bit)
   - `test_hart_index_boundaries`: Tests hart index encoding from 0 to 16383 (14-bit)

## Test Pattern

Following the existing PLIC test pattern at `tests/drivers/interrupt_controller/intc_plic/`,
these tests validate:
- Register offset helper functions
- Bit field definitions and masks
- Encoding/decoding of hardware register values
- Boundary conditions and overflow handling

The tests do NOT require actual AIA hardware - they test the driver API constants,
helper functions, and register encoding logic.

## Running the Tests

```bash
# Build and run tests
west build -p -b qemu_riscv64 tests/drivers/interrupt_controller/intc_riscv_aia
west build -t run
```

## Test Results

All 17 tests pass successfully:

```
SUITE PASS - 100.00% [intc_riscv_aia]: pass = 17, fail = 0, skip = 0, total = 17
```

## Architecture

These tests validate the AIA driver implementation consisting of:
- **APLIC driver** (`drivers/interrupt_controller/intc_riscv_aplic_msi.c`)
- **IMSIC driver** (`drivers/interrupt_controller/intc_riscv_imsic.c`)
- **Unified AIA coordinator** (`drivers/interrupt_controller/intc_riscv_aia.c`)

The tests ensure correct encoding of:
- MSI routing (APLIC → IMSIC)
- Interrupt enable/disable operations
- SMP hart targeting
- Software-triggered MSI injection (GENMSI)
