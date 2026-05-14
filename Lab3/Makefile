PROG_NAME  = test
RISCV64_GCC = riscv64-unknown-elf-gcc
GCC_FLAGS = -march=rv64g -mabi=lp64 -static -mcmodel=medany \
     -fvisibility=hidden -fno-builtin \
     -nostdlib -nostartfiles -g -D MAX_SOFT_TIMERS=10

LD_FLAGS=-T linker.ld
SRC_DIR = src
BUILD_DIR = build

.PHONY: all clean qemu
all: $(BUILD_DIR) | $(BUILD_DIR)/$(PROG_NAME)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/start.o: 
	$(RISCV64_GCC) $(GCC_FLAGS) -c $(SRC_DIR)/start.S -o $(BUILD_DIR)/start.o 

$(BUILD_DIR)/test.o: $(SRC_DIR)/test.c
	$(RISCV64_GCC) $(GCC_FLAGS) -ffunction-sections -c $(SRC_DIR)/test.c -o $(BUILD_DIR)/test.o

$(BUILD_DIR)/sw_timer.o: $(SRC_DIR)/sw_timer.c
	$(RISCV64_GCC) $(GCC_FLAGS) -ffunction-sections -c $(SRC_DIR)/sw_timer.c -o $(BUILD_DIR)/sw_timer.o

$(BUILD_DIR)/print.o: $(SRC_DIR)/print.c
	$(RISCV64_GCC) $(GCC_FLAGS) -ffunction-sections -c $(SRC_DIR)/print.c -o $(BUILD_DIR)/print.o

$(BUILD_DIR)/$(PROG_NAME): $(BUILD_DIR)/start.o $(BUILD_DIR)/test.o $(BUILD_DIR)/sw_timer.o $(BUILD_DIR)/print.o
	$(RISCV64_GCC) $(GCC_FLAGS) -o $@ $(BUILD_DIR)/start.o $(BUILD_DIR)/print.o $(BUILD_DIR)/test.o $(BUILD_DIR)/sw_timer.o $(LD_FLAGS)

dump: $(BUILD_DIR)/$(PROG_NAME)
	riscv64-unknown-elf-objdump -d $(BUILD_DIR)/$(PROG_NAME) > $(BUILD_DIR)/$(PROG_NAME).dump

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.dump $(BUILD_DIR)/$(PROG_NAME)

run:
	qemu-system-riscv64 -machine sifive_u -nographic -bios none -kernel $(BUILD_DIR)/$(PROG_NAME)

debug:
	qemu-system-riscv64 -machine sifive_u -nographic -bios none -S -s -kernel $(BUILD_DIR)/$(PROG_NAME)
