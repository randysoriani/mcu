# =============================================================================
#  curso.gdb - comandos e ajustes de GDB para o curso (STM32F0/F4)
#
#  Carregado por "make MCU=f4 gdb" / "make MCU=f4 debug" no módulo 24, ou à mão:
#      (gdb) source gdb/curso.gdb
#  Cada "define" cria um comando novo; "document" cria o texto do "help".
# =============================================================================

set pagination off
set confirm off
set print pretty on
set print array on
set history save on
set history filename ~/.gdb_history_curso

# ---------------------------------------------------------------------------
define recarregar
    monitor reset halt
    load
    monitor reset halt
end
document recarregar
Grava o ELF de novo e deixa o alvo parado no reset (depois de recompilar).
end

# ---------------------------------------------------------------------------
define reiniciar
    monitor reset halt
    continue
end
document reiniciar
Reset do alvo e continua (sem regravar).
end

# ---------------------------------------------------------------------------
define nucleo
    set $cpuid = *(unsigned int *)0xE000ED00
    set $part  = ($cpuid >> 4) & 0xfff
    if $part == 0xc20
        printf "Cortex-M0 (CPUID 0x%08x)\n", $cpuid
    else
        if $part == 0xc24
            printf "Cortex-M4 (CPUID 0x%08x)\n", $cpuid
        else
            printf "CPUID 0x%08x\n", $cpuid
        end
    end
end
document nucleo
Mostra qual núcleo Cortex-M está conectado.
end

# ---------------------------------------------------------------------------
define fault
    printf "--- contexto salvo pelo HardFault_Handler (fault_info) ---\n"
    printf "pc   = 0x%08x   <- onde aconteceu\n", fault_info.pc
    printf "lr   = 0x%08x   <- quem chamou\n", fault_info.lr
    printf "xpsr = 0x%08x   r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x\n", fault_info.xpsr, fault_info.r0, fault_info.r1, fault_info.r2, fault_info.r3
    info symbol fault_info.pc
    info line *fault_info.pc
    set $part = (*(unsigned int *)0xE000ED00 >> 4) & 0xfff
    if $part == 0xc24
        set $cfsr = *(unsigned int *)0xE000ED28
        set $hfsr = *(unsigned int *)0xE000ED2C
        printf "--- registradores de falha (Cortex-M4) ---\n"
        printf "CFSR = 0x%08x   HFSR = 0x%08x\n", $cfsr, $hfsr
        if $hfsr & (1 << 30)
            printf "  HFSR.FORCED   : falha configurável escalada para HardFault\n"
        end
        if $cfsr & (1 << 0)
            printf "  MMFSR.IACCVIOL: execução em região proibida\n"
        end
        if $cfsr & (1 << 1)
            printf "  MMFSR.DACCVIOL: acesso a dado proibido (MPU)\n"
        end
        if $cfsr & (1 << 8)
            printf "  BFSR.IBUSERR  : erro de barramento na busca de instrução\n"
        end
        if $cfsr & (1 << 9)
            printf "  BFSR.PRECISERR: erro de barramento PRECISO em 0x%08x (BFAR)\n", *(unsigned int *)0xE000ED38
        end
        if $cfsr & (1 << 10)
            printf "  BFSR.IMPRECISERR: erro de barramento impreciso (PC aproximado)\n"
        end
        if $cfsr & (1 << 16)
            printf "  UFSR.UNDEFINSTR: instrução indefinida\n"
        end
        if $cfsr & (1 << 17)
            printf "  UFSR.INVSTATE : estado inválido (salto para endereço com bit0 = 0)\n"
        end
        if $cfsr & (1 << 18)
            printf "  UFSR.INVPC    : EXC_RETURN inválido\n"
        end
        if $cfsr & (1 << 19)
            printf "  UFSR.NOCP     : FPU desligada\n"
        end
        if $cfsr & (1 << 24)
            printf "  UFSR.UNALIGNED: acesso desalinhado\n"
        end
        if $cfsr & (1 << 25)
            printf "  UFSR.DIVBYZERO: divisão por zero\n"
        end
    else
        printf "(Cortex-M0: não há CFSR/HFSR; analise o pc e a instrução abaixo)\n"
    end
    printf "--- instruções em volta do pc ---\n"
    x/6i (fault_info.pc & ~1) - 4
end
document fault
Decodifica a última falha: pc, lr, símbolo, linha do fonte e (no M4) CFSR/HFSR.
end

# ---------------------------------------------------------------------------
define gpio
    if $argc != 1
        printf "uso: gpio GPIOA   (ou GPIOB, GPIOC...)\n"
    else
        p/x *$arg0
    end
end
document gpio
Imprime a struct de registradores de uma porta: gpio GPIOA
end

# ---------------------------------------------------------------------------
define pilha
    printf "MSP = 0x%08x, topo (_estack) = 0x%08x, usado = %d bytes\n", $msp, &_estack, (unsigned int)&_estack - $msp
    x/16xw $sp
end
document pilha
Mostra o stack pointer, quanto da pilha está em uso e as 16 words do topo.
end

# ---------------------------------------------------------------------------
define semi
    monitor arm semihosting enable
end
document semi
Liga o semihosting no OpenOCD (as mensagens aparecem no terminal do OpenOCD).
end

printf "curso.gdb carregado: recarregar, reiniciar, nucleo, fault, gpio, pilha, semi (help <cmd>)\n"
