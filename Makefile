.PHONY: all clean

all: zmcuboot zzb_actipod_app

BOARD_NAME ?= zb_tv_h743zi

zmcuboot:
	west build -b zb_tv_h743zi ../bootloader/mcuboot/boot/zephyr -d build/$(BOARD_NAME)/mcuboot
	export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$(BOARD_NAME)/mcuboot --hex-file build/$(BOARD_NAME)/mcuboot/zephyr/zephyr.hex

zzbacrux_setup:
	west build -b zb_tv_h743zi zbacrux_setup -d build/$(BOARD_NAME)/zbacrux_setup
	/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.signed.hex
	/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0 --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zbacrux_setup/zephyr/zephyr.signed.bin
	export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$(BOARD_NAME)/zbacrux_setup --hex-file build/$(BOARD_NAME)/zbacrux_setup/zephyr/zephyr.signed.hex

zzb_actipod_app:
	west build -b zb_tv_h743zi zb_actipod_app -d build/$(BOARD_NAME)/zb_actipod_app
	/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0  --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.hex /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.signed.hex
	/home/sviraaj/.local/bin/imgtool sign --version 0.0.0+0  --align 32 --header-size 1024 --slot-size 393216 --key ../bootloader/mcuboot/zb-ed25519.pem /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.bin /home/sviraaj/all_repositories/zephyr_new/zephyrproject/zephyr/build/zb_tv_h743zi/zb_actipod_app/zephyr/zephyr.signed.bin
	export BOARD_NAME=zb_tv_h743zi;west flash --build-dir build/$(BOARD_NAME)/zb_actipod_app --hex-file build/$(BOARD_NAME)/zb_actipod_app/zephyr/zephyr.signed.hex
