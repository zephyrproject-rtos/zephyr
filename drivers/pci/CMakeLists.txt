zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            pci.c
            pci_config.c
            pci_interface.c
            IFDEF:${CONFIG_PCI_LEGACY_BRIDGE} pci_legacy_bridge.c
            IFDEF:${CONFIG_PCI_SHELL}         pci_shell.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
