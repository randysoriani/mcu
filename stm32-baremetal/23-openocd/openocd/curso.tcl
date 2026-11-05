# =============================================================================
#  curso.tcl - comandos próprios para o OpenOCD (STM32F0/F4)
#
#  Carregado por curso.cfg. Depois de "telnet localhost 4444", digite:
#     ajuda_curso
# =============================================================================

# ---- leitura/escrita de 32 bits -------------------------------------------
# read_memory/write_memory existem a partir do OpenOCD 0.12.
proc rd {addr} {
    return [format 0x%08x [lindex [read_memory $addr 32 1] 0]]
}

proc wr {addr val} {
    write_memory $addr 32 [list $val]
}

# Seta / limpa bits sem mexer nos outros (read-modify-write)
proc setbits {addr mask} { wr $addr [expr {[rd $addr] | $mask}] }
proc clrbits {addr mask} { wr $addr [expr {[rd $addr] & ~$mask}] }

# ---- identificação ----------------------------------------------------------
proc chip_info {} {
    set cpuid [rd 0xE000ED00]
    set part  [expr {($cpuid >> 4) & 0xFFF}]
    switch $part {
        3104    { set nucleo "Cortex-M0" }
        3108    { set nucleo "Cortex-M4" }
        default { set nucleo "?" }
    }
    echo "CPUID  = $cpuid ($nucleo)"
    # DBGMCU_IDCODE: F0 em 0x40015800, F4 em 0xE0042000
    if {$part == 3104} { set id [rd 0x40015800] } else { set id [rd 0xE0042000] }
    echo [format "IDCODE = %s (DEV_ID 0x%03x)" $id [expr {$id & 0xFFF}]]
    if {$part == 3104} { set fs 0x1FFFF7CC } else { set fs 0x1FFF7A22 }
    echo "Flash  = [expr {[rd $fs] & 0xFFFF}] KB"
}

# ---- GPIO --------------------------------------------------------------------
proc gpio_base {porta} {
    set idx [expr {[scan [string toupper $porta] %c] - 65}]
    set part [expr {([rd 0xE000ED00] >> 4) & 0xFFF}]
    if {$part == 3104} {
        return [expr {0x48000000 + 0x400 * $idx}]      ;# F0: AHB2
    } else {
        return [expr {0x40020000 + 0x400 * $idx}]      ;# F4: AHB1
    }
}

proc gpio_dump {porta} {
    set b [gpio_base $porta]
    foreach {nome off} {MODER 0x00 OTYPER 0x04 OSPEEDR 0x08 PUPDR 0x0C IDR 0x10 ODR 0x14 AFRL 0x20 AFRH 0x24} {
        echo [format "GPIO%s_%-8s @ 0x%08x = %s" [string toupper $porta] $nome [expr {$b + $off}] [rd [expr {$b + $off}]]]
    }
}

# LED LD2 = PA5. Escreve no BSRR: não precisa parar a CPU.
proc led {estado} {
    set bsrr [expr {[gpio_base A] + 0x18}]
    if {$estado eq "on"} { wr $bsrr [expr {1 << 5}] } else { wr $bsrr [expr {1 << 21}] }
}

# ---- falhas (só Cortex-M4 tem CFSR/HFSR) ---------------------------------
proc fault_info {} {
    echo "CFSR  = [rd 0xE000ED28]   (MMFSR[7:0] BFSR[15:8] UFSR[31:16])"
    echo "HFSR  = [rd 0xE000ED2C]   (bit30 FORCED = falha escalada)"
    echo "MMFAR = [rd 0xE000ED34]"
    echo "BFAR  = [rd 0xE000ED38]"
}

# ---- fluxo comum ------------------------------------------------------------
proc gravar {elf} {
    reset halt
    program $elf verify
    reset run
}

# Liga semihosting e reinicia o programa
proc semi {} {
    arm semihosting enable
    reset run
}

proc ajuda_curso {} {
    echo "rd <end>            lê word        | wr <end> <val>     escreve word"
    echo "setbits/clrbits     RMW de bits    | chip_info          CPUID/IDCODE/Flash"
    echo "gpio_dump <A..H>    registradores  | led on|off         LD2 (PA5)"
    echo "fault_info          CFSR/HFSR (M4) | gravar <arquivo.elf>"
    echo "semi                liga semihosting e reinicia"
}
