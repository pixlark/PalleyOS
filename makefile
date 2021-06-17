CC=i686-elf-gcc
AS=i686-elf-as
FLAGS=-ffreestanding -O0 -nostdlib -Wall -Wextra -Iinclude -g -std=c99
QEMU_FLAGS=-m 64M -cdrom $(BUILD_DIR)/palleyos.iso

SRC_DIR=src
BUILD_DIR=build
ISO_DIR=isodir

STD_LIB_DIR=stdlib
TIMER_DIR=timer

OBJ_DIR=obj
OBJS=$(OBJ_DIR)/boot.o \
	$(OBJ_DIR)/kernel.o \
	$(OBJ_DIR)/idt.o \
	$(OBJ_DIR)/isr.o \
	$(OBJ_DIR)/gdt.o \
	$(OBJ_DIR)/pci.o \
	$(OBJ_DIR)/pic.o \
	$(OBJ_DIR)/fadt.o \
	$(OBJ_DIR)/ata.o \
	$(OBJ_DIR)/ata_helper.o \
	$(OBJ_DIR)/k_term_proc.o \
	$(OBJ_DIR)/cpuid.o \
	$(OBJ_DIR)/cpuid_fetch.o \
	$(OBJ_DIR)/keyboard_io.o \
	$(OBJ_DIR)/memory.o \
	$(OBJ_DIR)/$(TIMER_DIR)/timer.o \
	$(OBJ_DIR)/$(TIMER_DIR)/sleep.o \
	$(OBJ_DIR)/$(TIMER_DIR)/PIT.o \
	$(OBJ_DIR)/$(STD_LIB_DIR)/tio.o \
	$(OBJ_DIR)/$(STD_LIB_DIR)/kstdio.o \
	$(OBJ_DIR)/$(STD_LIB_DIR)/kheap.o \
	$(OBJ_DIR)/$(STD_LIB_DIR)/kstdlib.o 

all: $(BUILD_DIR)/palleyos.iso

$(BUILD_DIR)/palleyos.iso: $(BUILD_DIR)/palleyos.bin
	@make --silent make-$(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp src/grub.cfg $(ISO_DIR)/boot/grub/
	cp $(BUILD_DIR)/palleyos.bin $(ISO_DIR)/boot/
	grub-mkrescue -o $@ $(ISO_DIR) 2> /dev/null

$(BUILD_DIR)/palleyos.bin: $(OBJS) $(SRC_DIR)/linker.ld
	@make --silent make-$(BUILD_DIR)
	$(CC) -T $(SRC_DIR)/linker.ld -o $@ $(FLAGS) $(OBJS) -lgcc
	@./check_multiboot.sh

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@make --silent make-$(OBJ_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	@make --silent make-$(OBJ_DIR)
	$(AS) $< -o $@

$(OBJ_DIR)/$(TIMER_DIR)/%.o: $(SRC_DIR)/$(TIMER_DIR)/%.c
	@make --silent make-$(TIMER_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/$(TIMER_DIR)/%.o: $(SRC_DIR)/$(TIMER_DIR)/%.s
	@make --silent make-$(TIMER_DIR)
	$(AS) $< -o $@

$(OBJ_DIR)/$(STD_LIB_DIR)/%.o: $(SRC_DIR)/$(STD_LIB_DIR)/%.c
	@make --silent make-$(STD_LIB_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

.PHONY: run run-bin run-server clean make-$(OBJ_DIR) make-$(BUILD_DIR) make-$(ISO_DIR) make-$(STD_LIB_DIR) make-$(TIMER_DIR)

make-$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

make-$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

make-$(ISO_DIR):
	@mkdir -p $(ISO_DIR)

make-$(STD_LIB_DIR):
	@mkdir -p obj/$(STD_LIB_DIR)

make-$(TIMER_DIR):
	@mkdir -p obj/$(TIMER_DIR)

run: $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-boot d \
		-m 256M \
		-cdrom $(BUILD_DIR)/palleyos.iso \
		-drive format=raw,file=./palleyos.img

run-bin: $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-boot d \
		-m 256M \
		-kernel $(BUILD_DIR)/palleyos.bin \
		-drive format=raw,file=./palleyos.img

run-server: $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-m 256M \
		-cdrom $(BUILD_DIR)/palleyos.iso
		-drive format=raw,file=./palleyos.img \
		-s -S

clean:
	rm -rf $(OBJ_DIR) $(ISO_DIR) $(BUILD_DIR)
