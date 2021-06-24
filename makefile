CC=i686-elf-gcc
AS=i686-elf-as
FLAGS=-ffreestanding -O0 -nostdlib -Wall -Wextra -Iinclude -g -std=c99 -Wno-unused-function
QEMU_FLAGS=-m 64M -cdrom $(BUILD_DIR)/palleyos.iso

SRC_DIR=src
BUILD_DIR=build
ISO_DIR=isodir

STD_LIB_DIR=stdlib
TIMER_DIR=timer
OBJ_DIR=obj
OBJ_DIRS := $(patsubst %, $(OBJ_DIR)/%, timer stdlib)
DIRS = $(OBJ_DIR) \
	   $(ISO_DIR) \
	   $(OBJ_DIRS)

# Recursive wildcard search
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

dirfiles:= $(call rwildcard,src,*.c *.s)
files:= $(foreach f, $(dirfiles), $(subst src,$(OBJ_DIR),$(f))) # Change the src/ to obj/ in all found .c and .s files
OBJSC = $(files:.c=.o)   # Change .c to .o
OBJS = $(patsubst %.s,%.o,$(OBJSC))  # Change .s to .o

TEST_ISO_DIR=test-isodir
TEST_SRC_DIR=testing
TEST_OBJS=$(filter-out $(OBJ_DIR)/kernel.o,$(OBJS)) $(OBJ_DIR)/test-main.o

all: $(DIRS) $(BUILD_DIR)/palleyos.iso

testing: $(DIRS) $(BUILD_DIR)/palleyos-unit-tests.iso

$(DIRS):
	mkdir -p $@
	echo $@

# Building the unit-test image

$(BUILD_DIR)/palleyos-unit-tests.iso: $(BUILD_DIR)/palleyos-unit-tests.bin
	@make --silent make-$(TEST_ISO_DIR)
	mkdir -p $(TEST_ISO_DIR)/boot/grub
	cp testing/grub.cfg $(TEST_ISO_DIR)/boot/grub/
	cp $(BUILD_DIR)/palleyos-unit-tests.bin $(TEST_ISO_DIR)/boot/
	grub-mkrescue -o $@ $(TEST_ISO_DIR) 2> /dev/null

$(BUILD_DIR)/palleyos-unit-tests.bin: $(TEST_OBJS) $(SRC_DIR)/linker.ld
	@make --silent make-$(BUILD_DIR)
	$(CC) -T $(SRC_DIR)/linker.ld -o $@ $(FLAGS) $(TEST_OBJS) -lgcc
	@./check_multiboot.sh palleyos.img || /bin/true
	@./check_multiboot.sh build/palleyos-unit-tests.bin

$(OBJ_DIR)/test-main.o: $(TEST_SRC_DIR)/test-main.c
	$(CC) -c $< -o $@ $(FLAGS)

#

$(BUILD_DIR)/palleyos.iso: $(BUILD_DIR)/palleyos.bin
	@make --silent make-$(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp src/grub.cfg $(ISO_DIR)/boot/grub/
	cp $(BUILD_DIR)/palleyos.bin $(ISO_DIR)/boot/
	grub-mkrescue -o $@ $(ISO_DIR) 2> /dev/null

$(BUILD_DIR)/palleyos.bin: $(OBJS) $(SRC_DIR)/linker.ld
	@make --silent make-$(BUILD_DIR)
	$(CC) -T $(SRC_DIR)/linker.ld -o $@ $(FLAGS) $(OBJS) -lgcc
	@./check_multiboot.sh palleyos.img || /bin/true
	@./check_multiboot.sh build/palleyos.bin

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(AS) $< -o $@

$(OBJ_DIR)/$(TIMER_DIR)/%.o: $(SRC_DIR)/$(TIMER_DIR)/%.c make-$(TIMER_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/$(TIMER_DIR)/%.o: $(SRC_DIR)/$(TIMER_DIR)/%.s make-$(TIMER_DIR)
	$(AS) $< -o $@

$(OBJ_DIR)/$(STD_LIB_DIR)/%.o: $(SRC_DIR)/$(STD_LIB_DIR)/%.c make-$(STD_LIB_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

.PHONY: testing run run-bin run-server clean make-$(OBJ_DIR) make-$(BUILD_DIR) make-$(ISO_DIR) make-$(STD_LIB_DIR) make-$(TIMER_DIR) file1

make-$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

make-$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

make-$(ISO_DIR):
	@mkdir -p $(ISO_DIR)

make-$(TEST_ISO_DIR):
	@mkdir -p $(TEST_ISO_DIR)

make-$(STD_LIB_DIR):
	@mkdir -p obj/$(STD_LIB_DIR)

make-$(TIMER_DIR):
	@mkdir -p obj/$(TIMER_DIR)

run: $(DIRS) $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-boot d \
		-m 256M \
		-cdrom $(BUILD_DIR)/palleyos.iso \
		-drive format=raw,file=./palleyos.img

run-bin: $(DIRS) $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-boot d \
		-m 256M \
		-kernel $(BUILD_DIR)/palleyos.bin \
		-drive format=raw,file=./palleyos.img

run-server: $(DIRS) $(BUILD_DIR)/palleyos.iso
	qemu-system-i386 \
		-m 256M \
		-cdrom $(BUILD_DIR)/palleyos.iso \
		-drive format=raw,file=./palleyos.img \
		-s -S

run-tests: $(DIRS) $(BUILD_DIR)/palleyos-unit-tests.iso
	qemu-system-i386 \
		-m 256M \
		-cdrom $(BUILD_DIR)/palleyos-unit-tests.iso \
		-drive format=raw,file=./palleyos.img

clean:
	rm -rf $(OBJ_DIR) $(ISO_DIR) $(BUILD_DIR) $(TEST_ISO_DIR)

file1:
	echo $(OBJS)
