# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/l2)

zephyr_library_sources_ifdef(CONFIG_ETH_DRIVER_RAW_MODE
  eth_raw.c
  )

zephyr_library_sources_ifdef(CONFIG_ETH_GECKO
	eth_gecko.c
	phy_gecko.c
	)

zephyr_library_sources_ifdef(CONFIG_ETH_XLNX_GEM
	eth_xlnx_gem.c
	phy_xlnx_gem.c
	)

zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC		eth_dwmac.c)
zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC_STM32H7X	eth_dwmac_stm32h7x.c)
zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC_MMU	eth_dwmac_mmu.c)

zephyr_library_sources_ifdef(CONFIG_ETH_E1000		eth_e1000.c)
zephyr_library_sources_ifdef(CONFIG_ETH_ENC28J60	eth_enc28j60.c)
zephyr_library_sources_ifdef(CONFIG_ETH_ENC424J600	eth_enc424j600.c)
zephyr_library_sources_ifdef(CONFIG_ETH_ESP32		eth_esp32.c)
zephyr_library_sources_ifdef(CONFIG_DSA_KSZ8XXX		dsa_ksz8xxx.c)
zephyr_library_sources_ifdef(CONFIG_ETH_LITEETH		eth_liteeth.c)
zephyr_library_sources_ifdef(CONFIG_ETH_MCUX		eth_mcux.c)
zephyr_library_sources_ifdef(CONFIG_ETH_SMSC911X	eth_smsc911x.c)
zephyr_library_sources_ifdef(CONFIG_ETH_STELLARIS	eth_stellaris.c)
zephyr_library_sources_ifdef(CONFIG_ETH_STM32_HAL	eth_stm32_hal.c)
zephyr_library_sources_ifdef(CONFIG_ETH_W5500		eth_w5500.c)
zephyr_library_sources_ifdef(CONFIG_ETH_SAM_GMAC	eth_sam_gmac.c)
zephyr_library_sources_ifdef(CONFIG_ETH_CYCLONEV	eth_cyclonev.c)
zephyr_library_sources_ifdef(CONFIG_SLIP_TAP		eth_slip_tap.c)
zephyr_library_sources_ifdef(CONFIG_ETH_SMSC91X		eth_smsc91x.c)
zephyr_library_sources_ifdef(CONFIG_ETH_IVSHMEM 	eth_ivshmem.c eth_ivshmem_queue.c)
zephyr_library_sources_ifdef(CONFIG_ETH_ADIN2111	eth_adin2111.c)
zephyr_library_sources_ifdef(CONFIG_ETH_LAN865X	        eth_lan865x.c oa_tc6.c)
zephyr_library_sources_ifdef(CONFIG_ETH_NXP_ENET	eth_nxp_enet.c)
zephyr_library_sources_ifdef(CONFIG_ETH_XMC4XXX     eth_xmc4xxx.c)

if(CONFIG_ETH_NXP_S32_NETC)
  zephyr_library_sources(eth_nxp_s32_netc.c)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_NETC_PSI_ENABLED	eth_nxp_s32_netc_psi.c)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_NETC_VSI_ENABLED	eth_nxp_s32_netc_vsi.c)
endif()

zephyr_library_sources_ifdef(CONFIG_ETH_NXP_S32_GMAC	eth_nxp_s32_gmac.c)
zephyr_library_sources_ifdef(CONFIG_ETH_NUMAKER		eth_numaker.c)

if(CONFIG_ETH_NATIVE_POSIX)
  if (CONFIG_NATIVE_APPLICATION)
    set(native_posix_source_files eth_native_posix.c eth_native_posix_adapt.c)
    set_source_files_properties(${native_posix_source_files}
      PROPERTIES COMPILE_DEFINITIONS
      "NO_POSIX_CHEATS;_BSD_SOURCE;_DEFAULT_SOURCE"
    )
    zephyr_library_sources(${native_posix_source_files})
  else()
    zephyr_library_sources(eth_native_posix.c)
    target_sources(native_simulator INTERFACE eth_native_posix_adapt.c)
  endif()
endif()

add_subdirectory(phy)
