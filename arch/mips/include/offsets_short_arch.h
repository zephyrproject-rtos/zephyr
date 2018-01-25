#ifndef _offsets_short_arch__h_
#define _offsets_short_arch__h_

#include <offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + 0)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + 0)

/* end - threads */

#endif /* _offsets_short_arch__h_ */
