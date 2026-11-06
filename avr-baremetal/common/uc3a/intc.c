/*
 * intc.c (UC3A) - tabela de rotinas de interrupção por grupo.
 * Explicado no módulo 06.
 *
 * O INTC do UC3 agrupa as interrupções: cada periférico tem um GRUPO e
 * cada grupo tem um registrador IPR com o nível de prioridade (0 a 3) e o
 * "autovetor": a distância, a partir de EVBA, do código a executar.
 * Todos os grupos de um mesmo nível apontam para a mesma entrada
 * (_int0.._int3 no startup.S), que chama intc_get_handler() para descobrir
 * qual grupo foi e qual função C chamar.
 */
#include "system.h"

extern char _evba[], _int0[], _int1[], _int2[], _int3[];

static irq_handler_t s_handlers[64];

void intc_init(void)
{
    for (uint32_t g = 0; g < 64; g++) s_handlers[g] = 0;
}

void intc_register(uint32_t group, irq_handler_t handler, uint32_t level)
{
    static char *const entry[4] = { _int0, _int1, _int2, _int3 };
    level &= 3U;
    s_handlers[group] = handler;
    INTC->IPR[group] = (level << INTC_IPR_INTLEVEL_Pos)
                     | (uint32_t)(entry[level] - _evba);   /* autovetor = offset desde EVBA */
}

/* Chamado pelo assembly com o nível (0..3); devolve a função do grupo */
irq_handler_t intc_get_handler(uint32_t level) __attribute__((used));
irq_handler_t intc_get_handler(uint32_t level)
{
    uint32_t group = INTC->ICR[3U - level];   /* ICR[0] é o nível 3 */
    return s_handlers[group & 63U];
}
