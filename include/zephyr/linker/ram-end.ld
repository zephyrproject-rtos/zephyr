/*
 * Added after the very last allocation that might land in RAM to define the various
 * end-of-used-memory symbols
 */

    SECTION_PROLOGUE(.last_ram_section, (NOLOAD),)
    {
#ifdef LAST_RAM_ALIGN
	LAST_RAM_ALIGN
#endif
	_image_ram_end = .;
	_image_ram_size = _image_ram_end - _image_ram_start;
	_end = .; /* end of image */
	z_mapped_end = .;
    } GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
