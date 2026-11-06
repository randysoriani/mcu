/* =============================================================================
 *  startup.S - inicialização do ATmega328P, em assembly AVR
 *  Explicado em 02-startup/README.md
 *
 *  No AVR de 8 bits a tabela de vetores NÃO é uma lista de endereços: é uma
 *  lista de INSTRUÇÕES de salto ("jmp"), uma por vetor, a partir do
 *  endereço 0 da Flash. No reset o PC vale 0 e executa "jmp __init".
 * ========================================================================== */

#define SREG_IO 0x3F            /* endereços no espaço de E/S (dados - 0x20) */
#define SPH_IO  0x3E
#define SPL_IO  0x3D

    .section .vectors, "ax", @progbits
    .global __vectors
__vectors:
    jmp __init                  ;  0 RESET
    jmp __vector_1              ;  1 INT0
    jmp __vector_2              ;  2 INT1
    jmp __vector_3              ;  3 PCINT0
    jmp __vector_4              ;  4 PCINT1
    jmp __vector_5              ;  5 PCINT2
    jmp __vector_6              ;  6 WDT
    jmp __vector_7              ;  7 TIMER2_COMPA
    jmp __vector_8              ;  8 TIMER2_COMPB
    jmp __vector_9              ;  9 TIMER2_OVF
    jmp __vector_10             ; 10 TIMER1_CAPT
    jmp __vector_11             ; 11 TIMER1_COMPA
    jmp __vector_12             ; 12 TIMER1_COMPB
    jmp __vector_13             ; 13 TIMER1_OVF
    jmp __vector_14             ; 14 TIMER0_COMPA
    jmp __vector_15             ; 15 TIMER0_COMPB
    jmp __vector_16             ; 16 TIMER0_OVF
    jmp __vector_17             ; 17 SPI_STC
    jmp __vector_18             ; 18 USART_RX
    jmp __vector_19             ; 19 USART_UDRE
    jmp __vector_20             ; 20 USART_TX
    jmp __vector_21             ; 21 ADC
    jmp __vector_22             ; 22 EE_READY
    jmp __vector_23             ; 23 ANALOG_COMP
    jmp __vector_24             ; 24 TWI
    jmp __vector_25             ; 25 SPM_READY

/* Vetores "fracos": se o programa não definir ISR(...), caem em __bad_interrupt */
    .macro WEAK_VECTOR n
    .weak __vector_\n
    .set  __vector_\n, __bad_interrupt
    .endm
    WEAK_VECTOR 1
    WEAK_VECTOR 2
    WEAK_VECTOR 3
    WEAK_VECTOR 4
    WEAK_VECTOR 5
    WEAK_VECTOR 6
    WEAK_VECTOR 7
    WEAK_VECTOR 8
    WEAK_VECTOR 9
    WEAK_VECTOR 10
    WEAK_VECTOR 11
    WEAK_VECTOR 12
    WEAK_VECTOR 13
    WEAK_VECTOR 14
    WEAK_VECTOR 15
    WEAK_VECTOR 16
    WEAK_VECTOR 17
    WEAK_VECTOR 18
    WEAK_VECTOR 19
    WEAK_VECTOR 20
    WEAK_VECTOR 21
    WEAK_VECTOR 22
    WEAK_VECTOR 23
    WEAK_VECTOR 24
    WEAK_VECTOR 25

    .section .init, "ax", @progbits
    .global __init
__init:
    /* 1) r1 = 0: o GCC usa r1 como "registrador zero" fixo */
    clr     r1
    /* 2) SREG = 0: interrupções desligadas */
    out     SREG_IO, r1
    /* 3) Pilha no fim da RAM. O SP do AVR aponta para a PRIMEIRA posição
     *    livre e decrementa DEPOIS de cada push, por isso começa em RAMEND */
    ldi     r28, lo8(__stack)
    ldi     r29, hi8(__stack)
    out     SPH_IO, r29
    out     SPL_IO, r28

    /* 4) Copia .data: Flash (Z, lida com LPM) -> RAM (X) */
    ldi     r26, lo8(__data_start)
    ldi     r27, hi8(__data_start)
    ldi     r30, lo8(__data_load_start)
    ldi     r31, hi8(__data_load_start)
    ldi     r17, hi8(__data_end)
    rjmp    2f
1:  lpm     r0, Z+                  ; r0 = Flash[Z], Z++  (LPM lê a memória de programa)
    st      X+, r0                  ; RAM[X] = r0, X++
2:  cpi     r26, lo8(__data_end)    ; compara X com o fim (16 bits: CPI + CPC)
    cpc     r27, r17
    brne    1b

    /* 5) Zera .bss */
    ldi     r26, lo8(__bss_start)
    ldi     r27, hi8(__bss_start)
    ldi     r18, hi8(__bss_end)
    rjmp    4f
3:  st      X+, r1                  ; r1 vale 0
4:  cpi     r26, lo8(__bss_end)
    cpc     r27, r18
    brne    3b

    /* 6) main() */
    call    main

    /* 7) main não deveria retornar: desliga interrupções e fica parado */
    cli
5:  rjmp    5b

    .section .text.__bad_interrupt, "ax", @progbits
    .global __bad_interrupt
__bad_interrupt:
    /* interrupção habilitada sem ISR: reinicia o programa (como a avr-libc) */
    jmp     __vectors
