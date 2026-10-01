ARCH ?= riscv64

OUTPUT_FOLDER := build
ISO_FOLDER    := $(OUTPUT_FOLDER)/iso_root
ISO_IMAGE     := $(OUTPUT_FOLDER)/image.iso

LIMINE_DIR    := limine-binary
ifeq ($(ARCH),amd64)
	TARGET := $(ISO_IMAGE)
	ARCH_FOLDER := arch/amd64
	CC          := x86_64-elf-gcc
	LD          := x86_64-elf-ld
	QEMU        := qemu-system-x86_64
	NASM		:= nasm
	NASMFLAGS	:= -f elf64 -g -F dwarf -Wall

	CFLAGS      := -Wall -Wextra -I./inc -I./inc/arch/ -ffreestanding \
				   -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer \
				   -fno-lto -fno-PIC -m64 -march=x86-64 -mabi=sysv \
					-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel
	
	LDFLAGS     := -T $(ARCH_FOLDER)/link.ld -nostdlib -static \
				   -m elf_x86_64 -z max-page-size=0x1000
	
	# QEMU boots the generated ISO image with serial monitoring
	QEMU_FLAGS  := -cdrom $(ISO_IMAGE) -serial mon:stdio -enable-kvm -cpu host,x2apic -m 512M
else ifeq ($(ARCH),riscv64)
	TARGET := $(OUTPUT_FOLDER)/os.elf
	ARCH_FOLDER := arch/riscv64
	CC          := riscv64-unknown-elf-gcc
	LD          := riscv64-unknown-elf-ld
	QEMU        := qemu-system-riscv64
	CFLAGS      := -Wall -Wextra -I./inc -I./inc/arch/ -march=rv64gc -mabi=lp64d -mcmodel=medany -ffreestanding
	LDFLAGS     := -T $(ARCH_FOLDER)/link.ld -nostdlib
	QEMU_FLAGS  := -machine virt -bios default -kernel $(OUTPUT_FOLDER)/os.elf -serial mon:stdio -nographic
else
	$(error Unsupported architecture: $(ARCH))
endif

ARCH_C_SRCS		:= $(wildcard $(ARCH_FOLDER)/*.c)
ARCH_ASM_SRCS	:= $(wildcard $(ARCH_FOLDER)/*.s) $(wildcard $(ARCH_FOLDER)/*.S)
ARCH_NASM_SRCS	:= $(wildcard $(ARCH_FOLDER)/*.asm)

ARCH_OBJS := $(patsubst $(ARCH_FOLDER)/%.c, $(OUTPUT_FOLDER)/%.o, $(ARCH_C_SRCS)) \
			$(patsubst $(ARCH_FOLDER)/%.s, $(OUTPUT_FOLDER)/%.o, \
			$(patsubst $(ARCH_FOLDER)/%.S, $(OUTPUT_FOLDER)/%.o, $(ARCH_ASM_SRCS))) \
			$(patsubst $(ARCH_FOLDER)/%.asm, $(OUTPUT_FOLDER)/%.asm.o, $(ARCH_NASM_SRCS))

.PHONY: all iso run clean

all: $(TARGET)

run: all
	$(QEMU) $(QEMU_FLAGS)

$(OUTPUT_FOLDER)/os.elf: $(ARCH_OBJS) $(OUTPUT_FOLDER)/init.o $(OUTPUT_FOLDER)/sercon.o $(OUTPUT_FOLDER)/vidcon.o \
$(OUTPUT_FOLDER)/gencon.o $(OUTPUT_FOLDER)/kdebug.o $(OUTPUT_FOLDER)/libstr.o $(OUTPUT_FOLDER)/balloc.o $(OUTPUT_FOLDER)/vmtest.o \
$(OUTPUT_FOLDER)/alltest.o | $(OUTPUT_FOLDER)
	$(LD) $(LDFLAGS) $(ARCH_OBJS) $(OUTPUT_FOLDER)/init.o $(OUTPUT_FOLDER)/sercon.o $(OUTPUT_FOLDER)/vidcon.o \
	$(OUTPUT_FOLDER)/gencon.o $(OUTPUT_FOLDER)/kdebug.o $(OUTPUT_FOLDER)/libstr.o $(OUTPUT_FOLDER)/balloc.o $(OUTPUT_FOLDER)/vmtest.o \
	$(OUTPUT_FOLDER)/alltest.o -o $@

$(OUTPUT_FOLDER)/%.o: $(ARCH_FOLDER)/%.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(ARCH_FOLDER)/%.s | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(ARCH_FOLDER)/%.S | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.asm.o: $(ARCH_FOLDER)/%.asm | $(OUTPUT_FOLDER)
	$(NASM) $(NASMFLAGS) $< -o $@

$(OUTPUT_FOLDER)/init.o: oskrn/kern/startup.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/sercon.o: oskrn/console/serial_console.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/vidcon.o: oskrn/console/video_console.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/gencon.o: oskrn/console/console_general.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/libstr.o: oslib/string.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/kdebug.o: oskrn/kern/debug.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/balloc.o: oskrn/vm/boot_alloc.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/vmtest.o: tests/vm.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/alltest.o: tests/all.c | $(OUTPUT_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER):
	mkdir -p $(OUTPUT_FOLDER)

$(LIMINE_DIR):
	curl -fL -o limine-binary.tar.gz https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz
	gunzip < limine-binary.tar.gz | tar -xf -
	rm limine-binary.tar.gz
	
	# Build "limine" utility.
	$(MAKE) -C limine-binary

$(ISO_IMAGE): $(OUTPUT_FOLDER)/os.elf limine.conf | $(LIMINE_DIR)
	mkdir -p $(ISO_FOLDER)/boot
	mkdir -p $(ISO_FOLDER)/EFI/BOOT
	cp $(OUTPUT_FOLDER)/os.elf $(ISO_FOLDER)/boot/
	cp limine.conf $(ISO_FOLDER)/boot/
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_FOLDER)/boot/
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_FOLDER)/boot/
	cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_FOLDER)/boot/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_FOLDER)/EFI/BOOT/
	cp $(LIMINE_DIR)/BOOTIA32.EFI $(ISO_FOLDER)/EFI/BOOT/
	xorriso -as mkisofs -b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_FOLDER) -o $(ISO_IMAGE)
	$(LIMINE_DIR)/limine bios-install $(ISO_IMAGE)

clean:
	rm -rf $(OUTPUT_FOLDER)
