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
