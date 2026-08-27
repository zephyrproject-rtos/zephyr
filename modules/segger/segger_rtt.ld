SECTION_DATA_PROLOGUE(_RTT_SECTION_NAME,(NOLOAD),)
{
__rtt_buff_data_start = .;
*(CONFIG_SEGGER_RTT_SECTION_CUSTOM_NAME)
__rtt_buff_data_end = ALIGN(4);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
__rtt_buff_data_size = __rtt_buff_data_end - __rtt_buff_data_start;
