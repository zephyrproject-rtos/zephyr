This `README.md` is designed to be placed in `samples/drivers/ad74416h/README.md`. It follows the standard Zephyr documentation format required for a Pull Request.

---

# AD74416H Multi-Function I/O Sample

## Overview

This sample application demonstrates the use of the **Analog Devices AD74416H** Quad-Channel Software Configurable I/O device. The driver exposes the device's high level of integration by mapping its channels to three different Zephyr subsystems simultaneously:

1.  **DAC API**: Channel 0 is configured as a Voltage Output (0-12V).
2.  **Sensor API (ADC)**: Channel 1 is configured as a Voltage Input (0-12V).
3.  **Sensor API (RTD)**: Channel 2 is configured for RTD Temperature measurement (Celsius).
4.  **GPIO API**: Channel 3 is configured as a Digital Input.

The sample also demonstrates the driver's internal handling of **CRC-8 error detection**, **40-bit SPI frames**, and **Callendar-Van Dusen RTD math**.

## Requirements

*   A board with SPI support.
*   AD74416H Software Configurable I/O IC (or EV-AD74416H-ARDZ evaluation board).
*   External connections:
    *   A Pt1000 RTD sensor connected to Channel 2.
    *   A voltage source (0-12V) or a jumper from CH0 to CH1 to test loopback.
    *   A switch or push-button connected to Channel 3.

## Wiring

The following pins must be defined in your Devicetree overlay:

| AD74416H Pin | Function | Description |
| :--- | :--- | :--- |
| SCLK | SPI SCK | Serial Clock |
| SDI | SPI MOSI | Master Out Slave In |
| SDO | SPI MISO | Master In Slave Out |
| SYNC | SPI CS | Chip Select (Active Low) |
| RESET | GPIO | Hardware Reset (Active Low) |
| ADC_RDY | GPIO | Data Ready Interrupt |

## Building and Running

Build the sample for your specific board (e.g., `nrf52840dk_nrf52840`):

```bash
west build -b nrf52840dk_nrf52840 samples/drivers/ad74416h
west flash
```

## Sample Output

Upon startup, the driver verifies the Silicon Revision (0x0002) and initializes the hardware watchdog. You will see the following output in the terminal:

```text
*** Booting Zephyr OS build v3.x.x ***
AD74416H Multi-Function Sample Started
--- AD74416H Status ---
CH0 (Out): 5.00 V
CH1 (In):  4.998234 V
CH2 (RTD): 24.52 C
CH3 (DIN): HIGH

--- AD74416H Status ---
CH0 (Out): 5.00 V
CH1 (In):  4.998112 V
CH2 (RTD): 24.55 C
CH3 (DIN): LOW
```

## Driver Features Demonstrated

### Adaptive Power Switching
The sample enables `analog,adaptive-power` in the Devicetree. This allows the driver to configure the IC to dynamically switch between `AVDD_HI` and `AVDD_LO` rails, reducing power consumption and heat dissipation by up to 40% in industrial modules.

### Industrial Reliability
The driver performs a **CRC-8 check** on every 40-bit SPI transaction. If an SPI bit-flip occurs due to industrial EMI, the driver will catch the error, log a warning, and discard the corrupted data.

### Precise Temperature Sensing
For Channel 2, the driver utilizes the on-chip 2.5V reference and internal 2kΩ precision resistors to perform a ratiometric measurement. It then applies the **Callendar-Van Dusen equation** to provide a high-accuracy Celsius reading directly to the application.

### Interrupt-Driven Data Fetch
Instead of blocking the CPU with `k_sleep()`, the driver utilizes the physical `ADC_RDY` pin. The `sensor_sample_fetch()` call triggers the ADC sequencer and puts the calling thread to sleep until the hardware interrupt signals that data is ready.
