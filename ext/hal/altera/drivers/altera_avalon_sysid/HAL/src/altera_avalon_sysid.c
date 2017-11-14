/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

#include "altera_avalon_sysid.h"
#include "altera_avalon_sysid_regs.h"
#include "alt_types.h"
#include <io.h>

/*
*  This component is special: there's only one of it.
*  Therefore we can dispense with a bunch of complexity
*  normally associated with components, such as specialized
*  structs containing parameter info, and instead use that
*  info by name directly out of system.h.  A downside of this
*  approach is that each time the system is regenerated, and
*  system.h changes, this file must be recompiled.  Fortunately
*  this file is, and is likely to remain, quite small.
*/
#include "system.h"

#ifdef SYSID_BASE
/*
*  return values:
*    0 if the hardware and software appear to be in sync
*    1 if software appears to be older than hardware
*   -1 if hardware appears to be older than software
*/

alt_32 alt_avalon_sysid_test(void)
{
  /* Read the hardware-tag, aka value0, from the hardware. */
  alt_u32 hardware_id = IORD_ALTERA_AVALON_SYSID_ID(SYSID_BASE);

  /* Read the time-of-generation, aka value1, from the hardware register. */
  alt_u32 hardware_timestamp = IORD_ALTERA_AVALON_SYSID_TIMESTAMP(SYSID_BASE);

  /* Return 0 if the hardware and software appear to be in sync. */
  if ((SYSID_TIMESTAMP == hardware_timestamp) && (SYSID_ID == hardware_id))
  {
    return 0;
  }

  /*
  *  Return 1 if software appears to be older than hardware (that is,
  *  the value returned by the hardware is larger than that recorded by
  *  the generator function).
  *  If the hardware time happens to match the generator program's value
  *  (but the hardware tag, value0, doesn't match or 0 would have been
  *  returned above), return an arbitrary value, let's say -1.
  */
  return ((alt_32)(hardware_timestamp - SYSID_TIMESTAMP)) > 0 ? 1 : -1;
}
#endif
