# Módulo 15 - comparador analógico: só ATmega328P
ifeq ($(MCU),uc3a)
all clean size disasm flash qemu:
	@echo "Modulo 15: o AT32UC3A nao tem comparador analogico (use MCU=m328p)."
else
include ../common/mk/rules.mk
endif
