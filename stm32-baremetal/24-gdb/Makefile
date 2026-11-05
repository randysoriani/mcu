# Módulo 24 - GDB
# -Og: otimiza sem destruir a correspondência linha de código <-> instruções
OPT := -Og
include ../common/mk/rules.mk

# carrega os comandos do curso em "make gdb" e "make debug"
GDBFLAGS := -x gdb/curso.gdb

.PHONY: gdb-batch
# Exemplo de GDB não interativo: imprime algumas variáveis e sai
gdb-batch: $(TARGET).elf
	$(GDB) -batch -ex "target extended-remote :3333" -ex "monitor halt" \
	       -x gdb/curso.gdb -ex "nucleo" -ex "p cfg" -ex "p estado" -ex "info registers pc sp" \
	       -ex "monitor resume" $<
