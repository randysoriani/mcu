# Módulo 23 - OpenOCD
include ../common/mk/rules.mk

# Usa a configuração deste módulo em vez de interface/... + target/...
OCD_ARGS := -s openocd -c "set MCU $(MCU)" -f openocd/curso.cfg

.PHONY: telnet semihost symbols ocd-info

# Abre o console do OpenOCD (o servidor precisa estar rodando: make openocd)
telnet:
	@if command -v telnet >/dev/null; then telnet localhost 4444; else nc localhost 4444; fi

# Sobe o OpenOCD já com semihosting ligado e o programa rodando
semihost: $(TARGET).elf
	$(OPENOCD) $(OCD_ARGS) -c "init; arm semihosting enable; reset run"

# Endereços das variáveis que o programa expõe ao debugger
symbols: $(TARGET).elf
	@$(CROSS)nm -n $< | grep -E " (periodo_ms|contador|comando|usar_semihosting)$$"

# Executa comandos e sai (modo "batch")
ocd-info:
	$(OPENOCD) $(OCD_ARGS) -c "init; chip_info; gpio_dump A; flash banks; exit"
