/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki desktop OS
 *
 *
 */

/**
 * \file
 * Argument buffer for passing arguments when starting processes
 * \author Adam Dunkels <adam@dunkels.com>
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup arg Argument buffer
 * @{
 *
 * The argument buffer can be used when passing an argument from an
 * exiting process to a process that has not been created yet. Since
 * the exiting process will have exited when the new process is
 * started, the argument cannot be passed in any of the processes'
 * addres spaces. In such situations, the argument buffer can be used.
 *
 * The argument buffer is statically allocated in memory and is
 * globally accessible to all processes.
 *
 * An argument buffer is allocated with the arg_alloc() function and
 * deallocated with the arg_free() function. The arg_free() function
 * is designed so that it can take any pointer, not just an argument
 * buffer pointer. If the pointer to arg_free() is not an argument
 * buffer, the function does nothing.
 */

#include "contiki.h"
#include "sys/arg.h"

/**
 * \internal Structure used for holding an argument buffer.
 */
struct argbuf {
  char buf[128];
  char used;
};

static struct argbuf bufs[1];

/*-----------------------------------------------------------------------------------*/
/**
 * \internal Initalizer, called by the dispatcher module.
 */
/*-----------------------------------------------------------------------------------*/
void
arg_init(void)
{
  bufs[0].used = 0;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Allocates an argument buffer.
 *
 * \param size The requested size of the buffer, in bytes.
 *
 * \return Pointer to allocated buffer, or NULL if no buffer could be
 * allocated.
 *
 * \note It currently is not possible to allocate argument buffers of
 * any other size than 128 bytes.
 *
 */
/*-----------------------------------------------------------------------------------*/
char *
arg_alloc(char size)
{
  if(bufs[0].used == 0) {
    bufs[0].used = 1;
    return bufs[0].buf;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Deallocates an argument buffer.
 *
 * This function deallocates the argument buffer pointed to by the
 * parameter, but only if the buffer actually is an argument buffer
 * and is allocated. It is perfectly safe to call this function with
 * any pointer.
 *
 * \param arg A pointer.
 */
/*-----------------------------------------------------------------------------------*/
void
arg_free(char *arg)
{
  if(arg == bufs[0].buf) {
    bufs[0].used = 0;
  }
}
/*-----------------------------------------------------------------------------------*/
/** @} */
/** @} */
