/* =============================================================================
 *  startup.S - inicialização do AT32UC3A0512 (AVR32 UC3), em assembly
 *  Explicado em 02-startup/README.md
 *
 *  Diferenças em relação ao AVR de 8 bits e ao ARM Cortex-M:
 *   - no reset o PC vale 0x8000_0000 (início da Flash) e a CPU simplesmente
 *     executa o que estiver lá: não há "valor inicial do SP" nem tabela no
 *     endereço do reset;
 *   - exceções e interrupções usam uma tabela apontada pelo registrador de
 *     sistema EVBA, que o startup precisa programar;
 *   - a placa EVK1100 vem com um bootloader USB (DFU) nos primeiros 8 KB da
 *     Flash, que salta para 0x8000_2000. Por isso o programa começa ali, e um
 *     pequeno "trampolim" em 0x8000_0000 cobre o caso de gravação por JTAG.
 * ========================================================================== */

#define SYSREG_EVBA 4

/* ---------------- Trampolim em 0x8000_0000 ---------------- */
    .section .reset, "ax", @progbits
    .global _trampoline
_trampoline:
    lddpc   pc, 1f                  /* PC = endereço de _start (lido da Flash) */
    .align  2
1:  .word   _start

/* ---------------- Código de inicialização (em 0x8000_2000) ---------------- */
    .section .text._start, "ax", @progbits
    .global _start
    .type   _start, @function
_start:
    /* 1) Pilha no fim da SRAM. No AVR32 a pilha também cresce para baixo. */
    lddpc   sp, .Lestack

    /* 2) Tabela de exceções: EVBA = _evba */
    lddpc   r0, .Levba
    mtsr    SYSREG_EVBA, r0

    /* 3) Libera as exceções (bit EM do registrador de status). As
     *    interrupções continuam mascaradas (bit GM) até o programa ligar. */
    csrf    21

    /* 4) Copia .data da Flash para a SRAM, uma word por vez */
    lddpc   r0, .Lsidata
    lddpc   r1, .Lsdata
    lddpc   r2, .Ledata
1:  cp.w    r1, r2
    brhs    2f                      /* r1 >= r2 (sem sinal): terminou */
    ld.w    r3, r0++                /* r3 = *r0; r0 += 4 */
    st.w    r1++, r3                /* *r1 = r3; r1 += 4 */
    rjmp    1b

    /* 5) Zera .bss */
2:  lddpc   r1, .Lsbss
    lddpc   r2, .Lebss
    mov     r3, 0
3:  cp.w    r1, r2
    brhs    4f
    st.w    r1++, r3
    rjmp    3b

    /* 6) main() */
4:  rcall   main
5:  rjmp    5b

    .align  2
.Lestack: .word _estack
.Levba:   .word _evba
.Lsidata: .word _sidata
.Lsdata:  .word _sdata
.Ledata:  .word _edata
.Lsbss:   .word _sbss
.Lebss:   .word _ebss

/* =============================================================================
 *  Tabela de exceções (EVBA). Cada exceção tem um endereço fixo a partir de
 *  EVBA. Aqui todas as falhas ficam paradas num laço ("rjmp $"), num lugar
 *  conhecido para o depurador.
 * ========================================================================== */
    .section .exception, "ax", @progbits
    .balign 0x200
    .global _evba
_evba:
    .org 0x000
_exc_unrecoverable:     rjmp _exc_unrecoverable
    .org 0x004
_exc_tlb_multiple:      rjmp _exc_tlb_multiple
    .org 0x008
_exc_bus_data:          rjmp _exc_bus_data          /* erro de barramento ao ler/escrever dado */
    .org 0x00C
_exc_bus_instr:         rjmp _exc_bus_instr
    .org 0x010
_exc_nmi:               rjmp _exc_nmi
    .org 0x014
_exc_instr_addr:        rjmp _exc_instr_addr
    .org 0x018
_exc_itlb_prot:         rjmp _exc_itlb_prot
    .org 0x01C
_exc_breakpoint:        rjmp _exc_breakpoint
    .org 0x020
_exc_illegal_opcode:    rjmp _exc_illegal_opcode
    .org 0x024
_exc_unimpl_instr:      rjmp _exc_unimpl_instr
    .org 0x028
_exc_privilege:         rjmp _exc_privilege
    .org 0x02C
_exc_fp:                rjmp _exc_fp
    .org 0x030
_exc_coproc_absent:     rjmp _exc_coproc_absent
    .org 0x034
_exc_data_addr_read:    rjmp _exc_data_addr_read    /* leitura desalinhada */
    .org 0x038
_exc_data_addr_write:   rjmp _exc_data_addr_write   /* escrita desalinhada */
    .org 0x03C
_exc_dtlb_prot_read:    rjmp _exc_dtlb_prot_read
    .org 0x040
_exc_dtlb_prot_write:   rjmp _exc_dtlb_prot_write
    .org 0x044
_exc_dtlb_modified:     rjmp _exc_dtlb_modified
    .org 0x050
_exc_itlb_miss:         rjmp _exc_itlb_miss
    .org 0x060
_exc_dtlb_miss_read:    rjmp _exc_dtlb_miss_read
    .org 0x070
_exc_dtlb_miss_write:   rjmp _exc_dtlb_miss_write
    .org 0x100
_exc_supervisor_call:   rjmp _exc_supervisor_call

/* =============================================================================
 *  Entradas das interrupções (uma por NÍVEL de prioridade, 0 a 3).
 *  O INTC salta para EVBA + "autovetor"; intc.c programa o autovetor de
 *  cada grupo para apontar para _int0.._int3. O hardware já salvou
 *  R8-R12, LR, PC e SR. Aqui descobrimos qual grupo causou a interrupção,
 *  buscamos a função C registrada e saltamos para ela (ela termina com rete).
 * ========================================================================== */
    .balign 4
    .global _int0, _int1, _int2, _int3
_int0:  mov     r12, 0
        rjmp    _int_common
_int1:  mov     r12, 1
        rjmp    _int_common
_int2:  mov     r12, 2
        rjmp    _int_common
_int3:  mov     r12, 3
_int_common:
        rcall   intc_get_handler    /* r12 = função a chamar (ou 0) */
        cp.w    r12, 0
        movne   pc, r12             /* salta para o handler (que termina com rete) */
        rete                        /* nenhuma registrada: só retorna */
