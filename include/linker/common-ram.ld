	SECTION_DATA_PROLOGUE(initlevel, (OPTIONAL),)
	{
		DEVICE_INIT_SECTIONS()
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	/* verify we don't have rogue .init_<something> initlevel sections */
	SECTION_DATA_PROLOGUE(initlevel_error, (OPTIONAL), )
	{
		DEVICE_INIT_UNDEFINED_SECTION()
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

	SECTION_DATA_PROLOGUE(initshell, (OPTIONAL),)
	{
		SHELL_INIT_SECTIONS()
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_static_thread_area, (OPTIONAL),)
	{
		_static_thread_data_list_start = .;
		KEEP(*(SORT_BY_NAME("._static_thread_data.static.*")))
		_static_thread_data_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_timer_area, (OPTIONAL),)
	{
		_k_timer_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_timer.static.*")))
		_k_timer_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mem_slab_area, (OPTIONAL),)
	{
		_k_mem_slab_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_mem_slab.static.*")))
		_k_mem_slab_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mem_pool_area, (OPTIONAL),)
	{
		KEEP(*(SORT_BY_NAME("._k_memory_pool.struct*")))
		_k_mem_pool_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_mem_pool.static.*")))
		_k_mem_pool_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_sem_area, (OPTIONAL),)
	{
		_k_sem_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_sem.static.*")))
		_k_sem_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mutex_area, (OPTIONAL),)
	{
		_k_mutex_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_mutex.static.*")))
		_k_mutex_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_alert_area, (OPTIONAL),)
	{
		_k_alert_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_alert.static.*")))
		_k_alert_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_fifo_area, (OPTIONAL),)
	{
		_k_fifo_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_fifo.static.*")))
		_k_fifo_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_lifo_area, (OPTIONAL),)
	{
		_k_lifo_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_lifo.static.*")))
		_k_lifo_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_stack_area, (OPTIONAL),)
	{
		_k_stack_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_stack.static.*")))
		_k_stack_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_msgq_area, (OPTIONAL),)
	{
		_k_msgq_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_msgq.static.*")))
		_k_msgq_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mbox_area, (OPTIONAL),)
	{
		_k_mbox_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_mbox.static.*")))
		_k_mbox_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_pipe_area, (OPTIONAL),)
	{
		_k_pipe_list_start = .;
		KEEP(*(SORT_BY_NAME("._k_pipe.static.*")))
		_k_pipe_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_task_list, (OPTIONAL),)
	{
		_k_task_list_start = .;
		*(._k_task_list.public.*)
		*(._k_task_list.private.*)
		_k_task_list_idle_start = .;
		*(._k_task_list.idle.*)
		KEEP(*(SORT_BY_NAME("._k_task_list*")))
		_k_task_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_task_ptr, (OPTIONAL),)
	{
		_k_task_ptr_start = .;
		*(._k_task_ptr.public.*)
		*(._k_task_ptr.private.*)
		*(._k_task_ptr.idle.*)
		KEEP(*(SORT_BY_NAME("._k_task_ptr*")))
		_k_task_ptr_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_pipe_ptr, (OPTIONAL),)
	{
		_k_pipe_ptr_start = .;
		*(._k_pipe_ptr.public.*)
		*(._k_pipe_ptr.private.*)
		KEEP(*(SORT_BY_NAME("._k_pipe_ptr*")))
		_k_pipe_ptr_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mem_map_ptr, (OPTIONAL),)
	{
		_k_mem_map_ptr_start = .;
		*(._k_mem_map_ptr.public.*)
		*(._k_mem_map_ptr.private.*)
		KEEP(*(SORT_BY_NAME("._k_mem_map_ptr*")))
		_k_mem_map_ptr_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_event_list, (OPTIONAL),)
	{
		_k_event_list_start = .;
		*(._k_event_list.event.*)
		KEEP(*(SORT_BY_NAME("._k_event_list*")))
		_k_event_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_memory_pool, (OPTIONAL),)
	{
		*(._k_memory_pool.struct*)
		KEEP(*(SORT_BY_NAME("._k_memory_pool.struct*")))

		_k_mem_pool_start = .;
		*(._k_memory_pool.*)
		KEEP(*(SORT_BY_NAME("._k_memory_pool*")))
		_k_mem_pool_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
