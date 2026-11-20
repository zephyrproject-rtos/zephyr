# Print a backtrace from each fault handler, and from z_arm_fpu_trap() which
# the undefined instruction handler calls first with FPU sharing. Quit once
# the application has taken all its faults.
set pagination off
set breakpoint pending on
break z_arm_fault_undef_instruction
break z_arm_fault_prefetch
break z_arm_fault_data
break z_arm_fpu_trap
commands 1-4
  bt
  continue
end
break test_done
commands 5
  quit
end
continue
