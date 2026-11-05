/*
 * Módulo 20 - RTC no módulo de Hibernação (HIB)
 *
 * O "RTC" do TM4C é um contador de SEGUNDOS de 32 bits (RTCC) + sub-segundos
 * (RTCSS, 1/32768 s), alimentado pelo cristal de 32,768 kHz do HIB (presente
 * nas duas LaunchPads) e pelo VBAT. Data/hora é calculada por software a
 * partir de um "epoch" (aqui: segundos desde 1970-01-01, como no Unix).
 *   - 1ª execução: acerta com a hora da COMPILAÇÃO e grava um magic em HIB->DATA[0]
 *   - depois: continua contando através de resets
 *   - alarme por comparação (RTCM0) todo minuto no segundo 30
 * Comando pela serial: 'r' = reacertar com a hora da compilação
 *
 * TODA escrita em registrador do HIB precisa esperar WRC (escrita concluída),
 * porque o módulo roda no domínio lento de 32 kHz.
 */
#include <string.h>
#include "system.h"

#define HIB_MAGIC 0x32F1C0DEU

static void hib_wait(void) { while (!(HIB->CTL & HIB_CTL_WRC)) { } }
static void hib_write(volatile uint32_t *reg, uint32_t v) { hib_wait(); *reg = v; hib_wait(); }

/* ---- calendário <-> epoch (algoritmo de Howard Hinnant, "days from civil") ---- */
static int32_t days_from_civil(int32_t y, uint32_t m, uint32_t d)
{
    y -= m <= 2;
    int32_t  era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153U * (m + (m > 2 ? -3 : 9)) + 2U) / 5U + d - 1U;
    uint32_t doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

static void civil_from_days(int32_t z, int32_t *y, uint32_t *m, uint32_t *d)
{
    z += 719468;
    int32_t  era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    uint32_t mp  = (5U * doy + 2U) / 153U;
    *d = doy - (153U * mp + 2U) / 5U + 1U;
    *m = mp < 10 ? mp + 3 : mp - 9;
    *y = (int32_t)yoe + era * 400 + (*m <= 2);
}

static uint32_t build_epoch(void)
{
    static const char nomes[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__, *t = __TIME__;   /* "Nov  4 2026", "21:40:00" */
    uint32_t mes = 1;
    for (int i = 0; i < 12; i++) if (memcmp(d, &nomes[i * 3], 3) == 0) mes = (uint32_t)i + 1U;
    uint32_t dia = (uint32_t)((d[4] == ' ' ? 0 : d[4] - '0') * 10 + (d[5] - '0'));
    int32_t  ano = (d[7] - '0') * 1000 + (d[8] - '0') * 100 + (d[9] - '0') * 10 + (d[10] - '0');
    uint32_t hh = (uint32_t)((t[0] - '0') * 10 + (t[1] - '0'));
    uint32_t mm = (uint32_t)((t[3] - '0') * 10 + (t[4] - '0'));
    uint32_t ss = (uint32_t)((t[6] - '0') * 10 + (t[7] - '0'));
    return (uint32_t)days_from_civil(ano, mes, dia) * 86400U + hh * 3600U + mm * 60U + ss;
}

static void rtc_set(uint32_t epoch)
{
    hib_write(&HIB->RTCLD, epoch);                 /* carrega o contador */
    hib_write(&HIB->DATA[0], HIB_MAGIC);
    kprintf("RTC acertado com %s %s (epoch %u)\n", __DATE__, __TIME__, epoch);
}

static void rtc_init(void)
{
    periph_enable(&SYSCTL_RCGCHIB, &SYSCTL_PRHIB, 0);
    if (!(HIB->CTL & HIB_CTL_CLK32EN)) {
        hib_write(&HIB->CTL, HIB_CTL_CLK32EN);     /* liga o oscilador de 32 kHz */
        delay_ms(1500);                            /* cristal leva até ~1,5 s para estabilizar */
    }
    if (HIB->DATA[0] != HIB_MAGIC || !(HIB->CTL & HIB_CTL_RTCEN)) {
        hib_write(&HIB->CTL, HIB->CTL | HIB_CTL_RTCEN);
        rtc_set(build_epoch());
    } else {
        kprintf("RTC ja estava rodando\n");
    }
}

static void alarm_next(uint32_t now)
{
    uint32_t next = now - (now % 60U) + 30U;       /* próximo segundo 30 */
    if (next <= now) next += 60U;
    hib_write(&HIB->RTCM0, next);
    hib_write(&HIB->IC, HIB_INT_RTCALT0);
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 20: RTC (modulo de hibernacao) ===\n");
    rtc_init();
    alarm_next(HIB->RTCC);

    uint32_t last = 0;
    for (;;) {
        uint32_t s = HIB->RTCC;
        if (s != last) {
            last = s;
            int32_t y; uint32_t mo, d;
            civil_from_days((int32_t)(s / 86400U), &y, &mo, &d);
            uint32_t r = s % 86400U;
            kprintf("%04d-%02u-%02u %02u:%02u:%02u  (RTCC=%u, RTCSS=%5u)\n", y, mo, d,
                    r / 3600U, (r / 60U) % 60U, r % 60U, s, HIB->RTCSS & 0x7FFFU);
        }
        if (HIB->RIS & HIB_INT_RTCALT0) {
            kprintf(">>> ALARME (segundo 30) <<<\n");
            alarm_next(s);
        }
        if (uart_getc() == 'r') rtc_set(build_epoch());
        delay_ms(50);
    }
}
