#!/bin/bash

ESP_DEVICE=${ESP_DEVICE:-/dev/ttyUSB0}
ESP_BAUD_RATE=${ESP_BAUD_RATE:-921600}
ESP_FLASH_SIZE=${ESP_FLASH_SIZE:-detect}
ESP_FLASH_FREQ=${ESP_FLASH_FREQ:-40m}
ESP_FLASH_MODE=${ESP_FLASH_MODE:-dio}
ESP_TOOL=${ESP_TOOL:-espidf}

cmd_flash() {
	local esptool
	local elf_name=${O}/${KERNEL_ELF_NAME}

	if [ "x${ESP_TOOL}" = "xespidf" ]; then
		esptool=${ESP_IDF_PATH}/components/esptool_py/esptool/esptool.py
	else
		esptool=${ESP_TOOL}
	fi
	if [ ! -x ${esptool} ]; then
		echo "esptool could not be found at ${esptool}"
		exit 1
	fi

	echo "Converting ELF to BIN"
	${esptool} --chip esp32 elf2image ${elf_name}

	echo "Flashing ESP32 on ${ESP_DEVICE} (${ESP_BAUD_RATE}bps)"
	${esptool} --chip esp32 \
		--port ${ESP_DEVICE} \
		--baud ${ESP_BAUD_RATE} \
		--before default_reset \
		--after hard_reset \
		write_flash \
		-u \
		--flash_mode ${ESP_FLASH_MODE} \
		--flash_freq ${ESP_FLASH_FREQ} \
		--flash_size ${ESP_FLASH_SIZE} \
		0x1000 ${elf_name/.elf/.bin}
}

CMD="$1"; shift
case "${CMD}" in
   flash)
   	cmd_flash "$@"
   	;;
   *)
   	echo "${CMD} not supported"
   	exit 1
   	;;
esac
