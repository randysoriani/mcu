# =============================================================================
#  rules.mk - Regras de build compartilhadas por todos os módulos do curso
# =============================================================================
#
#  Uso (dentro da pasta de um módulo):
#       make                -> compila para o MCU padrão (f4)
#       make MCU=f0         -> compila para STM32F072RB (Cortex-M0)
#       make MCU=f4         -> compila para STM32F446RE (Cortex-M4F)
#       make MCU=f0 flash   -> grava via OpenOCD (ST-LINK)
#       make MCU=f0 stflash -> grava via st-flash (stlink-tools)
#       make MCU=f4 size    -> mostra o uso de FLASH/RAM
#       make MCU=f4 disasm  -> gera listagem assembly (.lst)
#       make MCU=f4 debug   -> abre OpenOCD + GDB
#       make clean
#
#  Variáveis que um módulo pode sobrescrever ANTES do include:
#       SRCS      lista de fontes do módulo   (padrão: $(MCU)/*.c $(MCU)/*.s)
#       STARTUP   arquivo de startup          (padrão: common/$(MCU)/startup.c)
#       LDSCRIPT  linker script               (padrão: common/$(MCU)/linker.ld)
#       LIB       módulos da "biblioteca" do curso a linkar
#                 (padrão: clock systick uart printf ; "LIB :=" = nenhum)
#       OPT       nível de otimização          (padrão: -O2)
# =============================================================================

MCU     ?= f4
CROSS   ?= arm-none-eabi-

# Diretório "common" (absoluto) calculado a partir da localização deste arquivo
COMMON  := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROJECT ?= $(notdir $(CURDIR))
BUILD   := build/$(MCU)
TARGET  := $(BUILD)/$(PROJECT)

CC      := $(CROSS)gcc
AS      := $(CROSS)gcc -x assembler-with-cpp
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size
GDB     ?= $(CROSS)gdb
GDBFLAGS ?=              # ex.: -x gdb/curso.gdb (módulo 24)

# ----------------------------------------------------------------------------
#  Seleção do microcontrolador
# ----------------------------------------------------------------------------
ifeq ($(MCU),f0)
  CPU        := -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
  DEFS       := -DSTM32F0 -DSTM32F072xB
  OCD_TARGET := target/stm32f0x.cfg
else ifeq ($(MCU),f4)
  CPU        := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
  DEFS       := -DSTM32F4 -DSTM32F446xx
  OCD_TARGET := target/stm32f4x.cfg
else
  $(error MCU inválido '$(MCU)'. Use MCU=f0 ou MCU=f4)
endif

STARTUP  ?= $(COMMON)/$(MCU)/startup.c
LDSCRIPT ?= $(COMMON)/$(MCU)/linker.ld
LIB      ?= clock systick uart printf
OPT      ?= -O2

SRCS ?= $(wildcard $(MCU)/*.c) $(wildcard $(MCU)/*.s) $(wildcard src/*.c)
SRCS += $(STARTUP)
# Cada nome em LIB vira common/<mcu>/<nome>.c ou common/lib/<nome>.c
SRCS += $(foreach m,$(LIB),$(wildcard $(COMMON)/$(MCU)/$(m).c $(COMMON)/lib/$(m).c))

INCS := -I$(MCU) -Isrc -I$(COMMON)/include -I$(COMMON)/$(MCU)

# ----------------------------------------------------------------------------
#  Flags
# ----------------------------------------------------------------------------
CFLAGS  := $(CPU) $(DEFS) $(INCS) $(OPT) -g3 -std=c11
CFLAGS  += -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -ffunction-sections -fdata-sections -fno-common
CFLAGS  += -MMD -MP
CFLAGS  += $(EXTRA)          # ex.: make EXTRA=-DCLOCK_SOURCE=2 (faça make clean antes)

ASFLAGS := $(CPU) $(DEFS) $(INCS) -g3 -MMD -MP

LDFLAGS := $(CPU) -T$(LDSCRIPT) -nostartfiles
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(TARGET).map -Wl,--print-memory-usage
LDFLAGS += -Wl,--no-warn-rwx-segments
LIBS    := -lc -lgcc

# ----------------------------------------------------------------------------
#  Objetos: cada fonte vira build/<mcu>/obj/<nome>.o
#  (os nomes de arquivo precisam ser únicos -> checado abaixo)
# ----------------------------------------------------------------------------
NAMES := $(notdir $(basename $(SRCS)))
ifneq ($(words $(NAMES)),$(words $(sort $(NAMES))))
  $(error Existem fontes com o mesmo nome: $(NAMES))
endif
OBJS := $(addprefix $(BUILD)/obj/,$(addsuffix .o,$(NAMES)))
DEPS := $(OBJS:.o=.d)
vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(SRCS)))
vpath %.S $(sort $(dir $(SRCS)))

.PHONY: all bin hex size disasm flash stflash erase debug openocd gdb clean

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex
	@$(SIZE) $(TARGET).elf

$(BUILD)/obj/%.o: %.c | $(BUILD)/obj
	@echo "  CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/obj/%.o: %.s | $(BUILD)/obj
	@echo "  AS   $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD)/obj/%.o: %.S | $(BUILD)/obj
	@echo "  AS   $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo "  LD   $@"
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(TARGET).bin: $(TARGET).elf
	@$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	@$(OBJCOPY) -O ihex $< $@

$(BUILD)/obj:
	@mkdir -p $@

bin: $(TARGET).bin
hex: $(TARGET).hex

size: $(TARGET).elf
	@$(SIZE) -A -x $<
	@$(SIZE) $<

disasm: $(TARGET).elf
	@$(OBJDUMP) -d -S -h $< > $(TARGET).lst
	@echo "Listagem gerada em $(TARGET).lst"

# ----------------------------------------------------------------------------
#  Gravação e depuração
# ----------------------------------------------------------------------------
OPENOCD ?= openocd
OCD_ARGS := -f interface/stlink.cfg -f $(OCD_TARGET)

flash: $(TARGET).elf
	$(OPENOCD) $(OCD_ARGS) -c "program $< verify reset exit"

stflash: $(TARGET).bin
	st-flash --reset write $< 0x08000000

erase:
	$(OPENOCD) $(OCD_ARGS) -c "init; reset halt; flash erase_sector 0 0 last; exit"

openocd:
	$(OPENOCD) $(OCD_ARGS)

gdb: $(TARGET).elf
	$(GDB) -ex "target extended-remote :3333" -ex "monitor reset halt" -ex "load" $(GDBFLAGS) $<

debug: $(TARGET).elf
	$(OPENOCD) $(OCD_ARGS) > $(BUILD)/openocd.log 2>&1 & \
	  sleep 1; \
	  $(GDB) -ex "target extended-remote :3333" -ex "monitor reset halt" -ex "load" \
	         $(GDBFLAGS) -ex "break main" -ex "continue" $<; \
	  kill %1

clean:
	rm -rf build

-include $(DEPS)
