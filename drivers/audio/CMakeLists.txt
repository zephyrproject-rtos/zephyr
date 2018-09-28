zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AUDIO_TLV320DAC	tlv320dac310x.c)
