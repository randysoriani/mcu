# =============================================================================
#  rules.mk - Regras de build do curso AVR bare-metal
# =============================================================================
#  Uso (dentro da pasta de um módulo):
#     make                     -> compila para o MCU padrão (m328p)
#     make MCU=m328p           -> ATmega328P (Arduino Uno/Nano), 8 bits, 16 MHz
#     make MCU=uc3a            -> AT32UC3A0512 (placa EVK1100), 32 bits, 48 MHz
#     make MCU=m328p flash     -> grava pelo bootloader do Arduino (avrdude)
#     make MCU=m328p flash PROG=usbasp   -> grava por um programador ISP USBasp
#     make MCU=uc3a  flash     -> grava pelo bootloader USB DFU (dfu-programmer)
#     make MCU=m328p qemu      -> roda no QEMU (placa Arduino Uno emulada)
#     make MCU=m328p qemu-gdb  -> QEMU esperando o avr-gdb na porta 1234
#     make size | disasm | clean
#
#  Variáveis que o módulo pode definir ANTES do include:
#     SRCS, STARTUP, LDSCRIPT, LIB (padrão: clock tick uart printf), OPT, EXTRA
# =============================================================================

MCU     ?= m328p
COMMON  := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROJECT ?= $(notdir $(CURDIR))
BUILD   := build/$(MCU)
TARGET  := $(BUILD)/$(PROJECT)

ifeq ($(MCU),m328p)
  CROSS    ?= avr-
  CPU      := -mmcu=atmega328p
  DEFS     := -DAVR8 -DATMEGA328P -DF_CPU=16000000UL
  OPT      ?= -Os
  ASEXT    := S
  # avrdude: PROG=arduino (bootloader, padrão) ou usbasp, avrisp2, ...
  PROG     ?= arduino
  PORT     ?= /dev/ttyACM0
  AVRDUDE_BAUD ?= 115200
else ifeq ($(MCU),uc3a)
  CROSS    ?= avr32-
  CPU      := -mpart=uc3a0512
  DEFS     := -DAVR32 -DUC3A0512 -DF_CPU=48000000UL
  OPT      ?= -O2
  ASEXT    := S
else
  $(error MCU inválido '$(MCU)'. Use MCU=m328p ou MCU=uc3a)
endif

CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size

STARTUP  ?= $(COMMON)/$(MCU)/startup.S
LDSCRIPT ?= $(COMMON)/$(MCU)/linker.ld
LIB      ?= clock tick uart printf

SRCS ?= $(wildcard $(MCU)/*.c) $(wildcard $(MCU)/*.S) $(wildcard src/*.c)
SRCS += $(STARTUP)
SRCS += $(foreach m,$(LIB),$(wildcard $(COMMON)/$(MCU)/$(m).c $(COMMON)/lib/$(m).c))
ifeq ($(MCU),uc3a)
  SRCS += $(COMMON)/uc3a/intc.c      # o startup.S do AVR32 sempre usa o despachante de interrupções
endif

INCS   := -I$(MCU) -Isrc -I$(COMMON)/include -I$(COMMON)/$(MCU)
CFLAGS := $(CPU) $(DEFS) $(INCS) $(OPT) -g -std=gnu99
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections -MMD -MP
CFLAGS += $(EXTRA)
ASFLAGS := $(CPU) $(DEFS) $(INCS) -g -MMD -MP $(EXTRA)
LDFLAGS := $(CPU) -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(TARGET).map

NAMES := $(notdir $(basename $(SRCS)))
OBJS  := $(addprefix $(BUILD)/obj/,$(addsuffix .o,$(NAMES)))
DEPS  := $(OBJS:.o=.d)
vpath %.c $(sort $(dir $(SRCS)))
vpath %.S $(sort $(dir $(SRCS)))

.PHONY: all size disasm flash flash-eeprom fuses-read qemu qemu-gdb gdb clean
all: $(TARGET).elf $(TARGET).hex
	@$(SIZE) $(TARGET).elf

$(BUILD)/obj/%.o: %.c | $(BUILD)/obj
	@echo "  CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/obj/%.o: %.S | $(BUILD)/obj
	@echo "  AS   $<"
	@$(CC) $(ASFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo "  LD   $@"
	@$(CC) $(LDFLAGS) $(OBJS) -lgcc -o $@

# .hex só com a Flash (a EEPROM, se houver, vai num .eep separado)
$(TARGET).hex: $(TARGET).elf
	@$(OBJCOPY) -O ihex -R .eeprom $< $@
	@$(OBJCOPY) -O ihex -j .eeprom --change-section-lma .eeprom=0 $< $(TARGET).eep 2>/dev/null || true

$(BUILD)/obj:
	@mkdir -p $@

size: $(TARGET).elf
	@$(SIZE) -A $<

disasm: $(TARGET).elf
	@$(OBJDUMP) -d -S -h $< > $(TARGET).lst
	@echo "Listagem gerada em $(TARGET).lst"

ifeq ($(MCU),m328p)
flash: $(TARGET).hex
	avrdude -p m328p -c $(PROG) $(if $(filter arduino,$(PROG)),-P $(PORT) -b $(AVRDUDE_BAUD)) \
	        -U flash:w:$<:i
# EEPROM (módulo 16): o bootloader do Arduino (optiboot) não grava EEPROM -> use PROG=usbasp
flash-eeprom: $(TARGET).hex
	avrdude -p m328p -c $(PROG) $(if $(filter arduino,$(PROG)),-P $(PORT) -b $(AVRDUDE_BAUD)) \
	        -U eeprom:w:$(TARGET).eep:i
# Fuses (módulo 04): só com programador ISP
fuses-read:
	avrdude -p m328p -c $(PROG) -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
qemu: $(TARGET).elf
	qemu-system-avr -M uno -nographic -serial mon:stdio -bios $<
# QEMU parado, esperando o GDB na porta 1234 (módulo 21)
qemu-gdb: $(TARGET).elf
	qemu-system-avr -M uno -nographic -serial mon:stdio -bios $< -s -S
# Em outro terminal: make MCU=m328p gdb   (GDB=gdb se o gdb do sistema tiver suporte a AVR)
GDB ?= avr-gdb
gdb: $(TARGET).elf
	$(GDB) -q -ex "target remote :1234" $<
else
flash: $(TARGET).hex
	dfu-programmer at32uc3a0512 erase
	dfu-programmer at32uc3a0512 flash --suppress-bootloader-mem $<
	dfu-programmer at32uc3a0512 start
qemu:
	@echo "O QEMU não emula o AVR32 UC3."
endif

clean:
	rm -rf build

-include $(DEPS)
