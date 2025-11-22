# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# UART Echo Demo - APLIC MSI Delivery

This demo demonstrates the complete RISC-V AIA interrupt flow using UART as the interrupt source.

## Interrupt Flow

```
UART Hardware Interrupt
         ↓
APLIC Source 10 (edge-triggered)
         ↓
MSI Write to IMSIC
         ↓
IMSIC Interrupt File
         ↓
CPU MEXT (Machine External Interrupt)
         ↓
ISR echoes received characters
```

## What This Demo Shows

1. **APLIC Configuration**: Configures APLIC source mode and routing
2. **MSI Routing**: Routes UART interrupt via MSI to a specific EIID
3. **IMSIC Delivery**: IMSIC delivers MSI as external interrupt to CPU
4. **Manual UART Handling**: Direct 16550 UART register access for clarity
5. **GENMSI Testing**: Software-generated MSI injection to verify ISR path

## Building and Running

### Build
```bash
export ZEPHYR_SDK_INSTALL_DIR="/home/afonsoo/zephyr-sdk-0.17.2"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
west build -p -b qemu_riscv32_aia samples/uart_echo_aia
```

### Run Interactively
```bash
~/qemu-aia/build/qemu-system-riscv32 \
  -nographic \
  -machine virt,aia=aplic-imsic \
  -bios none \
  -m 256 \
  -kernel build/zephyr/zephyr.elf
```

Type characters and press Enter to see them echoed back.
Press Ctrl+A then X to exit QEMU.

### Automated Test
```bash
(echo "Hello"; echo "Test123"; sleep 2) | \
  timeout 5 ~/qemu-aia/build/qemu-system-riscv32 \
  -nographic -machine virt,aia=aplic-imsic \
  -bios none -m 256 -kernel build/zephyr/zephyr.elf
```

## Expected Output

```
╔════════════════════════════════════════════════╗
║   UART Echo Demo - APLIC MSI Delivery        ║
║   Manual UART Register Handling              ║
╚════════════════════════════════════════════════╝

✓ APLIC device ready
✓ UART base address: 0x10000000

Configuration:
  UART IRQ number (APLIC source): 10
  Target EIID: 32

Step 1: Registering ISR for EIID 32
  ✓ ISR registered and enabled
Step 2: Configuring APLIC source 10
  ✓ Source configured as edge-triggered
Step 3: Routing APLIC source 10 → hart:0 eiid:32
  ✓ Route configured
Step 4: Enabling APLIC source 10
  ✓ Source enabled
Step 5: Enabling UART RX interrupts (IER)
  ✓ UART interrupts enabled (IER = 0x01)
Step 6: Testing ISR with GENMSI injection
  Injecting software interrupt to EIID 32...
  ✓ GENMSI successfully triggered ISR!

═══════════════════════════════════════════════
  Setup complete! Type characters to echo.
  Interrupt flow: UART → APLIC → MSI → IMSIC
═══════════════════════════════════════════════

Hello
[Status] ISR entries: 2, Characters echoed: 6

Test123
[Status] ISR entries: 3, Characters echoed: 14
```

## Key Configuration

- **UART IRQ**: APLIC source 10 (QEMU virt platform UART0)
- **EIID**: 32 (first available EIID after platform-reserved EIIDs)
- **Source Mode**: Edge-triggered rising edge
- **Target**: Hart 0, Guest 0
- **UART Base**: 0x10000000 (QEMU virt UART0)

## Implementation Details

### APLIC Configuration
```c
/* Configure source mode */
riscv_aplic_msi_config_src(aplic, UART_IRQ_NUM, APLIC_SM_EDGE_RISE);

/* Route to hart 0, EIID 32 */
riscv_aplic_msi_route(aplic, UART_IRQ_NUM, 0, UART_EIID);

/* Enable source */
riscv_aplic_enable_source(UART_IRQ_NUM);
```

### ISR Registration
```c
IRQ_CONNECT(UART_EIID, 1, uart_isr, NULL, 0);
irq_enable(UART_EIID);
```

### UART Setup
```c
/* Enable RX interrupts via IER register */
uart_write_reg(UART_IER, UART_IER_RDI);
```

### ISR Handler
```c
static void uart_isr(const void *arg)
{
    /* Check if data is available */
    uint8_t lsr = uart_read_reg(UART_LSR);
    while (lsr & UART_LSR_DR) {
        /* Read and echo character */
        uint8_t c = uart_read_reg(UART_RBR);
        uart_write_reg(UART_THR, c);
        char_count++;

        /* Check for more data */
        lsr = uart_read_reg(UART_LSR);
    }
}
```

## Verification

The demo verifies correct operation through:

1. **GENMSI Test**: Software MSI injection confirms ISR can be triggered
2. **Status Messages**: Periodic reports show ISR entry count and character count
3. **Echo Functionality**: Typed characters are immediately echoed back
4. **IMSIC Logs**: Debug logs show IMSIC claiming EIID 32 on each interrupt

## Platform Requirements

- **Board**: qemu_riscv32_aia (QEMU virt with AIA support)
- **QEMU**: Build with AIA support (`virt,aia=aplic-imsic`)
- **Zephyr**: AIA drivers enabled (APLIC-MSI + IMSIC)

## References

- RISC-V Advanced Interrupt Architecture Specification
- APLIC MSI Mode Documentation
- IMSIC Specification
- 16550 UART Register Documentation
