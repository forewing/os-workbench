NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c" -o -name "*.S")
INC_DIR        := include/ framework/
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86-qemu
endif

PREBUILD := git initrd
include $(AM_HOME)/Makefile.app
include ../Makefile.lab

.PHONY: initrd

initrd:
	@seq 1 65535 > build/ramdisk.img
	# dd if=/dev/zero of=build/ramdisk.img bs=4KB count=1

QEMU_FLAGS := -serial stdio -machine accel=kvm:tcg -drive format=raw,file=$(BINARY)

run2: image
	qemu-system-i386 -smp 2 $(QEMU_FLAGS)

run4: image
	qemu-system-i386 -smp 4 $(QEMU_FLAGS)

remote2: image
	qemu-system-i386 -smp 2 $(QEMU_FLAGS) -vnc :0 # 5900+0

remote4: image
	qemu-system-i386 -smp 4 $(QEMU_FLAGS) -vnc :0

gdb2: image
	qemu-system-i386 -smp 2 -gdb tcp::9000 -S $(QEMU_FLAGS)

gdb4: image
	qemu-system-i386 -smp 4 -gdb tcp::9000 -S $(QEMU_FLAGS)