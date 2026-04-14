# V6 eBPF/CO-RE IntP -- Build System
#
# Build stages:
#   1. Generate vmlinux.h from running kernel BTF
#   2. Compile eBPF programs (clang -target bpf)
#   3. Generate BPF skeleton header (bpftool gen skeleton)
#   4. Compile and link userspace loader (gcc + libbpf)

CLANG     = clang
CC        = gcc
BPFTOOL   = bpftool

# eBPF compilation flags
BPF_CFLAGS = -O2 -target bpf -g -D__TARGET_ARCH_x86 \
             -I src/ -Wall -Werror

# Userspace compilation flags
CFLAGS     = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE -I src/
LDFLAGS    = -lbpf -lelf -lz -lpthread

TARGET     = intp-ebpf

# Source files
BPF_SRC    = src/intp_kern.bpf.c
BPF_OBJ    = src/intp_kern.bpf.o
BPF_SKEL   = src/intp_kern.skel.h
USER_SRCS  = src/main.c resctrl/mbw.c resctrl/llcocc.c
USER_OBJS  = $(USER_SRCS:.c=.o)

.PHONY: all clean vmlinux

all: $(TARGET)

# Step 1: Generate vmlinux.h from running kernel BTF
src/vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

# Step 2: Compile eBPF programs
$(BPF_OBJ): $(BPF_SRC) src/intp_kern.bpf.h src/vmlinux.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

# Step 3: Generate BPF skeleton
$(BPF_SKEL): $(BPF_OBJ)
	$(BPFTOOL) gen skeleton $< > $@

# Step 4: Compile userspace
src/main.o: src/main.c $(BPF_SKEL) src/intp_kern.bpf.h
	$(CC) $(CFLAGS) -c $< -o $@

resctrl/%.o: resctrl/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Step 5: Link
$(TARGET): $(USER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Convenience target to regenerate vmlinux.h
vmlinux:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > src/vmlinux.h

clean:
	rm -f $(TARGET) $(BPF_OBJ) $(BPF_SKEL) $(USER_OBJS) src/vmlinux.h
