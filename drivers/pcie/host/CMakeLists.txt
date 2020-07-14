zephyr_sources(pcie.c)
zephyr_sources_ifdef(CONFIG_PCIE_MSI msi.c)
zephyr_sources_ifdef(CONFIG_PCIE_SHELL shell.c)
