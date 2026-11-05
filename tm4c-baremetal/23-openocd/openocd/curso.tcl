# =============================================================================
#  curso.tcl - comandos próprios para o OpenOCD (TM4C123 / TM4C129)
#  Depois de "telnet localhost 4444":   ajuda_curso
# =============================================================================

proc rd {addr} {
    return [format 0x%08x [lindex [read_memory $addr 32 1] 0]]
}

proc wr {addr val} {
    write_memory $addr 32 [list $val]
}

proc setbits {addr mask} { wr $addr [expr {[rd $addr] | $mask}] }
proc clrbits {addr mask} { wr $addr [expr {[rd $addr] & ~$mask}] }

# DID0.CLASS: 0x05 = TM4C123, 0x0A = TM4C129
proc chip_class {} {
    return [expr {([rd 0x400FE000] >> 16) & 0xFF}]
}

proc chip_info {} {
    set did0 [rd 0x400FE000]
    set did1 [rd 0x400FE004]
    switch [chip_class] {
        5       { set nome "TM4C123" }
        10      { set nome "TM4C129" }
        default { set nome "?" }
    }
    echo "CPUID = [rd 0xE000ED00]"
    echo [format "DID0  = %s  (%s, revisao %c%d)" $did0 $nome \
              [expr {65 + (($did0 >> 8) & 0xFF)}] [expr {$did0 & 0xFF}]]
    echo [format "DID1  = %s  (PARTNO 0x%02x)" $did1 [expr {($did1 >> 16) & 0xFF}]]
    echo "Flash = [expr {(([rd 0x400FDFC0] & 0xFFFF) + 1) * 2}] KB"
    echo "SRAM  = [expr {(([rd 0x400FDFC4] & 0xFFFF) + 1) / 4}] KB"
}

# Endereço-base de uma porta GPIO
proc gpio_base {porta} {
    set p [string toupper $porta]
    if {[chip_class] == 5} {
        # TM4C123, abertura APB
        array set b {A 0x40004000 B 0x40005000 C 0x40006000 D 0x40007000 E 0x40024000 F 0x40025000}
        return $b($p)
    }
    # TM4C129, AHB: A..H, J..N, P, Q (sem I e O)
    set letras {A B C D E F G H J K L M N P Q}
    return [expr {0x40058000 + 0x1000 * [lsearch $letras $p]}]
}

proc gpio_dump {porta} {
    set b [gpio_base $porta]
    foreach {nome off} {DATA 0x3FC DIR 0x400 AFSEL 0x420 DR2R 0x500 ODR 0x50C PUR 0x510 PDR 0x514 DEN 0x51C AMSEL 0x528 PCTL 0x52C} {
        echo [format "GPIO%s_%-6s @ 0x%08x = %s" [string toupper $porta] $nome [expr {$b + $off}] [rd [expr {$b + $off}]]]
    }
}

# LED do curso: PF3 (123) ou PN1 (129), pelo DATA mascarado (sem parar a CPU)
proc led {estado} {
    if {[chip_class] == 5} {
        set a [expr {[gpio_base F] + ((1 << 3) << 2)}]
    } else {
        set a [expr {[gpio_base N] + ((1 << 1) << 2)}]
    }
    if {$estado eq "on"} { wr $a 0xFF } else { wr $a 0x00 }
}

# Quais periféricos estão com clock (RCGCxxx)
proc clocks {} {
    foreach {nome off} {WD 0x600 TIMER 0x604 GPIO 0x608 DMA 0x60C HIB 0x614 UART 0x618 SSI 0x61C I2C 0x620 CAN 0x634 ADC 0x638 PWM 0x640 QEI 0x644} {
        echo [format "RCGC%-6s = %s" $nome [rd [expr {0x400FE000 + $off}]]]
    }
}

proc fault_info {} {
    echo "CFSR  = [rd 0xE000ED28]   (MMFSR[7:0] BFSR[15:8] UFSR[31:16])"
    echo "HFSR  = [rd 0xE000ED2C]   (bit30 FORCED = falha escalada)"
    echo "MMFAR = [rd 0xE000ED34]"
    echo "BFAR  = [rd 0xE000ED38]"
    echo "RESC  = [rd 0x400FE05C]   (causa do último reset)"
}

proc gravar {elf} {
    reset halt
    program $elf verify
    reset run
}

proc semi {} {
    arm semihosting enable
    reset run
}

proc ajuda_curso {} {
    echo "rd <end> | wr <end> <val> | setbits/clrbits <end> <mascara>"
    echo "chip_info           DID0/DID1, Flash, SRAM"
    echo "gpio_dump <A..Q>    registradores de uma porta"
    echo "led on|off          LED do curso (PF3 / PN1)"
    echo "clocks              registradores RCGC"
    echo "fault_info          CFSR/HFSR/BFAR/RESC"
    echo "gravar <arquivo.elf> | semi (liga semihosting e reinicia)"
}
