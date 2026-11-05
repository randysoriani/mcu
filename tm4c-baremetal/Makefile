# =============================================================================
#  Makefile raiz: compila TODOS os módulos para os dois MCUs (teste de sanidade)
#     make            -> compila tudo (f0 e f4)
#     make clean      -> limpa tudo
#     make MCUS=f4    -> só um MCU
# =============================================================================
MODULES := $(sort $(dir $(wildcard [0-9][0-9]-*/Makefile)))
MCUS    ?= 123 129

.PHONY: all clean $(MODULES)

all:
	@set -e; for m in $(MODULES); do for u in $(MCUS); do \
	    printf "==> %-32s %s\n" "$$m" "$$u"; \
	    $(MAKE) -s -C $$m MCU=$$u > /dev/null || { echo "FALHOU: $$m $$u"; exit 1; }; \
	done; done; echo "OK: todos os módulos compilaram."

clean:
	@for m in $(MODULES); do $(MAKE) -s -C $$m clean; done
