# Right now only X86 implements required symbols
# e.g pcie_conf_read and pcie_conf_write
zephyr_sources_ifdef(CONFIG_X86 pcie.c)
zephyr_sources_ifdef(CONFIG_PCIE_MSI msi.c)
zephyr_sources_ifdef(CONFIG_PCIE_SHELL shell.c)

add_subdirectory_ifdef(CONFIG_PCIE_ENDPOINT endpoint)
