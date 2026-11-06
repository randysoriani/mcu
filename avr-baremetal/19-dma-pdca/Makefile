# Módulo 19 - DMA (PDCA): só AT32UC3A (o ATmega328P não tem DMA)
MCU ?= uc3a
ifeq ($(MCU),m328p)
all clean size disasm flash qemu:
	@echo "Modulo 19: o ATmega328P nao tem DMA (use MCU=uc3a)."
else
include ../common/mk/rules.mk
endif
