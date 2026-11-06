# Módulo 20 - autoprogramação
# No ATmega328P usa um linker script próprio, com a seção .bootloader em 0x7000
ifeq ($(MCU),uc3a)
include ../common/mk/rules.mk
else
LDSCRIPT := m328p/linker_boot.ld
include ../common/mk/rules.mk
endif
