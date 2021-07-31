# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/l2)

zephyr_sources_ifdef(CONFIG_ETH_SAM_GMAC
	eth_sam_gmac.c
	phy_sam_gmac.c
	)

zephyr_sources_ifdef(CONFIG_ETH_GECKO
	eth_gecko.c
	phy_gecko.c
	)

zephyr_sources_ifdef(CONFIG_ETH_XLNX_GEM
	eth_xlnx_gem.c
	phy_xlnx_gem.c
	)

zephyr_sources_ifdef(CONFIG_ETH_STELLARIS	 eth_stellaris.c)
zephyr_sources_ifdef(CONFIG_ETH_E1000		   eth_e1000.c)
zephyr_sources_ifdef(CONFIG_ETH_ENC28J60	 eth_enc28j60.c)
zephyr_sources_ifdef(CONFIG_ETH_ENC424J600 eth_enc424j600.c)
zephyr_sources_ifdef(CONFIG_ETH_MCUX		   eth_mcux.c)
zephyr_sources_ifdef(CONFIG_ETH_SMSC911X	 eth_smsc911x.c)
zephyr_sources_ifdef(CONFIG_ETH_STM32_HAL	 eth_stm32_hal.c)
zephyr_sources_ifdef(CONFIG_ETH_LITEETH		 eth_liteeth.c)
zephyr_sources_ifdef(CONFIG_ETH_W5500		 eth_w5500.c)
zephyr_sources_ifdef(CONFIG_DSA_KSZ8XXX		 dsa_ksz8xxx.c)

if(CONFIG_ETH_NATIVE_POSIX)
	zephyr_library()
	zephyr_library_compile_definitions(
    NO_POSIX_CHEATS
    _BSD_SOURCE
    _DEFAULT_SOURCE
    )
	zephyr_library_sources(
		eth_native_posix.c
		eth_native_posix_adapt.c
		)
endif()
