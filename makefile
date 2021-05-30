CC=i686-elf-gcc
AS=i686-elf-as
FLAGS=-ffreestanding -O2 -nostdlib -Wall -Wextra

SRC_DIR=src
BUILD_DIR=build

OBJ_DIR=obj
OBJS=$(OBJ_DIR)/boot.o \
	$(OBJ_DIR)/kernel.o

all: palleyos.bin

$(OBJ_DIR): 
	mkdir -p obj/

palleyos.bin: $(OBJS)
	$(CC) -I $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/$@ $(FLAGS) $(OBJS) -lgcc
	./check_multiboot.sh

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(AS) $< -o $@

clean:
	rm -rf *.iso *.bin $(OBJ_DIR) *.o

