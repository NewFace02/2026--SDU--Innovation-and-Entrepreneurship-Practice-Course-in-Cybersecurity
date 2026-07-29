CC ?= gcc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude -Isrc/common

COMMON = src/common/sm3.c
X86 = src/x86/sm3_x86_avx.c
ARM64 = src/arm64/sm3_arm64_neon.c
TEST = tests/main.c
OBJ = sm3.o sm3_x86_avx.o sm3_arm64_neon.o main.o

sm3_demo: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

sm3.o: $(COMMON) include/sm3.h src/common/sm3_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(COMMON) -o $@

sm3_x86_avx.o: $(X86) include/sm3.h src/common/sm3_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(X86) -o $@

sm3_arm64_neon.o: $(ARM64) include/sm3.h src/common/sm3_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(ARM64) -o $@

main.o: $(TEST) include/sm3.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(TEST) -o $@

clean:
ifeq ($(OS),Windows_NT)
	-cmd /C del /Q $(OBJ) sm3_demo.exe 2>NUL
else
	$(RM) $(OBJ) sm3_demo
endif

.PHONY: clean
