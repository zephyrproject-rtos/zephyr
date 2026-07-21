# The last 2 files inside the target_sources should be
# userspace_handler.c and userspace.c. If not the linker would complain.
# This order has to be maintained. Any new file should be placed
# above these 2 files.
target_sources(
  kernel PRIVATE
  futex.c
  mem_domain.c
  userspace_handler.c
  userspace.c
  )
zephyr_sources(
  mutex.c
  user_work.c
  sem.c
)
