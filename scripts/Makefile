###
# scripts contains sources for various helper programs used throughout
# the kernel for the build process.
# ---------------------------------------------------------------------------
# docproc:       Used in Documentation/DocBook

HOST_EXTRACFLAGS += -I$(srctree)/tools/include

always		:= $(hostprogs-y) $(hostprogs-m)

# The following hostprogs-y programs are only build on demand
hostprogs-y += unifdef docproc

# These targets are used internally to avoid "is up to date" messages
PHONY += build_unifdef build_docproc
build_unifdef: $(obj)/unifdef
	@:
build_docproc: $(obj)/docproc
	@:

# Let clean descend into subdirs
subdir-	+= basic kconfig gen_idt
