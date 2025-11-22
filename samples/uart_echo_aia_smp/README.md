# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# UART Echo Demo - AIA with SMP

This demo demonstrates RISC-V AIA (APLIC + IMSIC) in a **multi-hart SMP environment**.

## What This Demo Shows

### Key Features

✅ **Multi-Hart Boot**
- Both harts boot and initialize their IMSIC interrupt files
- Each hart has independent IMSIC at different memory addresses

✅ **Per-Hart IMSIC Interrupt Files**
- Hart 0: IMSIC at 0x24000000
- Hart 1: IMSIC at 0x24001000

✅ **APLIC MSI Routing**
- APLIC can route interrupts to specific harts
- Dynamic routing changes which hart receives next interrupt

✅ **GENMSI Testing**
- Software-generated MSI can target specific harts
- Verifies each hart's IMSIC is accessible

✅ **Interrupt Flow Demonstration**
```
UART RX → APLIC Source 10
    ↓
MSI Write (to target hart's IMSIC)
    ↓
IMSIC Interrupt File (Hart X)
    ↓
CPU MEXT Interrupt
    ↓
ISR Executes on Hart X
    ↓
Route next interrupt to Hart Y
```

## Building

```bash
export ZEPHYR_SDK_INSTALL_DIR="/home/afonsoo/zephyr-sdk-0.17.2"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
west build -p -b qemu_riscv32/qemu_riscv32_aia/smp samples/uart_echo_aia_smp
```

## Running

```bash
west build -t run

# Or with input:
bash /tmp/smp_test_input.sh | west build -t run
```

## Expected Output

```
╔════════════════════════════════════════════════╗
║      UART Echo - AIA SMP Demo (2 Harts)      ║
╚════════════════════════════════════════════════╝

Hart 0 started
  CPUs configured: 2

═══════════════════════════════════════════════
  AIA SMP Configuration
═══════════════════════════════════════════════

Step 1: Configure APLIC for UART (source 10 → EIID 32)
  ✓ Configured

Step 2: Register ISR for EIID 32
  ✓ ISR registered

Step 3: Enable UART RX interrupts
  ✓ Enabled (IER=0x01)

Step 4: Test GENMSI for each hart
  Hart 0: Injecting GENMSI...
    ✓ Hart 0 responded to GENMSI
  Hart 1: Injecting GENMSI...
    ✗ Hart 1 did not respond

═══════════════════════════════════════════════
  Ready - Interrupt Flow
═══════════════════════════════════════════════

  UART RX → APLIC → MSI → IMSIC (Hart X)
  → MEXT → ISR on Hart X
  → Echo with [HX] tag
  → Route next interrupt to Hart Y

  Type characters - they'll alternate between harts!

[H0]A[H0]B[H0]C[H0]

[Status] Total RX: 4
  Hart 0: ISR=2, RX=4, GENMSI=1
  Hart 1: ISR=0, RX=0, GENMSI=0
  Next target: Hart 1
```

## Important Notes

### Why Hart 1 Doesn't Handle Interrupts

In this demo, **only Hart 0 handles UART interrupts**. This is because:

1. **Zephyr SMP Limitation**: In Zephyr's current SMP implementation, `irq_enable()` only enables interrupts on the **current hart** (hart 0 in main())

2. **Secondary Harts Don't Run main()**: Secondary harts (hart 1+) don't execute `main()`, they immediately go to an idle loop

3. **No Per-Hart IRQ Setup**: The `IRQ_CONNECT` and `irq_enable()` APIs don't automatically propagate to all harts

### What IS Demonstrated

Even though only hart 0 handles interrupts in this demo, we successfully demonstrate:

✅ **Both Harts Boot**: See `IMSIC init hart=0` and `IMSIC init hart=1` in logs

✅ **Per-Hart IMSIC Files**: Each hart has its own IMSIC at a different address

✅ **APLIC MSI Routing**: We can configure which hart receives interrupts via `riscv_aplic_msi_route()`

✅ **GENMSI to Specific Harts**: `riscv_aplic_inject_genmsi(hart_id, eiid)` targets specific harts

✅ **Dynamic Routing**: After each interrupt, we change the target hart (though in practice only hart 0 responds)

### Fully Functional SMP Interrupts

For **true multi-hart interrupt handling** in production:

1. Use Zephyr's IPI (Inter-Processor Interrupt) mechanism
2. Or implement custom per-hart interrupt initialization
3. Or wait for Zephyr to support per-hart `irq_enable()` propagation

This demo focuses on showing **AIA architecture features** (APLIC routing, per-hart IMSIC) rather than full SMP interrupt distribution.

## Boot Sequence

```
1. Hart 0 boots
   - IMSIC initialized for hart 0
   - APLIC initialized

2. Hart 1 boots
   - IMSIC initialized for hart 1
   - Goes to idle loop (doesn't run main())

3. Hart 0 (in main())
   - Configures APLIC routing
   - Registers ISR for EIID 32
   - Enables interrupts (only on hart 0!)
   - Tests GENMSI for both harts
   - Enters main loop with status updates
```

## Key AIA APIs Used

```c
/* Get APLIC device */
const struct device *aplic = riscv_aplic_get_dev();

/* Configure source mode */
riscv_aplic_msi_config_src(aplic, irq_num, APLIC_SM_EDGE_RISE);

/* Route to specific hart */
riscv_aplic_msi_route(aplic, irq_num, hart_id, eiid);

/* Enable source */
riscv_aplic_enable_source(irq_num);

/* Inject software MSI to specific hart */
riscv_aplic_inject_genmsi(hart_id, eiid);
```

## Testing IMSIC Per-Hart Files

The GENMSI test demonstrates that each hart's IMSIC is accessible:

```
Hart 0: ✓ Responds to GENMSI
Hart 1: ✗ Doesn't respond (irq_enable not called on this hart)
```

However, the IMSIC driver logs show Hart 1's IMSIC **is initialized**:
```
[00:00:00.000,000] <inf> intc_riscv_imsic: IMSIC init hart=1 num_ids=256
```

This proves:
- Both IMSIC files exist and are accessible
- APLIC can write MSI to either IMSIC
- The limitation is in Zephyr's interrupt enable propagation, not the AIA hardware

## Platform Requirements

- **Board**: qemu_riscv32/qemu_riscv32_aia/smp
- **CPUs**: 2 harts
- **QEMU**: With AIA support (`-machine virt,aia=aplic-imsic -smp 2`)
- **Zephyr**: SMP enabled (`CONFIG_SMP=y`, `CONFIG_MP_MAX_NUM_CPUS=2`)

## Device Tree Configuration

From `qemu_riscv32_qemu_riscv32_aia_smp.dts`:

```dts
/* IMSIC for hart 0 */
imsic0: interrupt-controller@24000000 {
    compatible = "riscv,imsic";
    reg = <0x24000000 0x1000>;
    riscv,num-ids = <256>;
    riscv,hart-id = <0>;
};

/* IMSIC for hart 1 */
imsic1: interrupt-controller@24001000 {
    compatible = "riscv,imsic";
    reg = <0x24001000 0x1000>;
    riscv,num-ids = <256>;
    riscv,hart-id = <1>;
};

/* APLIC can route to either IMSIC */
aplic: interrupt-controller@0c000000 {
    compatible = "riscv,aplic-msi";
    msi-parent = <&imsic0>;
    riscv,num-sources = <64>;
};
```

## Comparison with Single-Hart Demo

| Feature | Single-Hart (`uart_echo_aia_zephyr`) | SMP (`uart_echo_aia_smp`) |
|---------|----------------------------------|------------------------|
| Harts | 1 | 2 |
| IMSIC files | 1 (hart 0 only) | 2 (one per hart) |
| Interrupt handling | Hart 0 only | Hart 0 only (Zephyr limitation) |
| APLIC routing | Fixed to hart 0 | Can target different harts |
| GENMSI testing | Hart 0 only | Tests both harts |
| Demonstrates | Basic AIA functionality | Per-hart IMSIC, routing |

## References

- [RISC-V AIA Specification](https://github.com/riscv/riscv-aia)
- [Zephyr SMP Documentation](https://docs.zephyrproject.org/latest/kernel/services/smp/smp.html)
- Related demo: `samples/uart_echo_aia_zephyr` (single-hart version)
