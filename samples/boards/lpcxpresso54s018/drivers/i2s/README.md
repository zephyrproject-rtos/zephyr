# I2S Audio Sample

This sample demonstrates I2S audio interface on the LPC54S018 using FLEXCOMM6
configured as I2S.

## Overview

The sample:
- Configures FLEXCOMM6 as I2S transmitter
- Generates a 1kHz sine wave test tone
- Outputs stereo audio at 44.1kHz sample rate
- Uses DMA for efficient audio streaming
- Optionally configures an audio codec via I2C

## Hardware Requirements

- LPCXpresso54S018M board
- I2S DAC or audio amplifier module
- Audio codec (optional)
- Speakers or headphones

## Pin Configuration

### I2S0 (FLEXCOMM6) Pins:
- P1.22: I2S SCLK (Bit Clock)
- P2.30: I2S WS (Word Select/LR Clock)
- P2.28: I2S TX_DATA (Audio Data Out)
- P2.29: I2S RX_DATA (Audio Data In - not used)
- P1.31: MCLK (Master Clock - if needed by DAC)

### Alternate I2S1 (FLEXCOMM7) Pins:
- P1.28: I2S SCLK
- P1.29: I2S DATA
- P1.30: I2S WS

## Building and Running

### Build the sample:
```bash
# From project root
west build -b lpcxpresso54s018 samples/drivers/i2s -p auto
```

### Flash to device:
```bash
west flash
```

## Connecting I2S Devices

### Simple I2S DAC (e.g., PCM5102A):
```
LPC54S018    ->  PCM5102A
---------        ---------
P1.22 (SCLK) ->  BCK
P2.30 (WS)   ->  LRCK
P2.28 (DATA) ->  DIN
GND          ->  GND
3.3V         ->  VIN
```

### I2S Amplifier (e.g., MAX98357A):
```
LPC54S018    ->  MAX98357A
---------        ----------
P1.22 (SCLK) ->  BCLK
P2.30 (WS)   ->  LRC
P2.28 (DATA) ->  DIN
GND          ->  GND
3.3V         ->  VIN
GPIO (any)   ->  SD (shutdown, pull high to enable)
```

## Configuration Options

### Sample Rate
Change in main.c:
```c
#define SAMPLE_RATE 44100  /* Options: 8000, 16000, 44100, 48000 */
```

### Audio Format
```c
#define CHANNELS        2   /* 1 for mono, 2 for stereo */
#define BITS_PER_SAMPLE 16  /* 16 or 24 bits */
```

### Test Tone Frequency
```c
#define TONE_FREQUENCY 1000  /* Frequency in Hz */
```

## LED Status

- Blinking: Audio data is being transmitted

## Audio Codecs

To use an external audio codec:

1. Connect codec via I2C (FLEXCOMM4)
2. Modify `configure_audio_codec()` function with codec-specific registers
3. Common codec examples:
   - WM8960: I2C address 0x1A
   - WM8904: I2C address 0x1A
   - SGTL5000: I2C address 0x0A

## Troubleshooting

1. **No audio output**:
   - Check I2S connections
   - Verify DAC/amplifier power
   - Ensure correct pin configuration
   - Check if DAC needs MCLK signal

2. **Distorted audio**:
   - Verify sample rate matches DAC expectations
   - Check bit clock frequency
   - Ensure proper grounding

3. **I2S configuration fails**:
   - Check if FLEXCOMM6 is available
   - Verify DMA channels are free
   - Check clock configuration

## Advanced Usage

### Recording Audio (I2S RX)
```c
/* Configure for receive */
i2s_configure(i2s_dev_rx, I2S_DIR_RX, &i2s_cfg);
i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_START);

/* Read audio data */
i2s_read(i2s_dev_rx, &rx_block, &rx_size);
```

### Full Duplex Operation
Configure both TX and RX directions for simultaneous playback and recording.

### Using FLEXCOMM7 as I2S
Change device tree to enable i2s1 instead of i2s0:
```dts
&i2s1 {
    status = "okay";
    pinctrl-0 = <&pinmux_flexcomm7_i2s>;
    pinctrl-names = "default";
};
```

## Notes

- FLEXCOMM6 and FLEXCOMM7 support I2S mode
- DMA is used for efficient audio streaming
- The sample generates a continuous sine wave
- Audio quality depends on DAC/amplifier used
- MCLK may be required by some DACs (typically 256 × sample rate)