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
 * Default definitions and error values for the Contiki program loader.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 */

/** \addtogroup sys
 * @{
 */

/**
 * \defgroup loader The Contiki program loader
 *
 * The Contiki program loader is an abstract interface for loading and
 * starting programs.
 *
 * @{
 */

#ifndef LOADER_H_
#define LOADER_H_

/* Errors that the LOADER_LOAD() function may return: */

#define LOADER_OK                0       /**< No error. */
#define LOADER_ERR_READ          1       /**< Read error. */
#define LOADER_ERR_HDR           2       /**< Header error. */
#define LOADER_ERR_OS            3       /**< Wrong OS. */
#define LOADER_ERR_FMT           4       /**< Data format error. */
#define LOADER_ERR_MEM           5       /**< Not enough memory. */
#define LOADER_ERR_OPEN          6       /**< Could not open file. */
#define LOADER_ERR_ARCH          7       /**< Wrong architecture. */
#define LOADER_ERR_VERSION       8       /**< Wrong OS version. */
#define LOADER_ERR_NOLOADER      9       /**< Program loading not supported. */

#ifdef LOADER_CONF_ARCH
#include LOADER_CONF_ARCH
#endif /* LOADER_CONF_ARCH */

/**
 * Load and execute a program.
 *
 * This macro is used for loading and executing a program, and
 * requires support from the architecture dependent code. The actual
 * program loading is made by architecture specific functions.
 *
 * \note A program loaded with LOADER_LOAD() must call the
 * LOADER_UNLOAD() function to unload itself.
 *
 * \param name The name of the program to be loaded.
 *
 * \param arg A pointer argument that is passed to the program.
 *
 * \return A loader error, or LOADER_OK if loading was successful.
 */
#ifndef LOADER_LOAD
#define LOADER_LOAD(name, arg) LOADER_ERR_NOLOADER
#endif /* LOADER_LOAD */

/**
 * Unload a program from memory.
 *
 * This macro is used for unloading a program and deallocating any
 * memory that was allocated during the loading of the program. This
 * function must be called by the program itself.
 *
 */
#ifndef LOADER_UNLOAD
#define LOADER_UNLOAD()
#endif /* LOADER_UNLOAD */

/**
 * Load a DSC (program description).
 *
 * Loads a DSC (program description) into memory and returns a pointer
 * to the dsc.
 *
 * \return A pointer to the DSC or NULL if it could not be loaded.
 */
#ifndef LOADER_LOAD_DSC
#define LOADER_LOAD_DSC(name) NULL
#endif /* LOADER_LOAD_DSC */

/**
 * Unload a DSC (program description).
 *
 * Unload a DSC from memory and deallocate any memory that was
 * allocated when it was loaded.
 */
#ifndef LOADER_UNLOAD_DSC
#define LOADER_UNLOAD_DSC(dsc)
#endif /* LOADER_UNLOAD */

#endif /* LOADER_H_ */

/** @} */
/** @} */
