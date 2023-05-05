# OLD -- deprecated
# Build mcuboot
west build -b zb_tv_h743zi ../bootloader/mcuboot/boot/zephyr -d mcuboot
export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$BOARD_NAME/mcuboot --hex-file build/$BOARD_NAME/mcuboot/zephyr/zephyr.signed.hex
## mcuboot has changes to help stm32h7 ota - not in upstream  (for the version we use)

# Build and flash zbacrux_setup
west build -b zb_tv_h743zi zbacrux_setup -d zbacrux_setup

/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.signed.hex

/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.signed.bin

export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$BOARD_NAME/zbacrux_setup --hex-file build/$BOARD_NAME/zbacrux_setup/zephyr/zephyr.signed.hex

# Build and flash zb_actipod_app
west build -b zb_tv_h743zi zb_actipod_app -d zb_actipod_app

/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.signed.hex

/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.signed.bin

export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$BOARD_NAME/zb_actipod_app --hex-file build/$BOARD_NAME/zb_actipod_app/zephyr/zephyr.signed.hex


export BOARD_NAME=zb_tv_h743zi;west build -p -c -b zb_tv_h743zi zb_actipod_app -d build/$(BOARD_NAME)/zb_actipod_app -DCONFIG_APP_SYS_VERSION=\"zbtv_4L_v1.0.2dbg\" -DCONFIG_ACTIPOD_4L=y;/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.hex;/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.bin;export BOARD_NAME=zb_tv_h743zi;west flash --skip-rebuild --build-dir build/zb_actipod_app --hex-file /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.hex

export BOARD_NAME=zb_tv_h743zi;west build -p -c -b zb_tv_h743zi zb_actipod_app -d build/$(BOARD_NAME)/zb_actipod_app -DCONFIG_APP_SYS_VERSION=\"zbtv_15L_v1.0.5\" -DCONFIG_ACTIPOD_15L=y;/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.hex;/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.bin;export BOARD_NAME=zb_tv_h743zi;west flash --skip-rebuild --build-dir build/zb_actipod_app --hex-file /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_actipod_app/zephyr/zephyr.signed.hex

mkdir zedblox_scripts/images/zb_tv_h743zi/D4_zbtv_v1.0.3/
cp -r build/zb_actipod_app/ zedblox_scripts/images/zb_tv_h743zi/D4_zbtv_v1.0.3/

source zephyr-env.sh 
./zedblox_scripts/new_id_acrux.sh -p 4L -b zb_tv_h743zi -t -f zedblox_scripts/ids_generated/optimus_50-ids.text
./zedblox_scripts/new_id_acrux.sh -p 4L -b zb_tv_h743zi -t -i "ZBAX1122PQ55W"


# NEW -- updated -- LATEST

# ON zedblox PC
cd ~/all_repositories/zephyr_new/zephyrproject

## For flashing from scratch -- new changes. not a fixed version
export ZEPHYR_SDK_INSTALL_DIR=/home/sviraaj

### BUILD AND FLASH MCUBOOT
west build -b zb_tv_h743zi bootloader/mcuboot/boot/zephyr -d build-mcuboot
west flash -d build-mcuboot

### BUILD AND FLASH APP
#### For 15L
export VERSION=D15_$(date +%d-%m-%y_%H-%M-%S)
west build -p -c -b zb_tv_h743zi zephyr/zb_actipod_app -d build-zb_actipod_app -- -DCONFIG_ACTIPOD_15L=y -DCONFIG_APP_SYS_VERSION=\"$(echo $VERSION)\" -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/zb-ed25519.pem\"
#### For 4L
export VERSION=D4_$(date +%d-%m-%y_%H-%M-%S)
west build -p -c -b zb_tv_h743zi zephyr/zb_actipod_app -d build-zb_actipod_app -- -DCONFIG_ACTIPOD_4L=y -DCONFIG_APP_SYS_VERSION=\"$(echo $VERSION)\" -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/zb-ed25519.pem\"

west flash -d build-zb_actipod_app

### COPY BUILT IMAGE TO IMAGES VAULT
cp -r build-zb_actipod_app zephyr/zedblox_scripts/images/zb_tv_h743zi/$VERSION

### Rename geenric zephyr bin and hex files to version specific names
mv zephyr/zedblox_scripts/images/zb_tv_h743zi/$VERSION/zephyr/zephyr.signed.hex zephyr/zedblox_scripts/images/zb_tv_h743zi/$VERSION/zephyr/zephyr_$VERSION.signed.hex
mv zephyr/zedblox_scripts/images/zb_tv_h743zi/$VERSION/zephyr/zephyr.signed.bin zephyr/zedblox_scripts/images/zb_tv_h743zi/$VERSION/zephyr/zephyr_$VERSION.signed.bin

### FLASH PRE_BUILT APP
west flash --skip-rebuild --build-dir /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/zedblox_scripts/images/zb_tv_h743zi/<version> --hex-file /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/zedblox_scripts/images/zb_tv_h743zi/<version>/zephyr/zephyr.signed.hex

Eg:
west flash --skip-rebuild --build-dir /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/zedblox_scripts/images/zb_tv_h743zi/D4_zbtv_v1.0.3 --hex-file /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/zedblox_scripts/images/zb_tv_h743zi/D4_zbtv_v1.0.3/zephyr/zephyr.signed.hex
