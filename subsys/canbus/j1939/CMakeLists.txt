zephyr_library()

zephyr_library_include_directories(
	.
	Application
	port
)

zephyr_library_sources(
	J1939.c
	J1939List.c
	J1939Timer.c
	J1939Ac.c
)

zephyr_library_sources_ifdef(CONFIG_J1939_TRANSPORT_PROTOCOL J1939Tp.c)

zephyr_library_sources(
	Application/J1939_App.c
	Application/J1939Ac_App.c
)

zephyr_library_sources_ifdef(CONFIG_J1939_LEGACY_TIME_APP_SOURCE
	Application/J1939Timer_App.c
)

zephyr_library_sources(
	port/j1939_can_port_stub.c
	port/j1939_zephyr_glue.c
)
