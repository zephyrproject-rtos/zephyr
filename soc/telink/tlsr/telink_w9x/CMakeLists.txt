# Copyright (c) 2024 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_include_directories(.)

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
	sys_heap_alloc_wrap.c
	cache.c
)

if (CONFIG_BT)
zephyr_sources(
	bluetooth/bt_mac.c
)
endif()

if(CONFIG_TELINK_W91_IPC_DISPATCHER)
zephyr_sources(
	ipc/ipc_dispatcher.c
	ipc/ipc_based_driver.c
)
endif()

if(CONFIG_TELINK_W91_BLOCKING_CORE)
zephyr_sources(
	blocking_core/blocking.c
)
endif()

if(CONFIG_TELINK_W91_CORE_HEARTBEAT)
zephyr_sources(
	core_heartbeat/core_heartbeat.c
)
endif()

if(CONFIG_TELINK_W91_DEBUG_ENABLE)
zephyr_sources(
	debug.c
)
endif()

zephyr_sources_ifdef(CONFIG_MCUBOOT_ACTION_HOOKS
	mcuboot_status_change.c
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_TELINK_W91_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 dlm.ld)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

# Wrap allocator functions
zephyr_link_libraries(
	-Wl,--wrap,sys_heap_alloc
	-Wl,--wrap,sys_heap_aligned_alloc
	-Wl,--wrap,sys_heap_aligned_realloc
)

if(CONFIG_TELINK_W91_ECC_HW_ACC)
zephyr_compile_definitions(
	MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
)

zephyr_include_directories(
	${ZEPHYR_MBEDTLS_MODULE_DIR}/include
	${ZEPHYR_BASE}/modules/mbedtls/configs
)

zephyr_sources(
	crypto/mbedtls_wrapper.c
	crypto/ecp_alt_backend.c
	crypto/ecp_alt_test.c
)

zephyr_link_libraries(
	-Wl,--wrap,mbedtls_ecp_check_pubkey
	-Wl,--wrap,mbedtls_ecp_mul
	-Wl,--wrap,mbedtls_ecp_mul_restartable
	-Wl,--wrap,mbedtls_ecp_muladd
	-Wl,--wrap,mbedtls_ecp_muladd_restartable
	-Wl,--wrap,mbedtls_ecp_self_test
)
endif()

if(CONFIG_TELINK_W91_FETCH_N22_BIN)
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
		${ZEPHYR_BASE}/scripts/utils/telink_w91_post_build ${ZEPHYR_BINARY_DIR} ${CONFIG_TELINK_W91_FETCH_N22_BIN_REVISION}
		$<IF:$<BOOL:${CONFIG_TELINK_W91_N22_MATTER_OTA_LAYOUT}>,1,0>
	)
endif()
