CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c11 -march=native
LDFLAGS ?=

TARGET = build/aes_lab.exe
SRC = src/aes_lab.c
SM4_TARGET = build/sm4_lab
SM4_SRC = src/sm4_lab.c
GIFT_TARGET = build/gift_lab
GIFT_SRC = src/gift_lab.c
TWINE_TARGET = build/twine_lab
TWINE_SRC = src/twine_lab.c
BENCH_FILE = data/bench_16MiB.bin

.PHONY: all run sm4 gift twine test clean report bench-data

all: $(TARGET) $(SM4_TARGET) $(GIFT_TARGET) $(TWINE_TARGET)

$(TARGET): $(SRC)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(SM4_TARGET): $(SM4_SRC)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(GIFT_TARGET): $(GIFT_SRC)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(TWINE_TARGET): $(TWINE_SRC)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

bench-data: $(BENCH_FILE)

$(BENCH_FILE):
	mkdir -p data
	dd if=/dev/zero of=$@ bs=1M count=16 status=none

run: $(TARGET) $(BENCH_FILE)
	./$(TARGET) --bench-file $(BENCH_FILE)

sm4: $(SM4_TARGET) $(BENCH_FILE)
	./$(SM4_TARGET) --bench-file $(BENCH_FILE)

gift: $(GIFT_TARGET) $(BENCH_FILE)
	./$(GIFT_TARGET) --bench-file $(BENCH_FILE)

twine: $(TWINE_TARGET) $(BENCH_FILE)
	./$(TWINE_TARGET) --bench-file $(BENCH_FILE)

test: run sm4 gift twine

report:
	cd LaTex && (xelatex -interaction=nonstopmode assignment3.tex || /mnt/c/texlive/2024/bin/windows/xelatex.exe -interaction=nonstopmode assignment3.tex)

clean:
	rm -f $(TARGET) $(SM4_TARGET) $(GIFT_TARGET) $(TWINE_TARGET) LaTex/assignment3.aux LaTex/assignment3.log LaTex/assignment3.out LaTex/assignment3.toc
