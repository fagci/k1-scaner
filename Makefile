# =============================================================================
# Directory Structure
# =============================================================================
SRC_DIR       := src
OBJ_DIR       := obj
BIN_DIR       := bin

# =============================================================================
# Project Configuration
# =============================================================================
PROJECT_NAME  := k1-scaner
TARGET        := $(BIN_DIR)/$(PROJECT_NAME)
GIT_HASH      := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_TIME    := $(shell date -u +'%Y-%m-%d_%H:%M_UTC')
BUILD_TAG     := $(shell date -u +'%Y%m%d_%H%M')

# =============================================================================
# Source Files
# =============================================================================
SRC := $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/driver/*.c) \
       $(wildcard $(SRC_DIR)/helper/*.c) \
       $(wildcard $(SRC_DIR)/ui/*.c) \
       $(wildcard $(SRC_DIR)/app/*.c) \
       $(OBJ_DIR)/app/cmd.o

OBJS := $(OBJ_DIR)/start.o \
        $(OBJ_DIR)/init.o \
        $(OBJ_DIR)/external/printf/printf.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_adc.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_comp.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_crc.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_dac.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_dma.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_exti.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_gpio.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_i2c.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_lptim.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_pwr.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_rcc.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_rtc.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_spi.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_tim.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_usart.o \
        $(OBJ_DIR)/external/PY32F071_HAL_Driver/Src/py32f071_ll_utils.o \
        $(OBJ_DIR)/external/littlefs/lfs.o \
        $(OBJ_DIR)/external/littlefs/lfs_util.o \
        $(OBJ_DIR)/external/CherryUSB/core/usbd_core.o \
        $(OBJ_DIR)/external/CherryUSB/class/cdc/usbd_cdc.o \
        $(OBJ_DIR)/external/CherryUSB/port/usb_dc_py32.o \
        $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
        $(OBJ_DIR)/driver/usbd_cdc_if.o

# =============================================================================
# Toolchain
# =============================================================================
TOOLCHAIN_PREFIX := arm-none-eabi-
AS       := $(TOOLCHAIN_PREFIX)gcc
CC       := $(TOOLCHAIN_PREFIX)gcc
LD       := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY  := $(TOOLCHAIN_PREFIX)objcopy
SIZE     := $(TOOLCHAIN_PREFIX)size

# =============================================================================
# Compiler Flags
# =============================================================================
COMMON_FLAGS := -mcpu=cortex-m0plus -mthumb -mabi=aapcs
OPTIMIZATION := -Os -flto=auto -ffunction-sections -fdata-sections

# Assembler flags
ASFLAGS  := $(COMMON_FLAGS) -c

CFLAGS   := $(COMMON_FLAGS) $(OPTIMIZATION) \
            -std=c2x \
            -Wall -Wextra \
            -Wno-missing-field-initializers \
            -Wno-incompatible-pointer-types \
            -Wno-strict-aliasing \
            -Wno-unused-function -Wno-unused-variable \
            -fno-builtin -fshort-enums \
            -Wno-unused-parameter \
            -fno-delete-null-pointer-checks \
            -fsingle-precision-constant \
            -finline-functions-called-once \
            -MMD -MP

RELEASE_FLAGS := -g0 -DNDEBUG
CFLAGS += $(RELEASE_FLAGS)

DEFINES  := -DPRINTF_INCLUDE_CONFIG_H \
            -DGIT_HASH=\"$(GIT_HASH)\" \
            -DTIME_STAMP=\"$(BUILD_TIME)\" \
            -DPY32F071xB \
            -DUSE_FULL_LL_DRIVER \
            -DLFS_NO_MALLOC \
            -DLFS_NO_ASSERT \
            -DLFS_NO_DEBUG \
            -DLFS_NO_WARN \
            -DLFS_NO_ERROR \
            -DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1

INC_DIRS := -I./src/config \
            -I./src/external/CMSIS/Device/PY32F071/Include \
            -I./src/external/CMSIS/Include \
            -I./src/external/littlefs \
            -I./src/external/PY32F071_HAL_Driver/Inc \
            -I./src/driver \
            -I./src/external/CherryUSB/common \
            -I./src/external/CherryUSB/core \
            -I./src/external/CherryUSB/class \
            -I./src/external/CherryUSB/port

LDFLAGS  := $(COMMON_FLAGS) \
            -nostartfiles \
            -Tfirmware.ld \
            -nostdlib \
            -ffreestanding \
            -Wl,--gc-sections \
            -Wl,--build-id=none \
            -Wl,--print-memory-usage \
            -Wl,-Map=$(OBJ_DIR)/output.map

.PHONY: all clean help info

all: $(TARGET).bin
	@echo "Build completed: $(TARGET).bin"

$(TARGET).bin: $(TARGET)
	@echo "Creating binary file..."
	$(OBJCOPY) -O binary $< $@

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Linking..."
	$(LD) $(LDFLAGS) $^ -o $@
	@echo ""
	$(SIZE) $@
	@echo ""

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(@D)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(INC_DIRS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	@mkdir -p $(@D)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

info:
	@echo "Project:     $(PROJECT_NAME)"
	@echo "Git Hash:    $(GIT_HASH)"
	@echo "Build Time:  $(BUILD_TIME)"
	@echo "CC:          $(CC)"

clean:
	@echo "Cleaning..."
	@rm -rf $(TARGET) $(TARGET).* $(OBJ_DIR) $(BIN_DIR)/*.bin

help:
	@echo "Available targets: all, clean, info"

DEPS := $(OBJS:.o=.d)
-include $(DEPS)
