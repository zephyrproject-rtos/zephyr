zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_BT_UWP5661	sipc.c)
zephyr_sources_ifdef(CONFIG_BT_UWP5661	uki_config.c)
zephyr_sources_ifdef(CONFIG_BT_UWP5661	uki_utlis.c)
zephyr_sources_ifdef(CONFIG_BT_UWP5661	uwp5661.c)

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_bt_cfg.sh
	bt_configure.h
	bt_configure_pskey.ini
	bt_configure_rf.ini
	bt_info.ini
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
