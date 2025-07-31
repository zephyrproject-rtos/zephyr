# SD/MMC Card Sample

This sample demonstrates SD/MMC card functionality on the LPC54S018, including:
- Card detection and initialization
- FAT file system mounting
- File read/write operations
- Directory operations
- Disk information retrieval
- Hot-plug support

## Requirements

- LPC54S018 board with SD card slot
- MicroSD card (FAT32 formatted recommended)
- SD card adapter/socket connected to the following pins:
  - P2.16: SD_CMD
  - P2.17: SD_DAT0
  - P2.18: SD_DAT1
  - P2.19: SD_DAT2
  - P2.20: SD_DAT3
  - P2.21: SD_CLK
  - P2.23: SD_CARD_DETECT (optional)
  - P2.24: SD_WRITE_PROTECT (optional)

## Wiring

Since the LPCXpresso54S018M board doesn't have a built-in SD card slot, you'll need to connect an external SD card breakout board:

```
SD Card Module    LPC54S018 Pin
--------------    -------------
VCC (3.3V)    ->  3.3V
GND           ->  GND
CMD           ->  P2.16
CLK           ->  P2.21
DAT0          ->  P2.17
DAT1          ->  P2.18 (for 4-bit mode)
DAT2          ->  P2.19 (for 4-bit mode)
DAT3/CS       ->  P2.20 (for 4-bit mode)
CD            ->  P2.23 (optional)
WP            ->  P2.24 (optional)
```

## Building and Running

```bash
cd samples/drivers/sdmmc
west build -b lpcxpresso54s018
west flash
```

## Expected Output

```
*** Booting Zephyr OS build v4.2.0 ***
[00:00:00.000,000] <inf> sdmmc_sample: SD/MMC Sample Application
[00:00:00.000,000] <inf> sdmmc_sample: Insert an SD card to begin...
[00:00:01.000,000] <inf> sdmmc_sample: SD card detected!
[00:00:01.100,000] <inf> sdmmc_sample: Mounting file system...
[00:00:01.200,000] <inf> sdmmc_sample: File system mounted successfully!
[00:00:01.210,000] <inf> sdmmc_sample: Disk info:
[00:00:01.210,000] <inf> sdmmc_sample:   Block size: 512 bytes
[00:00:01.210,000] <inf> sdmmc_sample:   Total blocks: 7864320
[00:00:01.210,000] <inf> sdmmc_sample:   Free blocks: 7864000
[00:00:01.210,000] <inf> sdmmc_sample:   Total size: 3840 MB
[00:00:01.210,000] <inf> sdmmc_sample:   Free size: 3839 MB
[00:00:01.210,000] <inf> sdmmc_sample:   Used: 0%

=== Testing file operations ===
[00:00:01.220,000] <inf> sdmmc_sample: Creating file: /SD:/test.txt
[00:00:01.230,000] <inf> sdmmc_sample: Writing data to file...
[00:00:01.240,000] <inf> sdmmc_sample: Wrote 65 bytes
[00:00:01.250,000] <inf> sdmmc_sample: Reading file: /SD:/test.txt
[00:00:01.260,000] <inf> sdmmc_sample: Read 65 bytes: Hello from LPC54S018!
This is a test of SD card functionality.

=== Testing directory operations ===
[00:00:01.270,000] <inf> sdmmc_sample: Creating directory: /SD:/testdir
[00:00:01.280,000] <inf> sdmmc_sample: Listing root directory:
[00:00:01.290,000] <inf> sdmmc_sample:   test.txt (size: 65)
[00:00:01.290,000] <inf> sdmmc_sample:   testdir/ (size: 0)

=== SD card operations complete ===
[00:00:01.300,000] <inf> sdmmc_sample: You can now safely remove the SD card
```

## Features Demonstrated

1. **Card Detection**: Waits for SD card insertion
2. **File System Mount**: Mounts FAT file system
3. **File Operations**: Create, write, and read files
4. **Directory Operations**: Create directories and list contents
5. **Disk Information**: Shows capacity and usage
6. **Hot-Plug Support**: Detects card removal and reinsertion

## Notes

- The sample uses 4-bit SD mode for better performance
- Card detect (CD) and write protect (WP) pins are optional
- The file system is FAT32 by default
- Maximum file name length depends on LFN configuration
- The sample creates test files that persist on the card

## Troubleshooting

1. **Card not detected**: 
   - Check wiring connections
   - Ensure card is properly formatted (FAT32)
   - Verify 3.3V power supply

2. **Mount fails**:
   - Card may need reformatting
   - Check if card is write-protected

3. **Slow performance**:
   - Ensure 4-bit mode is enabled
   - Check SD clock frequency in device tree