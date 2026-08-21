zephyr_library()

zephyr_library_include_directories(
    .
    Application
    port
)

zephyr_library_sources(
    J1939.c
    J1939Timer.c
    J1939Ac.c
    CriticalSectionManager.c
)

zephyr_library_sources_ifdef(CONFIG_J1939_TRANSPORT_PROTOCOL J1939Tp.c)

zephyr_library_sources(
    Application/J1939_App.c
    Application/J1939Ac_App.c
)

zephyr_library_sources(
    port/j1939_can_port_stub.c
)

zephyr_linker_sources(DATA_SECTIONS iterables_j1939_nodes_cfg.ld)
zephyr_iterable_section(NAME j1939_node_cfg GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})
