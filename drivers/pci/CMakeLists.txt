zephyr_library()
target_link_libraries(drivers__pci drivers)

target_sources(drivers__pci PRIVATE
	pci.c
	pci_config.c
	pci_interface.c
	)
target_sources_ifdef(CONFIG_PCI_LEGACY_BRIDGE drivers__pci PRIVATE pci_legacy_bridge.c)

target_include_directories_ifdef(CONFIG_PCI zephyr PRIVATE $ENV{ZEPHYR_BASE}/include/drivers)
