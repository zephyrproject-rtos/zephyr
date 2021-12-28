ITERABLE_SECTION_ROM(event_type, 4)
ITERABLE_SECTION_ROM(event_listener, 4)
ITERABLE_SECTION_ROM(app_event_manager_postinit_hook, 4)
ITERABLE_SECTION_ROM(event_submit_hook, 4)
ITERABLE_SECTION_ROM(event_preprocess_hook, 4)
ITERABLE_SECTION_ROM(event_postprocess_hook, 4)

event_subscribers_all : ALIGN_WITH_INPUT
{
	__start_event_subscribers_all = .;
	KEEP(*(SORT(event_subscribers_*)));
	__stop_event_subscribers_all = .;
} GROUP_LINK_IN(ROMABLE_REGION)
