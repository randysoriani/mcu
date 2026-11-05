# =============================================================================
#  rules.mk - Regras de build compartilhadas (curso TM4C bare-metal)
# =============================================================================
#  Uso (dentro da pasta de um módulo):
#       make                -> compila para o MCU padrão (123)
#       make MCU=123        -> TM4C123GH6PM  (EK-TM4C123GXL, 80 MHz)
#       make MCU=129        -> TM4C1294NCPDT (EK-TM4C1294XL, 120 MHz)
#       make MCU=123 flash  -> grava via OpenOCD (ICDI)
#       make MCU=123 lmflash-> grava via lm4flash (lm4tools)
#       make MCU=129 size | disasm | debug | openocd | gdb
#       make clean
#
#  Variáveis que um módulo pode definir ANTES do include:
#       SRCS      fontes do módulo   (padrão: $(MCU)/*.c $(MCU)/*.s src/*.c)
#       STARTUP   startup            (padrão: common/$(MCU)/startup.c)
#       LDSCRIPT  linker script      (padrão: common/$(MCU)/linker.ld)
#       LIB       partes da biblioteca do curso (padrão: clock systick uart printf)
#       OPT       otimização         (padrão: -O2)
#       EXTRA     flags extras, ex.: make EXTRA=-DDEMO=2  (faça make clean antes)
# =============================================================================

MCU     ?= 123
CROSS   ?= arm-none-eabi-

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

# Os dois chips são Cortex-M4F: mesma CPU, periféricos e mapa diferentes
CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
ifeq ($(MCU),123)
  DEFS      := -DTM4C123 -DPART_TM4C123GH6PM
  OCD_BOARD := board/ek-tm4c123gxl.cfg
else ifeq ($(MCU),129)
  DEFS      := -DTM4C129 -DPART_TM4C1294NCPDT
  OCD_BOARD := board/ek-tm4c1294xl.cfg
else
  $(error MCU inválido '$(MCU)'. Use MCU=123 ou MCU=129)
endif

STARTUP  ?= $(COMMON)/$(MCU)/startup.c
LDSCRIPT ?= $(COMMON)/$(MCU)/linker.ld
LIB      ?= clock systick uart printf
OPT      ?= -O2

SRCS ?= $(wildcard $(MCU)/*.c) $(wildcard $(MCU)/*.s) $(wildcard src/*.c)
SRCS += $(STARTUP)
SRCS += $(foreach m,$(LIB),$(wildcard $(COMMON)/$(MCU)/$(m).c $(COMMON)/lib/$(m).c))

INCS := -I$(MCU) -Isrc -I$(COMMON)/include -I$(COMMON)/$(MCU)

CFLAGS  := $(CPU) $(DEFS) $(INCS) $(OPT) -g3 -std=c11
CFLAGS  += -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -ffunction-sections -fdata-sections -fno-common
CFLAGS  += -MMD -MP
CFLAGS  += $(EXTRA)

ASFLAGS := $(CPU) $(DEFS) $(INCS) -g3 -MMD -MP

LDFLAGS := $(CPU) -T$(LDSCRIPT) -nostartfiles
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(TARGET).map -Wl,--print-memory-usage
LDFLAGS += -Wl,--no-warn-rwx-segments
LIBS    := -lc -lgcc

NAMES := $(notdir $(basename $(SRCS)))
ifneq ($(words $(NAMES)),$(words $(sort $(NAMES))))
  $(error Existem fontes com o mesmo nome: $(NAMES))
endif
OBJS := $(addprefix $(BUILD)/obj/,$(addsuffix .o,$(NAMES)))
DEPS := $(OBJS:.o=.d)
vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(SRCS)))
vpath %.S $(sort $(dir $(SRCS)))

.PHONY: all bin hex size disasm flash lmflash debug openocd gdb clean

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

OPENOCD  ?= openocd
OCD_ARGS := -f $(OCD_BOARD)

flash: $(TARGET).elf
	$(OPENOCD) $(OCD_ARGS) -c "program $< verify reset exit"

lmflash: $(TARGET).bin
	lm4flash $<

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
