# sessao.gdb - roteiro do módulo 21 (ATmega328P no QEMU)
#
#   terminal 1:  make MCU=m328p EXTRA=-DQEMU OPT=-Og && make MCU=m328p qemu-gdb
#   terminal 2:  avr-gdb -q -x gdb/sessao.gdb build/m328p/21-depuracao-e-simulacao.elf
#                (ou "gdb" do sistema, se ele tiver suporte a AVR)
#   e digite m, o, p no terminal 1 quando o GDB continuar.
set pagination off
target remote :1234

# Bug 1: media() devolve valor errado
break media
continue
bt
info args
finish

# Bug 2: quem altera cfg.limite? (o QEMU AVR não tem watchpoint de hardware:
# o GDB executa passo a passo e compara o valor - lento, mas funciona)
set can-use-hw-watchpoints 0
watch cfg.limite
continue
bt 2
print cfg

# Bug 3: em qual iteração processa() falha?
delete
break processa if (i & 0x2E1) == 0x2E1
continue
print i
info registers r24 r25 SREG SP pc
x/8xb &cfg
kill
