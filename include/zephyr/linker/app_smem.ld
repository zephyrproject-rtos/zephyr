/* SPDX-License-Identifier: Apache-2.0 */

/*
 * This hackish way of including files is due to CMake issues:
 *   https://gitlab.kitware.com/cmake/cmake/issues/11985
 *   https://gitlab.kitware.com/cmake/cmake/issues/13718
 *
 * When using the "Unix Makefiles" generator, CMake simply
 * greps for "#include" to generate dependency list.
 * So if doing it normally, both files are being included
 * in the dependency list. This creates weird dependency
 * issue:
 *
 *   1. Using A.ld to create a linker script A.cmd.
 *   2. Using A.cmd to generate A_prebuilt.elf.
 *   3. Using A_prebuilt.elf to create B.ld.
 *   4. Creating B.cmd with B.ld.
 *   5. Creating B_prebuilt.elf using B.cmd.
 *
 * Since the dependency list of A.cmd contains both
 * A.ld and B.ld, when make is invoked again, B.ld
 * is newer than A.cmd so everything from this point on
 * gets rebuilt. In order to break this cycle, this
 * hackish needs to be used since CMake does not parse
 * macros, and thus these will not appear in
 * the dependency list. The dependencies should then be
 * put in CMakeLists.txt instead.
 *
 * Note: Ninja generator does not suffer from this issue.
 */
#ifdef LINKER_APP_SMEM_UNALIGNED
#define APP_SMEM_LD <app_smem_unaligned.ld>
#else
#define APP_SMEM_LD <app_smem_aligned.ld>
#endif

#include APP_SMEM_LD
#undef APP_SMEM_LD
