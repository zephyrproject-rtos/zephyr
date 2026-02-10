# AD74115H Software Configurable I/O Sample

## Overview
This sample demonstrates how to use the AD74115H driver in Zephyr to perform ADC voltage sensing.

## Hardware Requirements
- AD74115H IC or Evaluation Board.
- A Zephyr-supported board with SPI (e.g., nRF52840DK).
- Correct wiring of the `ADC_RDY` pin is required for interrupt-driven sensing.

## Wiring
| AD74115H Pin | Function |
| :--- | :--- |
| SCLK | SPI Clock |
| SDI | SPI MOSI |
| SDO | SPI MISO |
| SYNC | SPI CS |
| ADC_RDY | GPIO Interrupt |

## Building
```bash
west build -b <your_board> samples/sensor/ad74115h
west flash
