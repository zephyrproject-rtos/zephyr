zephyr_library()
zephyr_library_link_libraries(drivers)

zephyr_library_sources(
	pci.c
	pci_config.c
	pci_interface.c
	)
zephyr_library_sources_ifdef(CONFIG_PCI_LEGACY_BRIDGE pci_legacy_bridge.c)

zephyr_library_include_directories_ifdef(CONFIG_PCI $ENV{ZEPHYR_BASE}/include/drivers)
