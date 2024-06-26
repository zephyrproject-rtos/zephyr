An extension may be loaded using any implementation of a :c:struct:`llext_loader`
which has a set of function pointers that provide the necessary functionality
to read the ELF data. A loader also provides some minimal context (memory)
needed by the :c:func:`llext_load` function. An implementation over a buffer
containing an ELF in addressable memory in memory is available as
:c:struct:`llext_buf_loader`.
