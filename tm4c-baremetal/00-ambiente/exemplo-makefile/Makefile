# =============================================================================
#  Makefile MÍNIMO, escrito do zero, sem includes - para entender cada passo.
#  Compila um blink para a EK-TM4C123GXL (troque as variáveis p/ o 129).
#
#     make          -> blink.elf, blink.bin, blink.hex
#     make flash    -> grava com OpenOCD
#     make clean
# =============================================================================

# ---- 1. Variáveis -----------------------------------------------------------
# '=' é avaliado quando usado (recursivo); ':=' é avaliado na hora (imediato).
CROSS   := arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

COMMON  := ../../common
CPU     := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
# Para o TM4C129: troque 123 -> 129 abaixo (a CPU é a mesma)
MCUDIR  := $(COMMON)/123
DEFS    := -DTM4C123

CFLAGS  := $(CPU) $(DEFS) -O2 -g -Wall -I$(COMMON)/include -I$(MCUDIR) \
           -ffunction-sections -fdata-sections
LDFLAGS := $(CPU) -T$(MCUDIR)/linker.ld -nostartfiles -Wl,--gc-sections \
           -Wl,-Map=blink.map --specs=nano.specs

OBJS    := main.o startup.o

# ---- 2. Regras --------------------------------------------------------------
# alvo: dependências
# <TAB>receita
# A primeira regra do arquivo é a padrão (executada por "make" sem argumentos).
all: blink.elf blink.bin blink.hex
	$(SIZE) blink.elf

# Linkagem: $@ = nome do alvo, $^ = todas as dependências
blink.elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# Regra de padrão: como fazer QUALQUER .o a partir do .c de mesmo nome.
# $< = primeira dependência
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# O startup mora em outra pasta: regra explícita
startup.o: $(MCUDIR)/startup.c
	$(CC) $(CFLAGS) -c $< -o $@

# Conversões do ELF para formatos "crus" de gravação
blink.bin: blink.elf
	$(OBJCOPY) -O binary $< $@

blink.hex: blink.elf
	$(OBJCOPY) -O ihex $< $@

flash: blink.elf
	openocd -f board/ek-tm4c123gxl.cfg \
	        -c "program $< verify reset exit"

# .PHONY: alvos que não são arquivos (senão um arquivo chamado "clean" os quebraria)
.PHONY: all flash clean
clean:
	rm -f *.o *.elf *.bin *.hex *.map
