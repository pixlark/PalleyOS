CC=i686-elf-gcc
AS=i686-elf-as
FLAGS=-ffreestanding -O2 -nostdlib -Wall -Wextra -Iinclude

SRC_DIR=src
BUILD_DIR=build
ISO_DIR=isodir

OBJ_DIR=obj
OBJS=$(OBJ_DIR)/boot.o \
	$(OBJ_DIR)/kernel.o

all: $(BUILD_DIR)/palleyos.iso

$(BUILD_DIR)/palleyos.iso: $(BUILD_DIR)/palleyos.bin
	@make --silent make-$(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp src/grub.cfg $(ISO_DIR)/boot/grub/
	cp $(BUILD_DIR)/palleyos.bin $(ISO_DIR)/boot/
	grub-mkrescue -o $@ $(ISO_DIR) 2> /dev/null

$(BUILD_DIR)/palleyos.bin: $(OBJS)
	@make --silent make-$(BUILD_DIR)
	$(CC) -I $(SRC_DIR)/linker.ld -o $@ $(FLAGS) $(OBJS) -lgcc
	@./check_multiboot.sh

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@make --silent make-$(OBJ_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	@make --silent make-$(OBJ_DIR)
	$(AS) $< -o $@

.PHONY: run clean make-$(OBJ_DIR) make-$(BUILD_DIR) make-$(ISO_DIR)

make-$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

make-$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

make-$(ISO_DIR):
	@mkdir -p $(ISO_DIR)

run: $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 -m 256 -cdrom $(BUILD_DIR)/palleyos.iso

clean:
	rm -rf $(OBJ_DIR) $(ISO_DIR) $(BUILD_DIR)
