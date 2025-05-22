# get the toolchain compiler and version from the environment
-setq=used_compiler,getenv("CC_ALIASES")

# Compilers.
-file_tag+={GCC,"^used_compiler$"}
# -file_tag+={GXX,"^/opt/zephyr-sdk-0\\.13\\.2/arm-zephyr-eabi/bin/arm-zephyr-eabi-g\\+\\+$"}

# Manuals.
-setq=GCC_MANUAL,"https://gcc.gnu.org/onlinedocs/gcc-10.3.0/gcc.pdf"
-setq=CPP_MANUAL,"https://gcc.gnu.org/onlinedocs/gcc-10.3.0/cpp.pdf"
-setq=C99_STD,"ISO/IEC 9899:1999"

-doc_begin="
    See Chapter \"6 Extensions to the C Language Family\" of "GCC_MANUAL":
    __auto_type: see \"6.7 Referring to a Type with typeof\";
    __asm__: see \"6.48 Alternate Keywords\", and \"6.47 How to Use Inline Assembly Language in C Code\";
    __attribute__: see \"6.39 Attribute Syntax\";
    __typeof__: see \"6.7 Referring to a Type with typeof\";
    __builtin_types_compatible_p: see \"6.59 Other Built-in Functions Provided by GCC\";
    __volatile__: see \"6.48 Alternate Keywords\" and \"6.47.2.1 Volatile\";
    __alignof: see \"6.48 Alternate Keywords\"  and \"6.44 Determining the Alignment of Functions, Types or
Variables\";
    __alignof__: see \"6.48 Alternate Keywords\"  and \"6.44 Determining the Alignment of Functions, Types or
Variables\";
    __const__: see \"6.48 Alternate Keywords\";
    __inline: see \"6.48 Alternate Keywords\";
    _Generic: see description of option \"-Wc99-c11-compat\" in \"3.8 Options to Request or Suppress Warnings\". The compiler allows to C11 features in C99 mode;
    _Static_assert: see descriptions of options \"-Wc99-c11-compat\" and \"-Wc99-c2x-compat\" in \"3.8 Options to Request or Suppress Warnings\". The compiler allows to use C11 and C2x features in C99 mode.
"
-config=STD.tokenext,behavior+={c99, GCC, "^(__auto_type|__asm__|__attribute__|__typeof__|__builtin_types_compatible_p|__volatile__|__alignof|__alignof__|__const__|__inline|_Generic|_Static_assert)$"}
-config=STD.tokenext,behavior+={c18, GCC, "^(__attribute__|__asm__|__const__|__volatile__|__inline)$"}
-doc_end

-doc="See Chapter \"6.7 Referring to a Type with typeof\". of "GCC_MANUAL"."
-config=STD.diag,diagnostics={safe,"^ext_auto_type$"}
-doc="See Chapter \"6.1 Statements and Declarations in Expressions\" of "GCC_MANUAL"."
-config=STD.stmtexpr,behavior+={c99,GCC,specified}
-doc="See Chapter \"6.24 Arithmetic on void- and Function-Pointers\" of "GCC_MANUAL"."
-config=STD.vptrarth,behavior={c99,GCC,specified}
-doc_begin="
    ext_missing_varargs_arg: non-documented GCC extension.
    ext_paste_comma: see Chapter \"6.21 Macros with a Variable Number of Arguments.\" of "GCC_MANUAL".
    ext_flexible_array_in_array: see Chapter \"6.18 Arrays of Length Zero\" of "GCC_MANUAL".
"
-config=STD.diag,behavior+={c99,GCC,"^(ext_missing_varargs_arg|ext_paste_comma|ext_flexible_array_in_array)$"}
-config=STD.diag,behavior+={c18,GCC,"^(ext_missing_varargs_arg)$"}
-doc_end
-doc_begin="Non-documented GCC extension"
-config=STD.emptinit,behavior={c99,GCC,specified}
-config=STD.emptinit,behavior={c18,GCC,specified}
-doc_end
-doc_begin="See Chapter \"6.19 Structures with No Members\" of "GCC_MANUAL"."
-config=STD.emptrecd,behavior={c99,GCC,specified}
-config=STD.emptrecd,behavior={c18,GCC,specified}
-doc_end
-doc="See Chapter \"6.18 Arrays of Length Zero\" of "GCC_MANUAL"."
-config=STD.arayzero,behavior={c99,GCC,specified}

-config=STD.inclnest,behavior+={c99, GCC, 24}
-config=STD.ppifnest,behavior+={c99, GCC, 32}
-config=STD.macident,behavior+={c99, GCC, 4096}

-doc_begin="Allowed headers in freestanding mode."
-config=STD.freestlb,behavior+={c99,GCC,"^(string|fcntl|time|errno|ctype|stdio|inttypes|stdlib).h$"}
-config=STD.freestlb,behavior+={c18,GCC,"^(string|errno|inttypes).h$"}
-doc_end

-doc_begin="See Annex \"J.5.7 Function pointer casts\" of "C99_STD"."
-config=STD.funojptr,behavior={c99,GCC,specified}
-doc_end

-doc_begin="The maximum size of an object is defined in the MAX_SIZE macro, and for a 32 bit architecture is 8MB.
    The maximum size for an array is defined in the PTRDIFF_MAX and in a 32 bit architecture is 2^30-1."
-config=STD.byteobjt,behavior={c99, GCC, 8388608}
-doc_end

-doc_begin="See Section \"6.62.13 Diagnostic Pragmas\" of "GCC_MANUAL"."
-config=STD.nonstdc,behavior+={c99, GCC, "^GCC diagnostic (push|pop|ignored \"-W.*\")$"}
-config=STD.nonstdc,behavior+={c18, GCC, "^GCC diagnostic (push|pop|ignored \"-W.*\")$"}
-doc_end

-doc_begin="See Section \"4.9 Structures, Unions, Enumerations, and Bit-Fields\" of "GCC_MANUAL". Other integer types, such as long int, and enumerated types are permitted even in strictly conforming mode."
-config=STD.bitfldtp,behavior+={c99, GCC, "unsigned char||unsigned short"}
-config=STD.bitfldtp,behavior+={c18, GCC, "unsigned char||unsigned short"}
-doc_end
