
-call_properties+={"name(z_phys_map)", {"pointee_read(1=never)","pointee_write(1=always)","taken()"}}
-call_properties+={"name(pcie_get_mbar)", {"pointee_read(3=never)","pointee_write(3=maybe)","taken()"}}
-call_properties+={"name(k_mem_region_align)", {
    "pointee_read(1..2=never)",
    "pointee_write(1..2=always)","taken()"
    }}
-call_properties+={"name(pentry_get)", {
        "pointee_read(1..2=never&&3..4=always)",
        "pointee_write(1..2=maybe&&3..4=never)"
        }}
-call_properties+={"name(z_phys_unmap)", {"pointee_read(1=never)","pointee_write(1=never)"}}
-call_properties+={"name(check_sum)", {"pointee_read(1=always)","pointee_write(1=never)"}}
-call_properties+={"name(z_impl_device_get_binding)", {"pointee_read(1=maybe)","pointee_write(1=maybe)","taken()"}}
-call_properties+={"name(z_setup_new_thread)", {"pointee_read(10=maybe)","pointee_write(10=never)","taken()"}}
-call_properties+={"name(mbox_message_put)", {"pointee_read(3=always)","pointee_write(3=always)"}}
-doc_begin="The functions can be implemented using the GCC built-in functions.
	See Section \"6.62.13 6.56 Built-in Functions to Perform Arithmetic with Overflow Checking\" of "GCC_MANUAL"."
-call_properties+={"name(size_mul_overflow)", {"pointee_read(3=never)","pointee_write(3=always)","taken()"}}
-call_properties+={"name(size_add_overflow)", {"pointee_read(3=never)","pointee_write(3=always)","taken()"}}
-call_properties+={"name(__builtin_mul_overflow)", {"pointee_read(3=never)","pointee_write(3=always)","taken()"}}
-call_properties+={"name(__builtin_add_overflow)", {"pointee_read(3=never)","pointee_write(3=always)","taken()"}}
-doc_end

-call_properties+={"name(__builtin_va_end)", {"taken()"}} # Not documented in gcc.pdf
-call_properties+={"name(arch_page_phys_get)", {"pointee_read(2=never)","pointee_write(2=maybe)","taken()"}}
-call_properties+={"name(cbvprintf)", {"taken()"}}
-call_properties+={"name(cbvprintf)", {"taken()"}}
-call_properties+={"name(char2hex)", {"pointee_read(2=never)","pointee_write(2=maybe)"}}
-call_properties+={"name(find_and_stack)", {"pointee_read(3=never)","pointee_write(3=always)"}}
-call_properties+={"name(fix_missing_black)", {"pointee_read(1=always)","pointee_write(1=maybe)"}}
-call_properties+={"name(__get_cpuid)", {"pointee_read(2..5=never)","pointee_write(2..5=always)"}}
-call_properties+={"name(k_mutex_init)", {"pointee_read(1=never)","pointee_write(1=always)"}}
-call_properties+={"name(k_sem_init)", {"pointee_read(1=never)","pointee_write(1=maybe)"}}
-call_properties+={"name(k_work_init)", {"pointee_read(1=never)","pointee_write(1=always)"}}
-call_properties+={"name(k_work_init_delayable)", {"pointee_read(1=never)","pointee_write(1=always)"}}
-call_properties+={"name(k_work_queue_start)", {"taken()"}}
-call_properties+={"name(log_from_user)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(log_n)", {"taken()"}}
-call_properties+={"name(log_string_sync)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(match_region)", {"pointee_read(5..6=never)","pointee_write(5=always&&6=maybe)"}}
-call_properties+={"name(mbox_async_alloc)", {"pointee_read(1=never)","pointee_write(1=maybe)"}}
-call_properties+={"name(pipe_xfer_prepare)", {"pointee_read(2=never)","pointee_write(2=always)"}}
-call_properties+={"name(printk)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(snprintk)", {"pointee_read(1=never)","pointee_write(1=always)", "taken()"}} # to check
-call_properties+={"name(snprintk)", {"taken()"}}
-call_properties+={"name(sys_bitarray_alloc)", {"pointee_read(3=never)","pointee_write(3=maybe)","taken()"}}
-call_properties+={"name(sys_slist_init)", {"pointee_read(1=never)","pointee_write(1=always)"}}
-call_properties+={"name(vprintk)", {"taken()"}}
-call_properties+={"name(z_dummy_thread_init)", {"pointee_read(1=never)","pointee_write(1=always)"}} # the function does not initialize all the fields
-call_properties+={"name(z_impl_k_stack_pop)", {"taken()"}}
-call_properties+={"name(z_impl_z_log_msg2_runtime_vcreate)", {"taken()"}}
-call_properties+={"name(z_log_minimal_printk)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(z_log_msg2_runtime_create)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(z_log_printk)", {"taken()"}}
-call_properties+={"name(z_log_printf_arg_checker)", {"pointee_write(1..=never)", "taken()"}}
-call_properties+={"name(z_log_strdup)", {"taken()"}}
-call_properties+={"name(z_rb_foreach_next)", {"taken()"}}
-call_properties+={"name(z_user_string_copy)", {"pointee_read(1=never)","pointee_write(1=maybe)","taken()"}}

-doc_begin="These macros are designed to evaluate to either 0 or 1."
-call_properties+={"macro(name(UTIL_NOT))",{"data_kind(0=int_bool)"}}
-call_properties+={"macro(name(IS_ENABLED))",{"data_kind(0=int_bool)"}}
-call_properties+={"decl(name(isspace))",{"data_kind(0=int_bool)"}}
-call_properties+={"macro(name(isdigit))",{"data_kind(0=int_bool)"}}
-call_properties+={"decl(name(isdigit))",{"data_kind(0=int_bool)"}}
-call_properties+={"macro(name(isalpha))",{"data_kind(0=int_bool)"}}
-call_properties+={"decl(name(isalpha))",{"data_kind(0=int_bool)"}}
-call_properties+={"macro(name(isupper))",{"data_kind(0=int_bool)"}}
-call_properties+={"decl(name(isupper))",{"data_kind(0=int_bool)"}}
-doc_end

-doc="__builtin_alloca cannot interfere with other effects."
-call_properties+={"decl(name(__builtin_alloca))",{"noeffect"}}

-doc="log_strdup cannot interfere with other effects."
-call_properties+={"decl(name(log_strdup))",{"noeffect"}}


# Not documented functions
# device_map
