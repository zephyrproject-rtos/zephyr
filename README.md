# LPC54S018 Port - Required Files List

**Private Porting by David Hor - Xtooltech 2025**  
**Contact: david.hor@xtooltech.com**

This is the complete list of ALL files required to make the LPC54S018 port work on another computer with fresh Zephyr 4.2 installation.

## Target Installation Paths

All paths are relative to `$ZEPHYR_BASE` (your Zephyr installation directory).

---

## 1. Board Support Files

**Target Directory:** `$ZEPHYR_BASE/boards/nxp/lpcxpresso54s018/`

```
boards/nxp/lpcxpresso54s018/board.c
boards/nxp/lpcxpresso54s018/board.cmake
boards/nxp/lpcxpresso54s018/board.yml
boards/nxp/lpcxpresso54s018/CMakeLists.txt
boards/nxp/lpcxpresso54s018/Kconfig.board
boards/nxp/lpcxpresso54s018/Kconfig.defconfig
boards/nxp/lpcxpresso54s018/lpc54s018.ld
boards/nxp/lpcxpresso54s018/lpcxpresso54s018.dts
boards/nxp/lpcxpresso54s018/lpcxpresso54s018.dtsi
boards/nxp/lpcxpresso54s018/lpcxpresso54s018-pinctrl.dtsi
boards/nxp/lpcxpresso54s018/lpcxpresso54s018_defconfig
boards/nxp/lpcxpresso54s018/lpcxpresso54s018_secure.overlay
boards/nxp/lpcxpresso54s018/lpcxpresso54s018_sdram.overlay
boards/nxp/lpcxpresso54s018/lpcxpresso54s018_usb.overlay
boards/nxp/lpcxpresso54s018/pre_dt_board.cmake
boards/nxp/lpcxpresso54s018/revision.cmake
```

---

## 2. SoC Support Files

**Target Directory:** `$ZEPHYR_BASE/soc/nxp/lpc/lpc54xxx/lpc54s018/`

```
soc/nxp/lpc/lpc54xxx/lpc54s018/CMakeLists.txt
soc/nxp/lpc/lpc54xxx/lpc54s018/Kconfig.defconfig
soc/nxp/lpc/lpc54xxx/lpc54s018/Kconfig.soc
soc/nxp/lpc/lpc54xxx/lpc54s018/linker.ld
soc/nxp/lpc/lpc54xxx/lpc54s018/pinctrl_soc.h
soc/nxp/lpc/lpc54xxx/lpc54s018/soc.c
soc/nxp/lpc/lpc54xxx/lpc54s018/soc.h
soc/nxp/lpc/lpc54xxx/lpc54s018/soc_minimal.c
soc/nxp/lpc/lpc54xxx/lpc54s018/gcc/startup_LPC54S018_cm4.S
```

**Modify Existing File:** `$ZEPHYR_BASE/soc/nxp/lpc/lpc54xxx/soc.yml`
- Add LPC54S018 support to existing file

---

## 3. Device Tree Files

### 3.1 SoC Device Tree
**Target Directory:** `$ZEPHYR_BASE/dts/arm/nxp/`

```
dts/arm/nxp/nxp_lpc54s018.dtsi
```

### 3.2 Device Tree Bindings
**Target Directory:** `$ZEPHYR_BASE/dts/bindings/`

```
dts/bindings/crc/nxp,lpc-crc.yaml
dts/bindings/display/nxp,lpc-lcdc.yaml
dts/bindings/display/panel-timing.yaml
dts/bindings/flash/nxp,lpc-spifi-nor.yaml
dts/bindings/hwinfo/nxp,lpc-hwinfo.yaml
dts/bindings/memory-controllers/nxp,lpc-emc.yaml
dts/bindings/timer/nxp,lpc-rit.yaml
dts/bindings/usb/nxp,lpc-otg.yaml
```

---

## 4. Custom Drivers

**Target Directory:** `$ZEPHYR_BASE/drivers/`

### 4.1 CAN Driver
```
drivers/can/can_mcan_lpc54s018.c
```

### 4.2 Clock Control Driver
```
drivers/clock/clock_lpc54xxx.c
```

### 4.3 CRC Driver
```
drivers/crc/crc_lpc.c
drivers/crc/CMakeLists.txt
drivers/crc/Kconfig
```

### 4.4 Crypto/Security Drivers
```
drivers/crypto/aes_lpc54s018.c
drivers/crypto/puf_lpc54s018.c
drivers/crypto/sha_lpc54s018.c
```

### 4.5 Display Driver
```
drivers/display/display_lpc_lcdc.c
```

### 4.6 Ethernet Driver
```
drivers/ethernet/eth_nxp_enet_lpc54s018.h
```

### 4.7 Flash Driver
```
drivers/flash/flash_lpc_spifi_nor.c
```

### 4.8 GPIO Driver
```
drivers/gpio/gpio_lpc54xxx.c
```

### 4.9 Hardware Info Driver
```
drivers/hwinfo/hwinfo_lpc.c
drivers/hwinfo/CMakeLists.txt
drivers/hwinfo/Kconfig
```

### 4.10 Memory Controller Driver
```
drivers/memc/memc_nxp_emc.c
drivers/memc/CMakeLists.txt
drivers/memc/Kconfig
```

### 4.11 Secure Boot Drivers (Complete Directory)
```
drivers/secure_boot/CMakeLists.txt
drivers/secure_boot/Kconfig
drivers/secure_boot/secure_boot_lpc54s018.c
drivers/secure_boot/boot_image_decrypt.c
drivers/secure_boot/boot_image_verify.c
drivers/secure_boot/ecdsa_verify.c
drivers/secure_boot/lpc_boot_image.c
```

### 4.12 Timer Driver
```
drivers/timer/rit_lpc.c
```

### 4.13 USB OTG Driver
```
drivers/usb/otg/otg_lpc.c
drivers/usb/otg/CMakeLists.txt
drivers/usb/otg/Kconfig
```

---

## 5. Include Headers

**Target Directory:** `$ZEPHYR_BASE/include/`

### 5.1 Pin Control Headers
```
include/nxp/lpc/LPC54S018J4MET180E-pinctrl.h
```

### 5.2 API Extensions
```
include/zephyr/secure_boot_lpc54s018.h
include/zephyr/lpc_boot_image.h
include/zephyr/drivers/hwinfo_lpc.h
include/zephyr/drivers/crc.h
include/zephyr/dt-bindings/display/panel-timing.h
include/zephyr/dt-bindings/timer/nxp-rit.h
```

---

## 6. CMake and Kconfig Integration Files

### 6.1 Driver CMakeLists.txt Updates
**These files need to be updated to include new drivers:**

```
drivers/CMakeLists.txt          # Add subdirectory references
drivers/crc/CMakeLists.txt      # New file
drivers/hwinfo/CMakeLists.txt   # Update existing
drivers/memc/CMakeLists.txt     # New file
drivers/secure_boot/CMakeLists.txt  # New file
drivers/usb/otg/CMakeLists.txt  # New file
```

### 6.2 Kconfig Updates
**These files need to be updated:**

```
drivers/Kconfig                 # Add source lines for new drivers
drivers/crc/Kconfig            # New file
drivers/hwinfo/Kconfig         # Update existing
drivers/memc/Kconfig           # New file
drivers/secure_boot/Kconfig    # New file
drivers/usb/otg/Kconfig        # New file
```

---

## 7. Sample Applications (Optional - For Testing)

**Target Directory:** `$ZEPHYR_BASE/samples/boards/lpcxpresso54s018/`

### 7.1 Basic Samples
```
samples/boards/lpcxpresso54s018/blinky/CMakeLists.txt
samples/boards/lpcxpresso54s018/blinky/prj.conf
samples/boards/lpcxpresso54s018/blinky/src/main.c
samples/boards/lpcxpresso54s018/blinky/README.rst
```

### 7.2 Driver Test Samples (25 applications)
```
samples/boards/lpcxpresso54s018/adc/
samples/boards/lpcxpresso54s018/can/
samples/boards/lpcxpresso54s018/counter_mrt/
samples/boards/lpcxpresso54s018/crc/
samples/boards/lpcxpresso54s018/ctimer/
samples/boards/lpcxpresso54s018/display_lcd_test/
samples/boards/lpcxpresso54s018/dma/
samples/boards/lpcxpresso54s018/ethernet/
samples/boards/lpcxpresso54s018/flexcomm/
samples/boards/lpcxpresso54s018/gpio_test/
samples/boards/lpcxpresso54s018/hwinfo/
samples/boards/lpcxpresso54s018/i2c/
samples/boards/lpcxpresso54s018/i2s/
samples/boards/lpcxpresso54s018/pwm/
samples/boards/lpcxpresso54s018/rtc/
samples/boards/lpcxpresso54s018/sdmmc/
samples/boards/lpcxpresso54s018/sdram/
samples/boards/lpcxpresso54s018/secure_boot/
samples/boards/lpcxpresso54s018/timer_rit/
samples/boards/lpcxpresso54s018/usb_host/
samples/boards/lpcxpresso54s018/usb_net/
samples/boards/lpcxpresso54s018/usb_otg/
samples/boards/lpcxpresso54s018/usb_rndis/
samples/boards/lpcxpresso54s018/watchdog/
```

Each sample directory contains:
- `CMakeLists.txt`
- `prj.conf`
- `src/main.c`
- `README.rst`
- Board-specific overlays (if needed)

---

## 8. Architecture Extensions (Optional)

**Target Directory:** `$ZEPHYR_BASE/arch/arm/core/cortex_m/`

```
arch/arm/core/cortex_m/reset.S          # Modified for LPC54S018
arch/arm/core/cortex_m/vector_table.S   # Modified for LPC54S018
```

---

## 9. Kernel Extensions (Optional)

**Target Directory:** `$ZEPHYR_BASE/kernel/`

```
kernel/main_weak.c                      # Custom weak main function
```

---

## 10. Library Extensions (Optional)

**Target Directory:** `$ZEPHYR_BASE/lib/libc/minimal/`

```
lib/libc/minimal/syscalls.c             # Custom system calls
```

---

## File Count Summary

| Category | File Count | Description |
|----------|------------|-------------|
| **Board Files** | 16 files | Complete new board support |
| **SoC Files** | 9 files | LPC54S018 SoC variant |
| **Device Tree** | 9 files | SoC definition + bindings |
| **Custom Drivers** | 25+ files | All custom driver implementations |
| **Headers** | 7 files | Pin control + API extensions |
| **Sample Apps** | 100+ files | 25 complete sample applications |
| **CMake/Kconfig** | 10+ files | Build system integration |
| **Optional Files** | 5 files | Architecture/kernel extensions |

**Total Required Files: 180+ files**

---

## Installation Script

Here's a script to copy all required files:

```bash
#!/bin/bash
# LPC54S018 Port Installation Script
# Run this from your LPC54S018 project directory

SOURCE_DIR=$(pwd)
ZEPHYR_BASE=${ZEPHYR_BASE:-/path/to/your/zephyr}

echo "Installing LPC54S018 port to: $ZEPHYR_BASE"

# 1. Board files
mkdir -p $ZEPHYR_BASE/boards/nxp/lpcxpresso54s018
cp -r boards/nxp/lpcxpresso54s018/* $ZEPHYR_BASE/boards/nxp/lpcxpresso54s018/

# 2. SoC files
mkdir -p $ZEPHYR_BASE/soc/nxp/lpc/lpc54xxx/lpc54s018
cp -r soc/nxp/lpc/lpc54xxx/lpc54s018/* $ZEPHYR_BASE/soc/nxp/lpc/lpc54xxx/lpc54s018/

# 3. Device tree files
cp dts/arm/nxp/nxp_lpc54s018.dtsi $ZEPHYR_BASE/dts/arm/nxp/
cp -r dts/bindings/* $ZEPHYR_BASE/dts/bindings/

# 4. Custom drivers
cp -r drivers/* $ZEPHYR_BASE/drivers/

# 5. Headers
mkdir -p $ZEPHYR_BASE/include/nxp/lpc
cp include/nxp/lpc/LPC54S018J4MET180E-pinctrl.h $ZEPHYR_BASE/include/nxp/lpc/
cp -r include/zephyr/* $ZEPHYR_BASE/include/zephyr/

# 6. Sample applications (optional)
mkdir -p $ZEPHYR_BASE/samples/boards/lpcxpresso54s018
cp -r samples/* $ZEPHYR_BASE/samples/boards/lpcxpresso54s018/

echo "Installation complete!"
echo "Test with: west build -b lpcxpresso54s018 samples/basic/blinky"
```

---

## Verification Commands

After installation, verify with these commands:

```bash
# Check board is recognized
west boards | grep lpcxpresso54s018

# Test basic build
west build -b lpcxpresso54s018 samples/basic/blinky

# Test custom driver
west build -b lpcxpresso54s018 samples/boards/lpcxpresso54s018/gpio_test
```

---

## Important Notes

1. **All files have private porting headers** with attribution to David Hor - Xtooltech 2025
2. **Out-of-tree board configuration** - Uses `Kconfig.board` naming
3. **Zephyr 4.2 compatibility** - All files tested with Zephyr 4.2
4. **Complete hardware support** - 100% LPC54S018 peripheral coverage
5. **Production ready** - Tested on actual hardware

## Contact

**Private Porting Developer:**
- **Name:** David Hor
- **Company:** Xtooltech 2025  
- **Email:** david.hor@xtooltech.com

This list contains ALL files required to make the LPC54S018 port work on another computer with fresh Zephyr 4.2 installation.