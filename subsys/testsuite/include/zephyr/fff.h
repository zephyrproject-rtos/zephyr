/*
LICENSE

The MIT License (MIT)

Copyright (c) 2010 Michael Long

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef FAKE_FUNCTIONS
#define FAKE_FUNCTIONS

#include <stdarg.h>
#include <string.h> /* For memset and memcpy */

#define FFF_MAX_ARGS (20u)
#ifndef FFF_ARG_HISTORY_LEN
#define FFF_ARG_HISTORY_LEN (50u)
#endif
#ifndef FFF_CALL_HISTORY_LEN
#define FFF_CALL_HISTORY_LEN (50u)
#endif
#ifndef FFF_GCC_FUNCTION_ATTRIBUTES
#define FFF_GCC_FUNCTION_ATTRIBUTES
#endif
/* -- INTERNAL HELPER MACROS -- */
#define SET_RETURN_SEQ(FUNCNAME, ARRAY_POINTER, ARRAY_LEN)                                         \
	FUNCNAME##_fake.return_val_seq = ARRAY_POINTER;                                            \
	FUNCNAME##_fake.return_val_seq_len = ARRAY_LEN;
#define SET_CUSTOM_FAKE_SEQ(FUNCNAME, ARRAY_POINTER, ARRAY_LEN)                                    \
	FUNCNAME##_fake.custom_fake_seq = ARRAY_POINTER;                                           \
	FUNCNAME##_fake.custom_fake_seq_len = ARRAY_LEN;

/* Defining a function to reset a fake function */
#define RESET_FAKE(FUNCNAME)                                                                       \
	{                                                                                          \
		FUNCNAME##_reset();                                                                \
	}

#define DECLARE_ARG(type, n, FUNCNAME)                                                             \
	type arg##n##_val;                                                                         \
	type arg##n##_history[FFF_ARG_HISTORY_LEN];

#define DECLARE_ALL_FUNC_COMMON                                                                    \
	unsigned int call_count;                                                                   \
	unsigned int arg_history_len;                                                              \
	unsigned int arg_histories_dropped;

#define DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                                  \
	RETURN_TYPE return_val_history[FFF_ARG_HISTORY_LEN];

#define SAVE_ARG(FUNCNAME, n)                                                                      \
	memcpy((void *)&FUNCNAME##_fake.arg##n##_val, (void *)&arg##n, sizeof(arg##n));

#define ROOM_FOR_MORE_HISTORY(FUNCNAME) FUNCNAME##_fake.call_count < FFF_ARG_HISTORY_LEN

#define SAVE_RET_HISTORY(FUNCNAME, RETVAL)                                                          \
	if ((FUNCNAME##_fake.call_count - 1) < FFF_ARG_HISTORY_LEN)                                 \
		memcpy((void *)&FUNCNAME##_fake.return_val_history[FUNCNAME##_fake.call_count - 1], \
		       (const void *)&RETVAL, sizeof(RETVAL));

#define SAVE_ARG_HISTORY(FUNCNAME, ARGN)                                                           \
	memcpy((void *)&FUNCNAME##_fake.arg##ARGN##_history[FUNCNAME##_fake.call_count],           \
	       (void *)&arg##ARGN, sizeof(arg##ARGN));

#define HISTORY_DROPPED(FUNCNAME) FUNCNAME##_fake.arg_histories_dropped++

#define DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                              \
	RETURN_TYPE return_val;                                                                    \
	int return_val_seq_len;                                                                    \
	int return_val_seq_idx;                                                                    \
	RETURN_TYPE *return_val_seq;

#define DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                          \
	int custom_fake_seq_len;                                                                   \
	int custom_fake_seq_idx;

#define INCREMENT_CALL_COUNT(FUNCNAME) FUNCNAME##_fake.call_count++

#define RETURN_FAKE_RESULT(FUNCNAME)                                                                \
	if (FUNCNAME##_fake.return_val_seq_len) { /* then its a sequence */                         \
		if (FUNCNAME##_fake.return_val_seq_idx < FUNCNAME##_fake.return_val_seq_len) {      \
			SAVE_RET_HISTORY(                                                           \
				FUNCNAME,                                                           \
				FUNCNAME##_fake.return_val_seq[FUNCNAME##_fake.return_val_seq_idx]) \
			return FUNCNAME##_fake                                                      \
				.return_val_seq[FUNCNAME##_fake.return_val_seq_idx++];              \
		}                                                                                   \
		SAVE_RET_HISTORY(                                                                   \
			FUNCNAME,                                                                   \
			FUNCNAME##_fake.return_val_seq[FUNCNAME##_fake.return_val_seq_len - 1])     \
		return FUNCNAME##_fake.return_val_seq[FUNCNAME##_fake.return_val_seq_len -          \
						      1]; /* return last element */                 \
	}                                                                                           \
	SAVE_RET_HISTORY(FUNCNAME, FUNCNAME##_fake.return_val)                                      \
	return FUNCNAME##_fake.return_val;

#ifdef __cplusplus
#define FFF_EXTERN_C extern "C" {
#define FFF_END_EXTERN_C }
#else /* ansi c */
#define FFF_EXTERN_C
#define FFF_END_EXTERN_C
#endif /* cpp/ansi c */

#define DEFINE_RESET_FUNCTION(FUNCNAME)                                                            \
	void FUNCNAME##_reset(void)                                                                \
	{                                                                                          \
		memset(&FUNCNAME##_fake, 0, sizeof(FUNCNAME##_fake));                              \
		FUNCNAME##_fake.arg_history_len = FFF_ARG_HISTORY_LEN;                             \
	}
/* -- END INTERNAL HELPER MACROS -- */

typedef void (*fff_function_t)(void);
typedef struct {
	fff_function_t call_history[FFF_CALL_HISTORY_LEN];
	unsigned int call_history_idx;
} fff_globals_t;

FFF_EXTERN_C
extern fff_globals_t fff;
FFF_END_EXTERN_C

#define DEFINE_FFF_GLOBALS                                                                         \
	FFF_EXTERN_C                                                                               \
	fff_globals_t fff;                                                                         \
	FFF_END_EXTERN_C

#define FFF_RESET_HISTORY()                                                                        \
	fff.call_history_idx = 0;                                                                  \
	memset(fff.call_history, 0, sizeof(fff.call_history));

#define REGISTER_CALL(function)                                                                    \
	if (fff.call_history_idx < FFF_CALL_HISTORY_LEN)                                           \
		fff.call_history[fff.call_history_idx++] = (fff_function_t)function;

#define DECLARE_FAKE_VOID_FUNC0(FUNCNAME)                                                          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(void);                                                         \
		void (**custom_fake_seq)(void);                                                    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(void);

#define DEFINE_FAKE_VOID_FUNC0(FUNCNAME)                                                           \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(void)                                            \
	{                                                                                          \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](); \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len -     \
							 1]();                                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake();                                             \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC0(FUNCNAME)                                                                  \
	DECLARE_FAKE_VOID_FUNC0(FUNCNAME)                                                          \
	DEFINE_FAKE_VOID_FUNC0(FUNCNAME)

#define DECLARE_FAKE_VOID_FUNC1(FUNCNAME, ARG0_TYPE)                                               \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0);                                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0);                                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0);

#define DEFINE_FAKE_VOID_FUNC1(FUNCNAME, ARG0_TYPE)                                                \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0)                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0);                                             \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0);                                             \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0);                                         \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC1(FUNCNAME, ARG0_TYPE)                                                       \
	DECLARE_FAKE_VOID_FUNC1(FUNCNAME, ARG0_TYPE)                                               \
	DEFINE_FAKE_VOID_FUNC1(FUNCNAME, ARG0_TYPE)

#define DECLARE_FAKE_VOID_FUNC2(FUNCNAME, ARG0_TYPE, ARG1_TYPE)                                    \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1);                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1);                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1);

#define DEFINE_FAKE_VOID_FUNC2(FUNCNAME, ARG0_TYPE, ARG1_TYPE)                                     \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1)                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1);                                       \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1);                                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1);                                   \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC2(FUNCNAME, ARG0_TYPE, ARG1_TYPE)                                            \
	DECLARE_FAKE_VOID_FUNC2(FUNCNAME, ARG0_TYPE, ARG1_TYPE)                                    \
	DEFINE_FAKE_VOID_FUNC2(FUNCNAME, ARG0_TYPE, ARG1_TYPE)

#define DECLARE_FAKE_VOID_FUNC3(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)                         \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2);               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2);

#define DEFINE_FAKE_VOID_FUNC3(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)                          \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2)  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2);                                 \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2);                                 \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2);                             \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC3(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)                                 \
	DECLARE_FAKE_VOID_FUNC3(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)                         \
	DEFINE_FAKE_VOID_FUNC3(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)

#define DECLARE_FAKE_VOID_FUNC4(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)              \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3);                                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3);                                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3);

#define DEFINE_FAKE_VOID_FUNC4(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)               \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3)                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3);                           \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3);                           \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3);                       \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC4(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)                      \
	DECLARE_FAKE_VOID_FUNC4(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)              \
	DEFINE_FAKE_VOID_FUNC4(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)

#define DECLARE_FAKE_VOID_FUNC5(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE)   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4);                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4);                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4);

#define DEFINE_FAKE_VOID_FUNC5(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE)    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4)                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4);                     \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4);                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4);                 \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC5(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE)           \
	DECLARE_FAKE_VOID_FUNC5(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE)   \
	DEFINE_FAKE_VOID_FUNC5(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE)

#define DECLARE_FAKE_VOID_FUNC6(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE)                                                         \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5);               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5);

#define DEFINE_FAKE_VOID_FUNC6(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE)                                                          \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5)  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5);               \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5);               \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5);           \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC6(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,           \
			ARG5_TYPE)                                                                 \
	DECLARE_FAKE_VOID_FUNC6(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE)                                                         \
	DEFINE_FAKE_VOID_FUNC6(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE)

#define DECLARE_FAKE_VOID_FUNC7(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE)                                              \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6);                                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6);                                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6);

#define DEFINE_FAKE_VOID_FUNC7(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE)                                               \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6)                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6);         \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6);         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6);     \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC7(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,           \
			ARG5_TYPE, ARG6_TYPE)                                                      \
	DECLARE_FAKE_VOID_FUNC7(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE)                                              \
	DEFINE_FAKE_VOID_FUNC7(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE)

#define DECLARE_FAKE_VOID_FUNC8(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                                   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7);                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7);                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7);

#define DEFINE_FAKE_VOID_FUNC8(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7)                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7);                                         \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC8(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,           \
			ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                                           \
	DECLARE_FAKE_VOID_FUNC8(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                                   \
	DEFINE_FAKE_VOID_FUNC8(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)

#define DECLARE_FAKE_VOID_FUNC9(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)                        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);

#define DEFINE_FAKE_VOID_FUNC9(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)                         \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8)  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8);                                             \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8);                                             \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8);                                   \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC9(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,           \
			ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)                                \
	DECLARE_FAKE_VOID_FUNC9(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)                        \
	DEFINE_FAKE_VOID_FUNC9(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)

#define DECLARE_FAKE_VOID_FUNC10(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9);                                               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9);                                          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9);

#define DEFINE_FAKE_VOID_FUNC10(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9)                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9);                                       \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9);                                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9);                             \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC10(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)                    \
	DECLARE_FAKE_VOID_FUNC10(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)            \
	DEFINE_FAKE_VOID_FUNC10(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)

#define DECLARE_FAKE_VOID_FUNC11(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10);                             \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10);                        \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10);

#define DEFINE_FAKE_VOID_FUNC11(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE) \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10)                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10);                                \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10);                                \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10);                      \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC11(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE)        \
	DECLARE_FAKE_VOID_FUNC11(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE)                                                       \
	DEFINE_FAKE_VOID_FUNC11(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE)

#define DECLARE_FAKE_VOID_FUNC12(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE)                                           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11);           \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11);      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11);

#define DEFINE_FAKE_VOID_FUNC12(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11)                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11);                         \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11);                         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11);               \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC12(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE)                                                               \
	DECLARE_FAKE_VOID_FUNC12(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE)                                           \
	DEFINE_FAKE_VOID_FUNC12(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE)

#define DECLARE_FAKE_VOID_FUNC13(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)                               \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12);                                             \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12);                                        \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12);

#define DEFINE_FAKE_VOID_FUNC13(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE)                                            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12)              \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12);                  \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12);                  \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12);        \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC13(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE)                                                   \
	DECLARE_FAKE_VOID_FUNC13(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)                               \
	DEFINE_FAKE_VOID_FUNC13(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE)

#define DECLARE_FAKE_VOID_FUNC14(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)                   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13);                           \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13);                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13);

#define DEFINE_FAKE_VOID_FUNC14(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)                                \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13)            \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13);           \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13);           \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13); \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC14(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)                                       \
	DECLARE_FAKE_VOID_FUNC14(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)                   \
	DEFINE_FAKE_VOID_FUNC14(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)

#define DECLARE_FAKE_VOID_FUNC15(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14);         \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14);    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12,              \
						  ARG13_TYPE arg13, ARG14_TYPE arg14);

#define DEFINE_FAKE_VOID_FUNC15(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12,              \
						  ARG13_TYPE arg13, ARG14_TYPE arg14)              \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14);    \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14);    \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14);                                        \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC15(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)                           \
	DECLARE_FAKE_VOID_FUNC15(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)       \
	DEFINE_FAKE_VOID_FUNC15(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)

#define DECLARE_FAKE_VOID_FUNC16(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15);                                             \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15);                                        \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15);

#define DEFINE_FAKE_VOID_FUNC16(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15)                                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15);                                            \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15);                                            \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15);                                 \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC16(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)               \
	DECLARE_FAKE_VOID_FUNC16(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE)                                                       \
	DEFINE_FAKE_VOID_FUNC16(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)

#define DECLARE_FAKE_VOID_FUNC17(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE)                                           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16);                           \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16);                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16);

#define DEFINE_FAKE_VOID_FUNC17(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16)                              \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16);                                     \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16);                                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16);                          \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC17(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE)   \
	DECLARE_FAKE_VOID_FUNC17(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE)                                           \
	DEFINE_FAKE_VOID_FUNC17(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE)

#define DECLARE_FAKE_VOID_FUNC18(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)                               \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);         \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);

#define DEFINE_FAKE_VOID_FUNC18(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE)                                            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17)            \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17);                              \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17);                              \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, arg17);                   \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC18(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,   \
			 ARG17_TYPE)                                                               \
	DECLARE_FAKE_VOID_FUNC18(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)                               \
	DEFINE_FAKE_VOID_FUNC18(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE)

#define DECLARE_FAKE_VOID_FUNC19(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)                   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,          \
				    ARG18_TYPE arg18);                                             \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,     \
					 ARG18_TYPE arg18);                                        \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18);

#define DEFINE_FAKE_VOID_FUNC19(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)                                \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18)                                                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18);                       \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18);                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, arg17, arg18);            \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC19(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,   \
			 ARG17_TYPE, ARG18_TYPE)                                                   \
	DECLARE_FAKE_VOID_FUNC19(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)                   \
	DEFINE_FAKE_VOID_FUNC19(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)

#define DECLARE_FAKE_VOID_FUNC20(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ARG(ARG19_TYPE, 19, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,          \
				    ARG18_TYPE arg18, ARG19_TYPE arg19);                           \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,     \
					 ARG18_TYPE arg18, ARG19_TYPE arg19);                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ARG19_TYPE arg19);

#define DEFINE_FAKE_VOID_FUNC20(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ARG19_TYPE arg19)                                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		SAVE_ARG(FUNCNAME, 19);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 19);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, arg19);                \
			} else {                                                                   \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, arg19);                \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, arg17, arg18, arg19);     \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC20(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,          \
			 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE,        \
			 ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,   \
			 ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)                                       \
	DECLARE_FAKE_VOID_FUNC20(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,  \
				 ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,            \
				 ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE,       \
				 ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)       \
	DEFINE_FAKE_VOID_FUNC20(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)

#define DECLARE_FAKE_VALUE_FUNC0(RETURN_TYPE, FUNCNAME)                                            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(void);                                                  \
		RETURN_TYPE (**custom_fake_seq)(void);                                             \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(void);

#define DEFINE_FAKE_VALUE_FUNC0(RETURN_TYPE, FUNCNAME)                                             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(void)                                     \
	{                                                                                          \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++]();         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1]();       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len -     \
							 1]();                                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake();                           \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake();                                      \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC0(RETURN_TYPE, FUNCNAME)                                                    \
	DECLARE_FAKE_VALUE_FUNC0(RETURN_TYPE, FUNCNAME)                                            \
	DEFINE_FAKE_VALUE_FUNC0(RETURN_TYPE, FUNCNAME)

#define DECLARE_FAKE_VALUE_FUNC1(RETURN_TYPE, FUNCNAME, ARG0_TYPE)                                 \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(ARG0_TYPE arg0);                                        \
		RETURN_TYPE (**custom_fake_seq)(ARG0_TYPE arg0);                                   \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0);

#define DEFINE_FAKE_VALUE_FUNC1(RETURN_TYPE, FUNCNAME, ARG0_TYPE)                                  \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0)                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](arg0);     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](arg0);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0);                                             \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0);                       \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0);                                  \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC1(RETURN_TYPE, FUNCNAME, ARG0_TYPE)                                         \
	DECLARE_FAKE_VALUE_FUNC1(RETURN_TYPE, FUNCNAME, ARG0_TYPE)                                 \
	DEFINE_FAKE_VALUE_FUNC1(RETURN_TYPE, FUNCNAME, ARG0_TYPE)

#define DECLARE_FAKE_VALUE_FUNC2(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE)                      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1);                        \
		RETURN_TYPE (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1);                   \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1);

#define DEFINE_FAKE_VALUE_FUNC2(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE)                       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1)           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1);                     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](arg0,    \
											  arg1);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1);                                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0, arg1);                 \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1);                            \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC2(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE)                              \
	DECLARE_FAKE_VALUE_FUNC2(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE)                      \
	DEFINE_FAKE_VALUE_FUNC2(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE)

#define DECLARE_FAKE_VALUE_FUNC3(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2);        \
		RETURN_TYPE (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2);   \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2);

#define DEFINE_FAKE_VALUE_FUNC3(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2)                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2);               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2);                         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2);                                 \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2);           \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2);                      \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC3(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)                   \
	DECLARE_FAKE_VALUE_FUNC3(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)           \
	DEFINE_FAKE_VALUE_FUNC3(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE)

#define DECLARE_FAKE_VALUE_FUNC4(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE)                                                        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3);                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3);                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3);

#define DEFINE_FAKE_VALUE_FUNC4(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE) \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3)           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3);         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3);                   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3);                           \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3);     \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3);                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC4(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)        \
	DECLARE_FAKE_VALUE_FUNC4(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE)                                                        \
	DEFINE_FAKE_VALUE_FUNC4(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE)

#define DECLARE_FAKE_VALUE_FUNC5(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE)                                             \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4);  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4);  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4);

#define DEFINE_FAKE_VALUE_FUNC5(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE)                                                         \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4)    \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4);             \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4);                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4);         \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4);          \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC5(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
			 ARG4_TYPE)                                                                \
	DECLARE_FAKE_VALUE_FUNC5(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE)                                             \
	DEFINE_FAKE_VALUE_FUNC5(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE)

#define DECLARE_FAKE_VALUE_FUNC6(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE)                                  \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5);                                                                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5);                                                                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5);

#define DEFINE_FAKE_VALUE_FUNC6(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE)                                              \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5)           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5);       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5);       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5);               \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5);   \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5);    \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC6(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
			 ARG4_TYPE, ARG5_TYPE)                                                     \
	DECLARE_FAKE_VALUE_FUNC6(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE)                                  \
	DEFINE_FAKE_VALUE_FUNC6(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE)

#define DECLARE_FAKE_VALUE_FUNC7(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE)                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6);                                                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6);                                                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6);

#define DEFINE_FAKE_VALUE_FUNC7(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE)                                   \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6)                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6);         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3,      \
								      arg4, arg5, arg6);           \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6);                                  \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC7(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
			 ARG4_TYPE, ARG5_TYPE, ARG6_TYPE)                                          \
	DECLARE_FAKE_VALUE_FUNC7(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE)                       \
	DEFINE_FAKE_VALUE_FUNC7(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE)

#define DECLARE_FAKE_VALUE_FUNC8(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7);                                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7);                                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7);

#define DEFINE_FAKE_VALUE_FUNC8(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7)           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7);               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7);                                     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3,      \
								      arg4, arg5, arg6, arg7);     \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7);                            \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC8(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
			 ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)                               \
	DECLARE_FAKE_VALUE_FUNC8(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)            \
	DEFINE_FAKE_VALUE_FUNC8(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE)

#define DECLARE_FAKE_VALUE_FUNC9(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE) \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8);

#define DEFINE_FAKE_VALUE_FUNC9(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8)                    \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8);         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8);                               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8);                                             \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(                             \
				arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);             \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8);                      \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC9(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
			 ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)                    \
	DECLARE_FAKE_VALUE_FUNC9(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE) \
	DEFINE_FAKE_VALUE_FUNC9(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE)

#define DECLARE_FAKE_VALUE_FUNC10(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE)                                            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9);  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9);  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9);

#define DEFINE_FAKE_VALUE_FUNC10(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9)    \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8, arg9);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9);                         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9);                                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(                             \
				arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);       \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9);                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC10(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE)        \
	DECLARE_FAKE_VALUE_FUNC10(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE)                                            \
	DEFINE_FAKE_VALUE_FUNC10(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE)

#define DECLARE_FAKE_VALUE_FUNC11(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE)                                \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10);                                                                \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10);                                                                \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10);

#define DEFINE_FAKE_VALUE_FUNC11(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE)                                            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10)                                                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10);                  \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10);                  \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10);                                \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10);        \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10);         \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC11(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE)                                                              \
	DECLARE_FAKE_VALUE_FUNC11(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE)                                \
	DEFINE_FAKE_VALUE_FUNC11(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE)

#define DECLARE_FAKE_VALUE_FUNC12(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE)                    \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11);                                              \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11);

#define DEFINE_FAKE_VALUE_FUNC12(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE)                                \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11)                                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11);                         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11); \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11);  \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC12(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE)                                                  \
	DECLARE_FAKE_VALUE_FUNC12(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE)                    \
	DEFINE_FAKE_VALUE_FUNC12(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE)

#define DECLARE_FAKE_VALUE_FUNC13(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12);                            \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12);                            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12);

#define DEFINE_FAKE_VALUE_FUNC13(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12)                              \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12);    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12);    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12);                  \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11,  \
							    arg12);                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12);                                 \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC13(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)                                      \
	DECLARE_FAKE_VALUE_FUNC13(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)        \
	DEFINE_FAKE_VALUE_FUNC13(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE)

#define DECLARE_FAKE_VALUE_FUNC14(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE)                                                      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13);          \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13);

#define DEFINE_FAKE_VALUE_FUNC14(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13)            \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8, arg9,    \
								  arg10, arg11, arg12, arg13);     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13);                                    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13);           \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11,  \
							    arg12, arg13);                         \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13);                          \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC14(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)                          \
	DECLARE_FAKE_VALUE_FUNC14(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE)                                                      \
	DEFINE_FAKE_VALUE_FUNC14(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE)

#define DECLARE_FAKE_VALUE_FUNC15(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE)                                          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14);                                                                \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14);                                                                \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14);

#define DEFINE_FAKE_VALUE_FUNC15(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE)                                                       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14)                                                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14);                             \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14);                             \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14);    \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11,  \
							    arg12, arg13, arg14);                  \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14);                   \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC15(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE)              \
	DECLARE_FAKE_VALUE_FUNC15(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE)                                          \
	DEFINE_FAKE_VALUE_FUNC15(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE)

#define DECLARE_FAKE_VALUE_FUNC16(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)                              \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15);                                              \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15);

#define DEFINE_FAKE_VALUE_FUNC16(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE)                                           \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15)                                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15);                      \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15);                      \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15);                                            \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11,  \
							    arg12, arg13, arg14, arg15);           \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14, arg15);            \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC16(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)  \
	DECLARE_FAKE_VALUE_FUNC16(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE)                              \
	DEFINE_FAKE_VALUE_FUNC16(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE)

#define DECLARE_FAKE_VALUE_FUNC17(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE)                  \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16);                            \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16);                            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16);

#define DEFINE_FAKE_VALUE_FUNC17(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE)                               \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16)                              \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16);               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16);               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16);                                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret =                                                          \
				FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,    \
							    arg6, arg7, arg8, arg9, arg10, arg11,  \
							    arg12, arg13, arg14, arg15, arg16);    \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14, arg15, arg16);     \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC17(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,  \
			  ARG16_TYPE)                                                              \
	DECLARE_FAKE_VALUE_FUNC17(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE)                  \
	DEFINE_FAKE_VALUE_FUNC17(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE)

#define DECLARE_FAKE_VALUE_FUNC18(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);          \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17);

#define DEFINE_FAKE_VALUE_FUNC18(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)                   \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17)            \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17);        \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17);        \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17);                              \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(                             \
				arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, \
				arg11, arg12, arg13, arg14, arg15, arg16, arg17);                  \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14, arg15, arg16,      \
							   arg17);                                 \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC18(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,  \
			  ARG16_TYPE, ARG17_TYPE)                                                  \
	DECLARE_FAKE_VALUE_FUNC18(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)      \
	DEFINE_FAKE_VALUE_FUNC18(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE)

#define DECLARE_FAKE_VALUE_FUNC19(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,      \
				  ARG18_TYPE)                                                      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18);                                                                \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18);                                                                \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18);

#define DEFINE_FAKE_VALUE_FUNC19(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18)                                                                  \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18);                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(                             \
				arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, \
				arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18);           \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14, arg15, arg16,      \
							   arg17, arg18);                          \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC19(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,  \
			  ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)                                      \
	DECLARE_FAKE_VALUE_FUNC19(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,      \
				  ARG18_TYPE)                                                      \
	DEFINE_FAKE_VALUE_FUNC19(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE)

#define DECLARE_FAKE_VALUE_FUNC20(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,      \
				  ARG18_TYPE, ARG19_TYPE)                                          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ARG(ARG19_TYPE, 19, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18, ARG19_TYPE arg19);                                              \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18, ARG19_TYPE arg19);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ARG19_TYPE arg19);

#define DEFINE_FAKE_VALUE_FUNC20(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE,       \
				 ARG19_TYPE)                                                       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ARG19_TYPE arg19)                                                \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		SAVE_ARG(FUNCNAME, 19);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 19);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18,  \
							arg19);                                    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
			} else {                                                                   \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18,  \
							arg19);                                    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, arg19);                \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret = FUNCNAME##_fake.custom_fake(                             \
				arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, \
				arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19);    \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
			return FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,     \
							   arg6, arg7, arg8, arg9, arg10, arg11,   \
							   arg12, arg13, arg14, arg15, arg16,      \
							   arg17, arg18, arg19);                   \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC20(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
			  ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE,        \
			  ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,  \
			  ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ARG19_TYPE)                          \
	DECLARE_FAKE_VALUE_FUNC20(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,          \
				  ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,           \
				  ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,        \
				  ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,      \
				  ARG18_TYPE, ARG19_TYPE)                                          \
	DEFINE_FAKE_VALUE_FUNC20(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE,       \
				 ARG19_TYPE)

#define DECLARE_FAKE_VOID_FUNC2_VARARG(FUNCNAME, ARG0_TYPE, ...)                                   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, va_list ap);                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, va_list ap);                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ...);

#define DEFINE_FAKE_VOID_FUNC2_VARARG(FUNCNAME, ARG0_TYPE, ...)                                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ...)                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg0);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, ap);                                         \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg0);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, ap);                                         \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg0);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, ap);                                     \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC2_VARARG(FUNCNAME, ARG0_TYPE, ...)                                           \
	DECLARE_FAKE_VOID_FUNC2_VARARG(FUNCNAME, ARG0_TYPE, ...)                                   \
	DEFINE_FAKE_VOID_FUNC2_VARARG(FUNCNAME, ARG0_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC3_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)                        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, va_list ap);                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, va_list ap);              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ...);

#define DEFINE_FAKE_VOID_FUNC3_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)                         \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ...)             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg1);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, ap);                                   \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg1);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, ap);                                   \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg1);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, ap);                               \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC3_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)                                \
	DECLARE_FAKE_VOID_FUNC3_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)                        \
	DEFINE_FAKE_VOID_FUNC3_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC4_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)             \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, va_list ap);   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ...);

#define DEFINE_FAKE_VOID_FUNC4_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)              \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ...)                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg2);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, ap);                             \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg2);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, ap);                             \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg2);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, ap);                         \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC4_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)                     \
	DECLARE_FAKE_VOID_FUNC4_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)             \
	DEFINE_FAKE_VOID_FUNC4_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC5_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ...)  \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, va_list ap);                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, va_list ap);                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ...);

#define DEFINE_FAKE_VOID_FUNC5_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ...)   \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ...)                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg3);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, ap);                       \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg3);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, ap);                       \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg3);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, ap);                   \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC5_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ...)          \
	DECLARE_FAKE_VOID_FUNC5_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ...)  \
	DEFINE_FAKE_VOID_FUNC5_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC6_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ...)                                             \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, va_list ap);                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, va_list ap);              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ...);

#define DEFINE_FAKE_VOID_FUNC6_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ...)                                              \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ...)             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg4);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, ap);                 \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg4);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, ap);                 \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg4);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, ap);             \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC6_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ...)                                                                \
	DECLARE_FAKE_VOID_FUNC6_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ...)                                             \
	DEFINE_FAKE_VOID_FUNC6_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC7_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ...)                                  \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5, va_list ap);   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ...);

#define DEFINE_FAKE_VOID_FUNC7_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ...)                                   \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ...)                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg5);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, ap);           \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg5);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, ap);           \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg5);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, ap);       \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC7_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ...)                                                     \
	DECLARE_FAKE_VOID_FUNC7_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ...)                                  \
	DEFINE_FAKE_VOID_FUNC7_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC8_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, va_list ap);                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, va_list ap);                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ...);

#define DEFINE_FAKE_VOID_FUNC8_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ...)                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg6);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, ap);     \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg6);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, ap);     \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg6);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6, ap); \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC8_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ...)                                          \
	DECLARE_FAKE_VOID_FUNC8_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)                       \
	DEFINE_FAKE_VOID_FUNC8_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC9_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, va_list ap);                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, va_list ap);              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ...);

#define DEFINE_FAKE_VOID_FUNC9_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ...)             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg7);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						ap);                                               \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg7);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						ap);                                               \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg7);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, ap);                                     \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC9_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,    \
			       ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)                               \
	DECLARE_FAKE_VOID_FUNC9_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)            \
	DEFINE_FAKE_VOID_FUNC9_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,        \
				      ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC10_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					...)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, va_list ap);   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ...);

#define DEFINE_FAKE_VOID_FUNC10_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ...) \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ...)                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg8);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, ap);                                         \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg8);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, ap);                                         \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg8);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, ap);                               \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC10_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ...)                   \
	DECLARE_FAKE_VOID_FUNC10_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					...)                                                       \
	DEFINE_FAKE_VOID_FUNC10_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC11_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ...)                                            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, va_list ap);                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, va_list ap);                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ...);

#define DEFINE_FAKE_VOID_FUNC11_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ...)                                             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ...)                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg9);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, ap);                                   \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg9);                                                \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, ap);                                   \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg9);                                                        \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, ap);                         \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC11_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ...)        \
	DECLARE_FAKE_VOID_FUNC11_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ...)                                            \
	DEFINE_FAKE_VOID_FUNC11_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC12_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ...)                                \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, va_list ap);                 \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, va_list ap);            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10, ...);

#define DEFINE_FAKE_VOID_FUNC12_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ...)                                 \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10, ...)           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg10);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, ap);                            \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg10);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, ap);                            \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg10);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, ap);                  \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC12_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				...)                                                               \
	DECLARE_FAKE_VOID_FUNC12_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ...)                                \
	DEFINE_FAKE_VOID_FUNC12_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC13_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)                    \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    va_list ap);                                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ...);

#define DEFINE_FAKE_VOID_FUNC13_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)                     \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ...)                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg11);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, ap);                     \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg11);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, ap);                     \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg11);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, ap);           \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC13_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ...)                                                   \
	DECLARE_FAKE_VOID_FUNC13_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)                    \
	DEFINE_FAKE_VOID_FUNC13_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC14_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ...)        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, va_list ap);                                 \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, va_list ap);                            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12, ...);

#define DEFINE_FAKE_VOID_FUNC14_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ...)         \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12, ...)         \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg12);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, ap);              \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg12);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, ap);              \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg12);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, ap);    \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC14_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ...)                                       \
	DECLARE_FAKE_VOID_FUNC14_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ...)        \
	DEFINE_FAKE_VOID_FUNC14_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC15_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					...)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, va_list ap);               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, va_list ap);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13, ...);

#define DEFINE_FAKE_VOID_FUNC15_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ...)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13, ...)       \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg13);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, ap);       \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg13);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, ap);       \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg13);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    ap);                                           \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC15_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ...)                           \
	DECLARE_FAKE_VOID_FUNC15_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					...)                                                       \
	DEFINE_FAKE_VOID_FUNC15_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ...)

#define DECLARE_FAKE_VOID_FUNC16_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ...)                                           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    va_list ap);                                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12,              \
						  ARG13_TYPE arg13, ARG14_TYPE arg14, ...);

#define DEFINE_FAKE_VOID_FUNC16_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ...)                                            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,  \
						  ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,  \
						  ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,  \
						  ARG9_TYPE arg9, ARG10_TYPE arg10,                \
						  ARG11_TYPE arg11, ARG12_TYPE arg12,              \
						  ARG13_TYPE arg13, ARG14_TYPE arg14, ...)         \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg14);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						ap);                                               \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg14);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						ap);                                               \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg14);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, ap);                                    \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC16_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ...)               \
	DECLARE_FAKE_VOID_FUNC16_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ...)                                           \
	DEFINE_FAKE_VOID_FUNC16_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC17_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ...)                               \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, va_list ap);                                 \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, va_list ap);                            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ...);

#define DEFINE_FAKE_VOID_FUNC17_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ...)                                \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ...)                                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg15);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, ap);                                        \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg15);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, ap);                                        \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg15);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, ap);                             \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC17_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ...)   \
	DECLARE_FAKE_VOID_FUNC17_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ...)                               \
	DEFINE_FAKE_VOID_FUNC17_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC18_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)                   \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, va_list ap);               \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, va_list ap);          \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ...);

#define DEFINE_FAKE_VOID_FUNC18_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)                    \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ...)                         \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg16);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, ap);                                 \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg16);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, ap);                                 \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg16);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, ap);                      \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC18_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ...)                                                   \
	DECLARE_FAKE_VOID_FUNC18_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)                   \
	DEFINE_FAKE_VOID_FUNC18_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC19_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ...)       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,          \
				    va_list ap);                                                   \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,     \
					 va_list ap);                                              \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17, ...);

#define DEFINE_FAKE_VOID_FUNC19_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ...)        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17, ...)       \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg17);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, ap);                          \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg17);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, ap);                          \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg17);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, arg17, ap);               \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC19_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ...)                                       \
	DECLARE_FAKE_VOID_FUNC19_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ...)       \
	DEFINE_FAKE_VOID_FUNC19_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ...)

#define DECLARE_FAKE_VOID_FUNC20_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,            \
					ARG18_TYPE, ...)                                           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		void (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,                \
				    ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,                \
				    ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,                \
				    ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,            \
				    ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,          \
				    ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,          \
				    ARG18_TYPE arg18, va_list ap);                                 \
		void (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2,           \
					 ARG3_TYPE arg3, ARG4_TYPE arg4, ARG5_TYPE arg5,           \
					 ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8,           \
					 ARG9_TYPE arg9, ARG10_TYPE arg10, ARG11_TYPE arg11,       \
					 ARG12_TYPE arg12, ARG13_TYPE arg13, ARG14_TYPE arg14,     \
					 ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,     \
					 ARG18_TYPE arg18, va_list ap);                            \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ...);

#define DEFINE_FAKE_VOID_FUNC20_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, \
				       ...)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	void FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                                 \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ...)                                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg18);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_idx++](   \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, ap);                   \
				va_end(ap);                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg18);                                               \
				FUNCNAME##_fake                                                    \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, ap);                   \
				va_end(ap);                                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			va_list ap;                                                                \
			va_start(ap, arg18);                                                       \
			FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, arg6,      \
						    arg7, arg8, arg9, arg10, arg11, arg12, arg13,  \
						    arg14, arg15, arg16, arg17, arg18, ap);        \
			va_end(ap);                                                                \
		}                                                                                  \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VOID_FUNC20_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE,   \
				ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, \
				ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ARG14_TYPE, ARG15_TYPE,        \
				ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ...)                           \
	DECLARE_FAKE_VOID_FUNC20_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,      \
					ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,     \
					ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, \
					ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE,            \
					ARG18_TYPE, ...)                                           \
	DEFINE_FAKE_VOID_FUNC20_VARARG(FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE,       \
				       ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE,      \
				       ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,  \
				       ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, \
				       ...)

#define DECLARE_FAKE_VALUE_FUNC2_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ...)                     \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(ARG0_TYPE arg0, va_list ap);                            \
		RETURN_TYPE (**custom_fake_seq)(ARG0_TYPE arg0, va_list ap);                       \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ...);

#define DEFINE_FAKE_VALUE_FUNC2_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ...)                      \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ...)                      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg0);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](arg0, ap); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg0);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](arg0,    \
											  ap);     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, ap);                                         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg0);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, ap);                               \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC2_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ...)                             \
	DECLARE_FAKE_VALUE_FUNC2_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ...)                     \
	DEFINE_FAKE_VALUE_FUNC2_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC3_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)(ARG0_TYPE arg0, ARG1_TYPE arg1, va_list ap);            \
		RETURN_TYPE (**custom_fake_seq)(ARG0_TYPE arg0, ARG1_TYPE arg1, va_list ap);       \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ...);

#define DEFINE_FAKE_VALUE_FUNC3_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)           \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1, ...)      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg1);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, ap);                 \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg1);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, ap);                           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, ap);                                   \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg1);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, ap);                         \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC3_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)                  \
	DECLARE_FAKE_VALUE_FUNC3_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)          \
	DEFINE_FAKE_VALUE_FUNC3_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC4_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					...)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, va_list ap);                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, va_list ap);                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ...);

#define DEFINE_FAKE_VALUE_FUNC4_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ...)                                                        \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ...)                      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg2);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, ap);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg2);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, ap);                     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, ap);                             \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg2);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, ap);                   \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC4_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)       \
	DECLARE_FAKE_VALUE_FUNC4_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					...)                                                       \
	DEFINE_FAKE_VALUE_FUNC4_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC5_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ...)                                            \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, va_list ap);      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, va_list ap);      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3, ...);

#define DEFINE_FAKE_VALUE_FUNC5_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ...)                                             \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3, ...)      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg3);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, ap);     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg3);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, ap);               \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, ap);                       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg3);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, ap);             \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC5_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				...)                                                               \
	DECLARE_FAKE_VALUE_FUNC5_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ...)                                            \
	DEFINE_FAKE_VALUE_FUNC5_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC6_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ...)                                 \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 va_list ap);                                                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 va_list ap);                                                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ...);

#define DEFINE_FAKE_VALUE_FUNC6_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ...)                                  \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ...)                      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg4);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, ap);         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg4);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, ap);         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, ap);                 \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg4);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, ap);       \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC6_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ...)                                                    \
	DECLARE_FAKE_VALUE_FUNC6_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ...)                                 \
	DEFINE_FAKE_VALUE_FUNC6_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC7_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ...)                      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, va_list ap);                                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, va_list ap);                                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5, ...);

#define DEFINE_FAKE_VALUE_FUNC7_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ...)                       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5, ...)      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg5);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, ap);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg5);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, ap);   \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, ap);           \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg5);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5, ap); \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC7_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ...)                                         \
	DECLARE_FAKE_VALUE_FUNC7_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ...)                      \
	DEFINE_FAKE_VALUE_FUNC7_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC8_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, va_list ap);                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, va_list ap);                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ...);

#define DEFINE_FAKE_VALUE_FUNC8_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ...)                      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg6);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, ap);                 \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg6);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							ap);                                       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, ap);     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg6);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, ap);                               \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC8_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)                              \
	DECLARE_FAKE_VALUE_FUNC8_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)           \
	DEFINE_FAKE_VALUE_FUNC8_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC9_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					...)                                                       \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, va_list ap);                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, va_list ap);                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7, ...);

#define DEFINE_FAKE_VALUE_FUNC9_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...) \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7, ...)      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg7);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, ap);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg7);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, ap);                                 \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						ap);                                               \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg7);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, ap);                         \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC9_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, \
				ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)                   \
	DECLARE_FAKE_VALUE_FUNC9_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					...)                                                       \
	DEFINE_FAKE_VALUE_FUNC9_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,     \
				       ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC10_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ...)                                           \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, va_list ap);      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, va_list ap);      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ...);

#define DEFINE_FAKE_VALUE_FUNC10_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ...)                                            \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ...)               \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg8);                                                \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8, ap);     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg8);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, ap);                           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, ap);                                         \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg8);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, ap);                   \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC10_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ...)                                                              \
	DECLARE_FAKE_VALUE_FUNC10_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ...)                                           \
	DEFINE_FAKE_VALUE_FUNC10_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC11_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ...)                                \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 va_list ap);                                                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 va_list ap);                                                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7,           \
							 ARG8_TYPE arg8, ARG9_TYPE arg9, ...);

#define DEFINE_FAKE_VALUE_FUNC11_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ...)                                 \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(ARG0_TYPE arg0, ARG1_TYPE arg1,           \
							 ARG2_TYPE arg2, ARG3_TYPE arg3,           \
							 ARG4_TYPE arg4, ARG5_TYPE arg5,           \
							 ARG6_TYPE arg6, ARG7_TYPE arg7,           \
							 ARG8_TYPE arg8, ARG9_TYPE arg9, ...)      \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg9);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, ap);                     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg9);                                                \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, ap);                     \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, ap);                                   \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg9);                                                        \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, ap);             \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC11_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ...)                                                   \
	DECLARE_FAKE_VALUE_FUNC11_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ...)                                \
	DEFINE_FAKE_VALUE_FUNC11_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC12_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ...)                    \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, va_list ap);                                                    \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, va_list ap);                                                    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ...);

#define DEFINE_FAKE_VALUE_FUNC12_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ...)                     \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ...)                                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg10);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, ap);              \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg10);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, ap);              \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, ap);                            \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg10);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, ap);      \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC12_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ...)                                       \
	DECLARE_FAKE_VALUE_FUNC12_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ...)                    \
	DEFINE_FAKE_VALUE_FUNC12_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC13_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)        \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, va_list ap);                                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, va_list ap);                                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ...);

#define DEFINE_FAKE_VALUE_FUNC13_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)         \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ...)                                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg11);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, ap);       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg11);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, ap);       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, ap);                     \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg11);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  ap);                                     \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC13_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)                           \
	DECLARE_FAKE_VALUE_FUNC13_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)        \
	DEFINE_FAKE_VALUE_FUNC13_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC14_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ...)                                                      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, va_list ap);                \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, va_list ap);                \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ...);

#define DEFINE_FAKE_VALUE_FUNC14_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					...)                                                       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ...)                         \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg12);                                               \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8, arg9,    \
								  arg10, arg11, arg12, ap);        \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg12);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							ap);                                       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, ap);              \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg12);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, ap);                              \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC14_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ...)               \
	DECLARE_FAKE_VALUE_FUNC14_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ...)                                                      \
	DEFINE_FAKE_VALUE_FUNC14_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					...)

#define DECLARE_FAKE_VALUE_FUNC15_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ...)                                          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 va_list ap);                                                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 va_list ap);                                                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13, ...);

#define DEFINE_FAKE_VALUE_FUNC15_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ...)                                           \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13, ...)       \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg13);                                               \
				RETURN_TYPE ret = FUNCNAME##_fake.custom_fake_seq                  \
							  [FUNCNAME##_fake.custom_fake_seq_idx++]( \
								  arg0, arg1, arg2, arg3, arg4,    \
								  arg5, arg6, arg7, arg8, arg9,    \
								  arg10, arg11, arg12, arg13, ap); \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg13);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, ap);                                \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, ap);       \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg13);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, ap);                       \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC15_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE, ...)   \
	DECLARE_FAKE_VALUE_FUNC15_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ...)                                          \
	DEFINE_FAKE_VALUE_FUNC15_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC16_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ...)                              \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, va_list ap);                                                    \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, va_list ap);                                                    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ...);

#define DEFINE_FAKE_VALUE_FUNC16_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ...)                               \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ...)                                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg14);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, ap);                         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg14);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, ap);                         \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						ap);                                               \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg14);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, arg14, ap);                \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC16_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ...)                                                  \
	DECLARE_FAKE_VALUE_FUNC16_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ...)                              \
	DEFINE_FAKE_VALUE_FUNC16_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC17_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ...)                  \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, va_list ap);                                  \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, va_list ap);                                  \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ...);

#define DEFINE_FAKE_VALUE_FUNC17_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ...)                   \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ...)                                           \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg15);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, ap);                  \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg15);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, ap);                  \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, ap);                                        \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg15);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, arg14, arg15, ap);         \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC17_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ...)                                      \
	DECLARE_FAKE_VALUE_FUNC17_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ...)                  \
	DEFINE_FAKE_VALUE_FUNC17_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC18_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)      \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, va_list ap);                \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, va_list ap);                \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ...);

#define DEFINE_FAKE_VALUE_FUNC18_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)       \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ...)                         \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg16);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, ap);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg16);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, ap);           \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, ap);                                 \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg16);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, arg14, arg15, arg16, ap);  \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC18_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)                          \
	DECLARE_FAKE_VALUE_FUNC18_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)      \
	DEFINE_FAKE_VALUE_FUNC18_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC19_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,           \
					 ARG17_TYPE, ...)                                          \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 va_list ap);                                                                      \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 va_list ap);                                                                      \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17, ...);

#define DEFINE_FAKE_VALUE_FUNC19_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,            \
					ARG17_TYPE, ...)                                           \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17, ...)       \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg17);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, ap);    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg17);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, ap);    \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, ap);                          \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg17);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, arg14, arg15, arg16,       \
							  arg17, ap);                              \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC19_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ...)              \
	DECLARE_FAKE_VALUE_FUNC19_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,           \
					 ARG17_TYPE, ...)                                          \
	DEFINE_FAKE_VALUE_FUNC19_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,            \
					ARG17_TYPE, ...)

#define DECLARE_FAKE_VALUE_FUNC20_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,           \
					 ARG17_TYPE, ARG18_TYPE, ...)                              \
	typedef struct FUNCNAME##_Fake {                                                           \
		DECLARE_ARG(ARG0_TYPE, 0, FUNCNAME)                                                \
		DECLARE_ARG(ARG1_TYPE, 1, FUNCNAME)                                                \
		DECLARE_ARG(ARG2_TYPE, 2, FUNCNAME)                                                \
		DECLARE_ARG(ARG3_TYPE, 3, FUNCNAME)                                                \
		DECLARE_ARG(ARG4_TYPE, 4, FUNCNAME)                                                \
		DECLARE_ARG(ARG5_TYPE, 5, FUNCNAME)                                                \
		DECLARE_ARG(ARG6_TYPE, 6, FUNCNAME)                                                \
		DECLARE_ARG(ARG7_TYPE, 7, FUNCNAME)                                                \
		DECLARE_ARG(ARG8_TYPE, 8, FUNCNAME)                                                \
		DECLARE_ARG(ARG9_TYPE, 9, FUNCNAME)                                                \
		DECLARE_ARG(ARG10_TYPE, 10, FUNCNAME)                                              \
		DECLARE_ARG(ARG11_TYPE, 11, FUNCNAME)                                              \
		DECLARE_ARG(ARG12_TYPE, 12, FUNCNAME)                                              \
		DECLARE_ARG(ARG13_TYPE, 13, FUNCNAME)                                              \
		DECLARE_ARG(ARG14_TYPE, 14, FUNCNAME)                                              \
		DECLARE_ARG(ARG15_TYPE, 15, FUNCNAME)                                              \
		DECLARE_ARG(ARG16_TYPE, 16, FUNCNAME)                                              \
		DECLARE_ARG(ARG17_TYPE, 17, FUNCNAME)                                              \
		DECLARE_ARG(ARG18_TYPE, 18, FUNCNAME)                                              \
		DECLARE_ALL_FUNC_COMMON                                                            \
		DECLARE_VALUE_FUNCTION_VARIABLES(RETURN_TYPE)                                      \
		DECLARE_RETURN_VALUE_HISTORY(RETURN_TYPE)                                          \
		DECLARE_CUSTOM_FAKE_SEQ_VARIABLES                                                  \
		RETURN_TYPE (*custom_fake)                                                         \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18, va_list ap);                                                    \
		RETURN_TYPE (**custom_fake_seq)                                                    \
		(ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,   \
		 ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,   \
		 ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,           \
		 ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,           \
		 ARG18_TYPE arg18, va_list ap);                                                    \
	} FUNCNAME##_Fake;                                                                         \
	extern FUNCNAME##_Fake FUNCNAME##_fake;                                                    \
	void FUNCNAME##_reset(void);                                                               \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ...);

#define DEFINE_FAKE_VALUE_FUNC20_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,            \
					ARG17_TYPE, ARG18_TYPE, ...)                               \
	FUNCNAME##_Fake FUNCNAME##_fake;                                                           \
	RETURN_TYPE FFF_GCC_FUNCTION_ATTRIBUTES FUNCNAME(                                          \
		ARG0_TYPE arg0, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3, ARG4_TYPE arg4,    \
		ARG5_TYPE arg5, ARG6_TYPE arg6, ARG7_TYPE arg7, ARG8_TYPE arg8, ARG9_TYPE arg9,    \
		ARG10_TYPE arg10, ARG11_TYPE arg11, ARG12_TYPE arg12, ARG13_TYPE arg13,            \
		ARG14_TYPE arg14, ARG15_TYPE arg15, ARG16_TYPE arg16, ARG17_TYPE arg17,            \
		ARG18_TYPE arg18, ...)                                                             \
	{                                                                                          \
		SAVE_ARG(FUNCNAME, 0);                                                             \
		SAVE_ARG(FUNCNAME, 1);                                                             \
		SAVE_ARG(FUNCNAME, 2);                                                             \
		SAVE_ARG(FUNCNAME, 3);                                                             \
		SAVE_ARG(FUNCNAME, 4);                                                             \
		SAVE_ARG(FUNCNAME, 5);                                                             \
		SAVE_ARG(FUNCNAME, 6);                                                             \
		SAVE_ARG(FUNCNAME, 7);                                                             \
		SAVE_ARG(FUNCNAME, 8);                                                             \
		SAVE_ARG(FUNCNAME, 9);                                                             \
		SAVE_ARG(FUNCNAME, 10);                                                            \
		SAVE_ARG(FUNCNAME, 11);                                                            \
		SAVE_ARG(FUNCNAME, 12);                                                            \
		SAVE_ARG(FUNCNAME, 13);                                                            \
		SAVE_ARG(FUNCNAME, 14);                                                            \
		SAVE_ARG(FUNCNAME, 15);                                                            \
		SAVE_ARG(FUNCNAME, 16);                                                            \
		SAVE_ARG(FUNCNAME, 17);                                                            \
		SAVE_ARG(FUNCNAME, 18);                                                            \
		if (ROOM_FOR_MORE_HISTORY(FUNCNAME)) {                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 0);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 1);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 2);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 3);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 4);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 5);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 6);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 7);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 8);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 9);                                             \
			SAVE_ARG_HISTORY(FUNCNAME, 10);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 11);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 12);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 13);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 14);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 15);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 16);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 17);                                            \
			SAVE_ARG_HISTORY(FUNCNAME, 18);                                            \
		} else {                                                                           \
			HISTORY_DROPPED(FUNCNAME);                                                 \
		}                                                                                  \
		INCREMENT_CALL_COUNT(FUNCNAME);                                                    \
		REGISTER_CALL(FUNCNAME);                                                           \
		if (FUNCNAME##_fake.custom_fake_seq_len) { /* a sequence of custom fakes */        \
			if (FUNCNAME##_fake.custom_fake_seq_idx <                                  \
			    FUNCNAME##_fake.custom_fake_seq_len) {                                 \
				va_list ap;                                                        \
				va_start(ap, arg18);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_idx++](           \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18,  \
							ap);                                       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
			} else {                                                                   \
				va_list ap;                                                        \
				va_start(ap, arg18);                                               \
				RETURN_TYPE ret =                                                  \
					FUNCNAME##_fake.custom_fake_seq                            \
						[FUNCNAME##_fake.custom_fake_seq_len - 1](         \
							arg0, arg1, arg2, arg3, arg4, arg5, arg6,  \
							arg7, arg8, arg9, arg10, arg11, arg12,     \
							arg13, arg14, arg15, arg16, arg17, arg18,  \
							ap);                                       \
				SAVE_RET_HISTORY(FUNCNAME, ret);                                   \
				va_end(ap);                                                        \
				return ret;                                                        \
				return FUNCNAME##_fake                                             \
					.custom_fake_seq[FUNCNAME##_fake.custom_fake_seq_len - 1]( \
						arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,    \
						arg8, arg9, arg10, arg11, arg12, arg13, arg14,     \
						arg15, arg16, arg17, arg18, ap);                   \
			}                                                                          \
		}                                                                                  \
		if (FUNCNAME##_fake.custom_fake) {                                                 \
			RETURN_TYPE ret;                                                           \
			va_list ap;                                                                \
			va_start(ap, arg18);                                                       \
			ret = FUNCNAME##_fake.custom_fake(arg0, arg1, arg2, arg3, arg4, arg5,      \
							  arg6, arg7, arg8, arg9, arg10, arg11,    \
							  arg12, arg13, arg14, arg15, arg16,       \
							  arg17, arg18, ap);                       \
			va_end(ap);                                                                \
			SAVE_RET_HISTORY(FUNCNAME, ret);                                           \
			return ret;                                                                \
		}                                                                                  \
		RETURN_FAKE_RESULT(FUNCNAME)                                                       \
	}                                                                                          \
	DEFINE_RESET_FUNCTION(FUNCNAME)

#define FAKE_VALUE_FUNC20_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,           \
				 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE, \
				 ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, ARG13_TYPE,        \
				 ARG14_TYPE, ARG15_TYPE, ARG16_TYPE, ARG17_TYPE, ARG18_TYPE, ...)  \
	DECLARE_FAKE_VALUE_FUNC20_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,   \
					 ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,    \
					 ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE, \
					 ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,           \
					 ARG17_TYPE, ARG18_TYPE, ...)                              \
	DEFINE_FAKE_VALUE_FUNC20_VARARG(RETURN_TYPE, FUNCNAME, ARG0_TYPE, ARG1_TYPE, ARG2_TYPE,    \
					ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE,     \
					ARG8_TYPE, ARG9_TYPE, ARG10_TYPE, ARG11_TYPE, ARG12_TYPE,  \
					ARG13_TYPE, ARG14_TYPE, ARG15_TYPE, ARG16_TYPE,            \
					ARG17_TYPE, ARG18_TYPE, ...)

/* MSVC expand macro fix */
#define EXPAND(x) x

#define PP_NARG_MINUS2(...) EXPAND(PP_NARG_MINUS2_(__VA_ARGS__, PP_RSEQ_N_MINUS2()))

#define PP_NARG_MINUS2_(...) EXPAND(PP_ARG_MINUS2_N(__VA_ARGS__))

#define PP_ARG_MINUS2_N(returnVal, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,     \
			_14, _15, _16, _17, _18, _19, _20, N, ...)                                 \
	N

#define PP_RSEQ_N_MINUS2() 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define PP_NARG_MINUS1(...) EXPAND(PP_NARG_MINUS1_(__VA_ARGS__, PP_RSEQ_N_MINUS1()))

#define PP_NARG_MINUS1_(...) EXPAND(PP_ARG_MINUS1_N(__VA_ARGS__))

#define PP_ARG_MINUS1_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
			_17, _18, _19, _20, N, ...)                                                \
	N

#define PP_RSEQ_N_MINUS1() 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/* DECLARE AND DEFINE FAKE FUNCTIONS - PLACE IN TEST FILES */

#define FAKE_VALUE_FUNC(...) EXPAND(FUNC_VALUE_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define FUNC_VALUE_(N, ...) EXPAND(FUNC_VALUE_N(N, __VA_ARGS__))

#define FUNC_VALUE_N(N, ...) EXPAND(FAKE_VALUE_FUNC##N(__VA_ARGS__))

#define FAKE_VOID_FUNC(...) EXPAND(FUNC_VOID_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define FUNC_VOID_(N, ...) EXPAND(FUNC_VOID_N(N, __VA_ARGS__))

#define FUNC_VOID_N(N, ...) EXPAND(FAKE_VOID_FUNC##N(__VA_ARGS__))

#define FAKE_VALUE_FUNC_VARARG(...)                                                                \
	EXPAND(FUNC_VALUE_VARARG_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define FUNC_VALUE_VARARG_(N, ...) EXPAND(FUNC_VALUE_VARARG_N(N, __VA_ARGS__))

#define FUNC_VALUE_VARARG_N(N, ...) EXPAND(FAKE_VALUE_FUNC##N##_VARARG(__VA_ARGS__))

#define FAKE_VOID_FUNC_VARARG(...)                                                                 \
	EXPAND(FUNC_VOID_VARARG_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define FUNC_VOID_VARARG_(N, ...) EXPAND(FUNC_VOID_VARARG_N(N, __VA_ARGS__))

#define FUNC_VOID_VARARG_N(N, ...) EXPAND(FAKE_VOID_FUNC##N##_VARARG(__VA_ARGS__))

/* DECLARE FAKE FUNCTIONS - PLACE IN HEADER FILES */

#define DECLARE_FAKE_VALUE_FUNC(...)                                                               \
	EXPAND(DECLARE_FUNC_VALUE_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define DECLARE_FUNC_VALUE_(N, ...) EXPAND(DECLARE_FUNC_VALUE_N(N, __VA_ARGS__))

#define DECLARE_FUNC_VALUE_N(N, ...) EXPAND(DECLARE_FAKE_VALUE_FUNC##N(__VA_ARGS__))

#define DECLARE_FAKE_VOID_FUNC(...)                                                                \
	EXPAND(DECLARE_FUNC_VOID_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define DECLARE_FUNC_VOID_(N, ...) EXPAND(DECLARE_FUNC_VOID_N(N, __VA_ARGS__))

#define DECLARE_FUNC_VOID_N(N, ...) EXPAND(DECLARE_FAKE_VOID_FUNC##N(__VA_ARGS__))

#define DECLARE_FAKE_VALUE_FUNC_VARARG(...)                                                        \
	EXPAND(DECLARE_FUNC_VALUE_VARARG_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define DECLARE_FUNC_VALUE_VARARG_(N, ...) EXPAND(DECLARE_FUNC_VALUE_VARARG_N(N, __VA_ARGS__))

#define DECLARE_FUNC_VALUE_VARARG_N(N, ...) EXPAND(DECLARE_FAKE_VALUE_FUNC##N##_VARARG(__VA_ARGS__))

#define DECLARE_FAKE_VOID_FUNC_VARARG(...)                                                         \
	EXPAND(DECLARE_FUNC_VOID_VARARG_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define DECLARE_FUNC_VOID_VARARG_(N, ...) EXPAND(DECLARE_FUNC_VOID_VARARG_N(N, __VA_ARGS__))

#define DECLARE_FUNC_VOID_VARARG_N(N, ...) EXPAND(DECLARE_FAKE_VOID_FUNC##N##_VARARG(__VA_ARGS__))

/* DEFINE FAKE FUNCTIONS - PLACE IN SOURCE FILES */

#define DEFINE_FAKE_VALUE_FUNC(...)                                                                \
	EXPAND(DEFINE_FUNC_VALUE_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define DEFINE_FUNC_VALUE_(N, ...) EXPAND(DEFINE_FUNC_VALUE_N(N, __VA_ARGS__))

#define DEFINE_FUNC_VALUE_N(N, ...) EXPAND(DEFINE_FAKE_VALUE_FUNC##N(__VA_ARGS__))

#define DEFINE_FAKE_VOID_FUNC(...)                                                                 \
	EXPAND(DEFINE_FUNC_VOID_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define DEFINE_FUNC_VOID_(N, ...) EXPAND(DEFINE_FUNC_VOID_N(N, __VA_ARGS__))

#define DEFINE_FUNC_VOID_N(N, ...) EXPAND(DEFINE_FAKE_VOID_FUNC##N(__VA_ARGS__))

#define DEFINE_FAKE_VALUE_FUNC_VARARG(...)                                                         \
	EXPAND(DEFINE_FUNC_VALUE_VARARG_(PP_NARG_MINUS2(__VA_ARGS__), __VA_ARGS__))

#define DEFINE_FUNC_VALUE_VARARG_(N, ...) EXPAND(DEFINE_FUNC_VALUE_VARARG_N(N, __VA_ARGS__))

#define DEFINE_FUNC_VALUE_VARARG_N(N, ...) EXPAND(DEFINE_FAKE_VALUE_FUNC##N##_VARARG(__VA_ARGS__))

#define DEFINE_FAKE_VOID_FUNC_VARARG(...)                                                          \
	EXPAND(DEFINE_FUNC_VOID_VARARG_(PP_NARG_MINUS1(__VA_ARGS__), __VA_ARGS__))

#define DEFINE_FUNC_VOID_VARARG_(N, ...) EXPAND(DEFINE_FUNC_VOID_VARARG_N(N, __VA_ARGS__))

#define DEFINE_FUNC_VOID_VARARG_N(N, ...) EXPAND(DEFINE_FAKE_VOID_FUNC##N##_VARARG(__VA_ARGS__))

#endif /* FAKE_FUNCTIONS */
