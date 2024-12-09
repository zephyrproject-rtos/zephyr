# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM power.c)

# CAT1B family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)

# In MTB for APPTYPE == flash: -DFLASH_BOOT -DCY_PDL_FLASH_BOOT
zephyr_compile_definitions(FLASH_BOOT)
zephyr_compile_definitions(CY_PDL_FLASH_BOOT)

# Use custome linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/linker.ld CACHE INTERNAL "")

if(CONFIG_INFINEON_SECURE_LCS OR CONFIG_BOOTLOADER_MCUBOOT)
	# Check cysecuretools
	find_program(CYSECURETOOLS cysecuretools)
	if(NOT CYSECURETOOLS)
		message(FATAL_ERROR "Can't find cysecuretools. To fix, install cysecuretools with pip3.")
	else()
		message("-- Found cysecuretools: ${CYSECURETOOLS}")
	endif()

	# Cysecuretools policy.
	if(NOT CONFIG_INFINEON_SECURE_POLICY)
		# Get default cysecuretools policy

		if(CONFIG_INFINEON_SECURE_LCS)
			message(INFO "CONFIG_INFINEON_SECURE_POLICY was not defined.")
		endif()

		set(default_policy)
		if(CONFIG_INFINEON_SECURE_LCS)
			set(default_policy_name  policy_secure.json)
		else()
			set(default_policy_name  policy_no_secure.json)
		endif()

		# Try to find policy file in project folder
		if("${default_policy}" STREQUAL "")
			if (EXISTS ${APPLICATION_SOURCE_DIR}/policy/${default_policy_name})
				set(default_policy ${APPLICATION_SOURCE_DIR}/policy/${default_policy_name})
			endif()
		endif()

		# Try to find policy file in Zephyr folder
		if("${default_policy}" STREQUAL "")
			if (EXISTS ${ZEPHYR_BASE}/policy/${default_policy_name})
				set(default_policy ${ZEPHYR_BASE}/policy/${default_policy_name})
			endif()
		endif()

		# Using default policy from cysecuretools targets
		if("${default_policy}" STREQUAL "")
			set(default_policy "${Python3_SITELIB}/cysecuretools/targets/cyw20829/policy/${default_policy_name}")
		endif()
	else()
		if(IS_ABSOLUTE ${CONFIG_INFINEON_SECURE_POLICY})
			if (NOT EXISTS  ${CONFIG_INFINEON_SECURE_POLICY})

				message(FATAL_ERROR "Can't find policy: ${CONFIG_INFINEON_SECURE_POLICY}. "
									"Please check if path in CONFIG_INFINEON_SECURE_POLICY is correct."
						)
			endif()
			set(default_policy ${CONFIG_INFINEON_SECURE_POLICY})
		else()

			# Try to find policy file in project folder
			if("${default_policy}" STREQUAL "")
				if (EXISTS ${APPLICATION_SOURCE_DIR}/${CONFIG_INFINEON_SECURE_POLICY})
					set(default_policy ${APPLICATION_SOURCE_DIR}/${CONFIG_INFINEON_SECURE_POLICY})
				endif()
			endif()

			# Try to find policy file in Zephyr folder
			if("${default_policy}" STREQUAL "")
				if (EXISTS ${ZEPHYR_BASE}/${CONFIG_INFINEON_SECURE_POLICY})
					set(default_policy ${ZEPHYR_BASE}/${CONFIG_INFINEON_SECURE_POLICY})
				endif()
			endif()
		endif()
	endif()

	set(cysecuretools_policy ${default_policy} CACHE PATH "cysecuretools policy")
	message("-- Using cysecuretools policy: ${cysecuretools_policy}")
endif()

# Copy Zephyr application hex
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
	${CMAKE_COMMAND} -E copy
	${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}_org.hex
	)

# Get sram_bootstrap address and size
dt_nodelabel(sram_bootstrap NODELABEL "sram_bootstrap")
dt_reg_addr(bootstrap_dst_addr PATH ${sram_bootstrap})
dt_reg_size(bootstrap_size PATH ${sram_bootstrap})

# Calculate the place in flash
math(EXPR flash_addr_offset
	"${CONFIG_CYW20829_FLASH_SAHB_ADDR} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_CYW20829_ROM_START_OFFSET}"
	OUTPUT_FORMAT HEXADECIMAL
)
set(gen_app_header_args --flash_addr_offset ${flash_addr_offset})
set(app_temp_path ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}_org)

if(CONFIG_INFINEON_SECURE_LCS)
	#
	# Addition postbuild action for SECURE LCS
	#
	set(gen_app_header_args ${gen_app_header_args} --secure_lcs True)
	set(app_temp_path ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}_signed_tmp)

	if(CONFIG_INFINEON_SMIF_ENCRYPTION)
		set(gen_app_header_args ${gen_app_header_args} --smif-config ${ZEPHYR_BINARY_DIR}/nonce-output.bin)
		set(enc_option --encrypt --nonce-output nonce-output.bin)
		set(bin2hex_option bin2hex --image ${app_temp_path}_encrypted.bin --output ${app_temp_path}.hex --offset 0x60000030)
	else()
		set(bin2hex_option bin2hex --image ${app_temp_path}.bin --output ${app_temp_path}.hex --offset 0x60000030)
	endif()

	# Sign Zephyr L1 app in SECURE LCS
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
		COMMAND ${CYSECURETOOLS} -q -t cyw20829
			-p ${cysecuretools_policy} sign-image --image-format bootrom_next_app
			-i ${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME} -k 0 -o ${app_temp_path}.bin
			--slot-size ${CONFIG_FLASH_LOAD_SIZE} --app-addr 0x08000030
			${enc_option} ${bin2hex_option}
		)
endif()

# Generate platform specific header (TOC2, l1_desc, etc)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
		${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/gen_app_header.py
		-p ${ZEPHYR_BINARY_DIR} -n ${KERNEL_NAME} ${gen_app_header_args}
		--bootstrap-size	 ${bootstrap_size}
		--bootstrap-dst-addr ${bootstrap_dst_addr}
	)

# Merge platform specific header and zephyr image to a single binary.
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
	COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
	  -o ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex
	  ${app_temp_path}.hex ${ZEPHYR_BINARY_DIR}/app_header.hex
	)

# Use custom mcuboot cmake for sign/encrypt by using cysecuretools
if (CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(zephyr_property_target PROPERTIES SIGNING_SCRIPT
						${CMAKE_CURRENT_LIST_DIR}/mcuboot.cmake)
endif()
