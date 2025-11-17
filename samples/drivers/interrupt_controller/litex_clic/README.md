# LiteX CLIC Test Application

## Overview

This sample application demonstrates and tests the LiteX CLIC (Core Local Interrupt Controller) driver functionality.

## Requirements

- LiteX SoC with VexRiscv CPU
- CLIC hardware module integrated
- Zephyr RTOS with LiteX support

## Building and Running

### For LiteX VexRiscv SOC
```bash
west build -b litex_vexriscv samples/drivers/interrupt_controller/litex_clic -DDTC_OVERLAY_FILE=/<path>/vexriscv.dts
```

## Test Coverage

The application tests the following functionality:

1. **Interrupt Enable/Disable**
   - Verifies arch_irq_enable() and arch_irq_disable()
   - Checks arch_irq_is_enabled() status reporting

2. **Interrupt Priority**
   - Configures different priority levels
   - Connects ISRs with varying priorities
   - Verifies priority configuration (actual arbitration not tested)

3. **CSR Register Access**
   - Tests read/write cycles to CSR registers
   - Verifies register value persistence

## Configuration

### prj.conf Options

```conf
# Enable LiteX CLIC driver
CONFIG_LITEX_CLIC=y

# Enable debug features
CONFIG_LITEX_CLIC_DEBUG_REGISTERS=y
CONFIG_LITEX_CLIC_DIRECT_API=y
CONFIG_LITEX_CLIC_LOG_LEVEL=4

# Disable nested interrupts (safer)
CONFIG_LITEX_CLIC_NESTED_INTERRUPTS=n
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Build fails | Ensure LiteX CLIC driver is enabled in Kconfig |
| No output | Check serial console configuration |
| Tests fail | Verify CLIC hardware is present Litex Vexriscv SOC |

## Further Information

- See [CLIC Litex Support](https://github.com/enjoy-digital/litex/pull/2260/) for Hardware support details
