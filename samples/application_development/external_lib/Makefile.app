
libmylib.a:
	@echo $(CFLAGS)
	@rm -f $(O)/libmylib.a
	$(Q)@+$(MAKE)  -C $(SOURCE_DIR)/mylib CFLAGS="$(KBUILD_CFLAGS) $(ZEPHYRINCLUDE)"
	@cp $(SOURCE_DIR)/mylib/lib/libmylib.a $(O)/libmylib.a

