/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \addtogroup mem
 * @{
 */

/**
 * \defgroup mmem Managed memory allocator
 *
 * The managed memory allocator is a fragmentation-free memory
 * manager. It keeps the allocated memory free from fragmentation by
 * compacting the memory when blocks are freed. A program that uses
 * the managed memory module cannot be sure that allocated memory
 * stays in place. Therefore, a level of indirection is used: access
 * to allocated memory must always be done using a special macro.
 *
 * \note This module has not been heavily tested.
 * @{
 */

/**
 * \file
 *         Header file for the managed memory allocator
 * \author
 *         Adam Dunkels <adam@sics.se>
 * 
 */

#ifndef MMEM_H_
#define MMEM_H_

/*---------------------------------------------------------------------------*/
/**
 * \brief      Get a pointer to the managed memory
 * \param m    A pointer to the struct mmem 
 * \return     A pointer to the memory block, or NULL if memory could
 *             not be allocated. 
 * \author     Adam Dunkels
 *
 *             This macro is used to get a pointer to a memory block
 *             allocated with mmem_alloc().
 *
 * \hideinitializer
 */
#define MMEM_PTR(m) (struct mmem *)(m)->ptr

struct mmem {
  struct mmem *next;
  unsigned int size;
  void *ptr;
};

/* XXX: tagga minne med "interrupt usage", vilke gör att man är
   speciellt varsam under free(). */

int  mmem_alloc(struct mmem *m, unsigned int size);
void mmem_free(struct mmem *);
void mmem_init(void);

#endif /* MMEM_H_ */

/** @} */
/** @} */
