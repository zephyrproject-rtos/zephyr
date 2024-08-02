zephyr_library()

zephyr_library_sources(pcie.c vc.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_CONTROLLER controller.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_ECAM pcie_ecam.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_MSI msi.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_PTM ptm.c)
