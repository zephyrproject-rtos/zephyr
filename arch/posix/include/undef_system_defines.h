/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Undefine all system-specific macros defined internally, by the compiler.
 * Run 'gcc -dM -E - < /dev/null | sort' to get full list of internally
 * defined macros.
 */

#undef __gnu_linux__
#undef __linux
#undef __linux__
#undef linux

#undef __unix
#undef __unix__
#undef unix
