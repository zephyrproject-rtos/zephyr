zephyr_sources(
	pci.c
	pci_config.c
	pci_interface.c
	)
zephyr_sources_ifdef(CONFIG_PCI_LEGACY_BRIDGE pci_legacy_bridge.c)
