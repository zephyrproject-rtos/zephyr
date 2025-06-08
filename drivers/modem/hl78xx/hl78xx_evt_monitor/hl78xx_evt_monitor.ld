/*HL78XX event monitors */
. = ALIGN(4);
_hl78xx_evt_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._hl78xx_evt_monitor_entry.*")));
_hl78xx_evt_monitor_entry_list_end = .;
