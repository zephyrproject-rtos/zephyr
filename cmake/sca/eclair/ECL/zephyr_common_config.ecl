-eval_file=out_of_initial_scope.ecl
-eval_file=language_extensions.ecl
-eval_file=call_properties.ecl
-eval_file=extra.ecl

-doc="Hides all reports that have all areas out of scope."
-reports+={hide,all_exp_external}

-doc="The API interface files are allowed to contain unused macros."
-config=MC3R1.R2.5,reports+={hide, "any_area(macro(loc(top(public()||kind(pseudo)))))"}

-doc="Several header files are meant to be included in C as well as in C++ translation units."
-config=MC3R1.R20.1,exception=extern_C

-default_call_properties+="pointee_read(1..=never)"
-default_call_properties+="pointee_write(1..=always)"
-default_call_properties+="taken()"

-doc_begin="These macros pass its first argument in a safe way to related compiler intrinsics."
-config=MC3R1.R20.7,macros={safe,"^(va_start||va_arg)$"}
-doc_end

-doc_begin="The value-preserving conversions of integer constants are safe"
-config=MC3R1.R10.1,etypes={safe,"any()","preserved_integer_constant()"}
-config=MC3R1.R10.3,etypes={safe,"any()","preserved_integer_constant()"}
-config=MC3R1.R10.4,etypes={safe,"any()","preserved_integer_constant()||sibling(rhs,preserved_integer_constant())"}
-doc_end

-doc_begin="Some macros are deliberately compile-time constants due to project configurability."
-macro_selector={const_wrapper_macros,"name(CONSTEXPR)"}
-config=MC3R1.R14.3,statements={safe, "node(if_stmt||conditional_operator)&&child(cond,wrapped(node(paren_expr)&&!macro(const_wrapper_macros), macro(const_wrapper_macros)))"}
-doc_end

-doc="Casts to log_arg_t in logging macros are safe and expected."
-config=MC3R1.R11.6,reports+={deliberate,"any_area(any_loc(any_exp(macro(name(__LOG_ARG_CAST)))))"}

-doc_begin="The following function-like macros are not writable as inline functions."
-config=MC3R1.D4.9,macros+={deliberate,"name(ARG_UNUSED)"}
-config=MC3R1.D4.9,macros+={deliberate,"name(CONSTEXPR)"}
-config=MC3R1.D4.9,macros+={deliberate,"name(compiler_barrier)"}
-config=MC3R1.D4.9,macros+={deliberate,"name(likely)"}
-config=MC3R1.D4.9,macros+={deliberate,"name(unlikely)"}
-doc_end

-doc_begin="For the following function-like macros it should be decided whether substitution with equivalent static inline functions is wanted and feasible."
-config=MC3R1.D4.9,macros+={questionable,"name(ATOMIC_ELEM)"}
-config=MC3R1.D4.9,macros+={questionable,"name(ATOMIC_MASK)"}
-config=MC3R1.D4.9,macros+={questionable,"name(BIT32)"}
-config=MC3R1.D4.9,macros+={questionable,"name(BIT64)"}
-config=MC3R1.D4.9,macros+={questionable,"name(BIT64_MASK)"}
-config=MC3R1.D4.9,macros+={questionable,"name(BIT_MASK)"}
-config=MC3R1.D4.9,macros+={questionable,"name(DEVICE_MMIO_GET)"}
-config=MC3R1.D4.9,macros+={questionable,"name(DEVICE_MMIO_MAP)"}
-config=MC3R1.D4.9,macros+={questionable,"name(DEVICE_MMIO_RAM_PTR)"}
-config=MC3R1.D4.9,macros+={questionable,"name(IN)"}
-config=MC3R1.D4.9,macros+={questionable,"name(LOG_CORE_INIT)"}
-config=MC3R1.D4.9,macros+={questionable,"name(MAX)"}
-config=MC3R1.D4.9,macros+={questionable,"name(MB)"}
-config=MC3R1.D4.9,macros+={questionable,"name(MIN)"}
-config=MC3R1.D4.9,macros+={questionable,"name(OUT)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_BDF)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_BDF_TO_BUS)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_BAR_64)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_BAR_ADDR)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_BAR_INVAL_FLAGS)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_BAR_IO)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_BAR_MEM)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_CAPPTR_FIRST)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_CAP_ID)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_CAP_NEXT)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_EXT_CAP_ID)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_EXT_CAP_NEXT)"}
-config=MC3R1.D4.9,macros+={questionable,"name(PCIE_CONF_INTR_IRQ)"}
-config=MC3R1.D4.9,macros+={questionable,"name(POINTER_TO_UINT)"}
-config=MC3R1.D4.9,macros+={questionable,"name(ROUND_DOWN)"}
-config=MC3R1.D4.9,macros+={questionable,"name(ROUND_UP)"}
-config=MC3R1.D4.9,macros+={questionable,"name(UINT_TO_POINTER)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_BOOT_VIRT_TO_PHYS)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_IRQ_TO_INTERRUPT_VECTOR)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_KERNEL_STACK_SIZE_ADJUST)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_LOG_MSG2_ALIGNED_WLEN)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_MEM_PHYS_ADDR)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_MEM_VIRT_ADDR)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_STACK_PTR_ALIGN)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_THREAD_STACK_SIZE_ADJUST)"}
-config=MC3R1.D4.9,macros+={questionable,"name(Z_TICK_ABS)"}
-config=MC3R1.D4.9,macros+={questionable,"name(ceiling_fraction)"}
-config=MC3R1.D4.9,macros+={questionable,"name(irq_enable)"}
-config=MC3R1.D4.9,macros+={questionable,"name(irq_lock)"}
-config=MC3R1.D4.9,macros+={questionable,"name(irq_unlock)"}
-config=MC3R1.D4.9,macros+={questionable,"name(k_panic)"}
-doc_end

-doc="Use of CODE_UNREACHABLE is defensive programming."
-config=MC3R1.R2.1,reports+={safe,"any_area(any_loc(any_exp(macro(name(CODE_UNREACHABLE)))))"}

-doc_begin="Identifers beginning with _ are tolerated."
-config=MC3R1.R21.1,macros={relied,"^_.*$"}
-config=MC3R1.R21.2,declarations={relied,"^(.*::)?_.*$"}
-doc_end

-eval_file=adopted_code.ecl
-eval_file=adopted_deviations.ecl

-doc="Hide reports marked as compliant."
-remap_rtag={compliant,hide}

-doc="Hide reports marked as safe."
-remap_rtag={safe,hide}

-doc="Hide reports marked as relied."
-remap_rtag={relied,hide}

-doc="Hide reports marked as questionable."
-remap_rtag={questionable,hide}

-doc="Hide reports marked as deliberate."
-remap_rtag={deliberate,hide}

-doc="Hide reports marked as disapplied."
-remap_rtag={disapplied,hide}
