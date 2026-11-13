zephyr_library()

if(CONFIG_J1939_NAME_MANAGEMENT)
  message(FATAL_ERROR "J1939 Name Management is not ported to the Zephyr node API")
endif()

zephyr_library_include_directories(
    .
    port
)

zephyr_library_sources(
    j1939.c
    J1939Timer.c
    j1939_ac.c
    CriticalSectionManager.c
)

zephyr_library_sources_ifdef(CONFIG_J1939_TRANSPORT_PROTOCOL j1939_tp.c)

zephyr_library_sources(
    port/j1939_can_port_stub.c
)

zephyr_linker_sources(DATA_SECTIONS iterables_j1939_nodes_cfg.ld)
zephyr_iterable_section(NAME j1939_node_cfg GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})
