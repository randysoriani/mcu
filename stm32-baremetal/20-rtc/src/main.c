/*
 * Módulo 20 - RTC (Real-Time Clock)
 *
 * - Na primeira execução (ou após perda da bateria/VBAT) inicializa o RTC
 *   com a hora da COMPILAÇÃO (__DATE__/__TIME__) e grava um "magic" num
 *   registrador de backup.
 * - Nos resets seguintes o RTC continua contando: só lê a hora.
 * - Usa o LSE (cristal de 32,768 kHz da Nucleo, X2). Se ele não oscilar,
 *   cai para o LSI (bem menos preciso).
 * - Programa o Alarme A para disparar todo minuto no segundo 30.
 *
 * Comandos pela serial: 'r' = reinicializar o RTC com a hora da compilação
 */
#include <string.h>
#include "system.h"

#define BKP_MAGIC 0x32F1C0DEU

static uint8_t bcd(uint32_t v)   { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }
static uint32_t unbcd(uint32_t v) { return ((v >> 4) & 0xFU) * 10U + (v & 0xFU); }

static void rtc_unlock(void) { RTC->WPR = 0xCA; RTC->WPR = 0x53; }   /* chave de escrita */
static void rtc_lock(void)   { RTC->WPR = 0xFF; }

static void backup_access(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_DBP;            /* Disable Backup domain write Protection */
}

/* Seleciona e liga o clock do RTC. Retorna 1 se LSE, 0 se LSI. */
static int rtc_clock_setup(void)
{
    /* RTCSEL só pode ser trocado após reset do backup domain */
    RCC->BDCR |=  RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

#if defined(STM32F0)
    RCC->BDCR |= RCC_BDCR_LSEDRV_Msk;              /* drive máximo: partida mais garantida */
#endif
    RCC->BDCR |= RCC_BDCR_LSEON;
    uint32_t t0 = millis();
    while (!(RCC->BDCR & RCC_BDCR_LSERDY)) {
        if (millis() - t0 > 3000) break;          /* cristal pode levar ~1-2 s */
    }
    if (RCC->BDCR & RCC_BDCR_LSERDY) {
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_Msk) | RCC_BDCR_RTCSEL_LSE | RCC_BDCR_RTCEN;
        return 1;
    }
    RCC->BDCR &= ~RCC_BDCR_LSEON;
    RCC->CSR |= RCC_CSR_LSION;
    while (!(RCC->CSR & RCC_CSR_LSIRDY)) { }
    RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_Msk) | RCC_BDCR_RTCSEL_LSI | RCC_BDCR_RTCEN;
    return 0;
}

static int mes_de(const char *m)
{
    static const char nomes[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    for (int i = 0; i < 12; i++) if (memcmp(m, &nomes[i * 3], 3) == 0) return i + 1;
    return 1;
}

static void rtc_full_init(void)
{
    int lse = rtc_clock_setup();

    /* f_ck_spre = f_rtcclk / ((PREDIV_A+1) * (PREDIV_S+1)) = 1 Hz */
    uint32_t pa = 127, ps;
#if defined(STM32F0)
    ps = lse ? 255U : 311U;           /* LSI ~40 kHz */
#else
    ps = lse ? 255U : 249U;           /* LSI ~32 kHz */
#endif

    rtc_unlock();
    RTC->ISR |= RTC_ISR_INIT;                     /* entra em modo de inicialização */
    while (!(RTC->ISR & RTC_ISR_INITF)) { }
    RTC->PRER = ps;                               /* duas escritas separadas (RM) */
    RTC->PRER = ps | (pa << 16);

    /* "Nov  4 2026" e "21:40:00" */
    const char *d = __DATE__, *t = __TIME__;
    uint32_t dia = (uint32_t)((d[4] == ' ' ? 0 : d[4] - '0') * 10 + (d[5] - '0'));
    uint32_t ano = (uint32_t)((d[9] - '0') * 10 + (d[10] - '0'));
    uint32_t hh = (uint32_t)((t[0] - '0') * 10 + (t[1] - '0'));
    uint32_t mm = (uint32_t)((t[3] - '0') * 10 + (t[4] - '0'));
    uint32_t ss = (uint32_t)((t[6] - '0') * 10 + (t[7] - '0'));

    RTC->TR = ((uint32_t)bcd(hh) << 16) | ((uint32_t)bcd(mm) << 8) | bcd(ss);
    RTC->DR = ((uint32_t)bcd(ano) << 16) | (1U << 13)            /* WDU = 1 (segunda) */
            | ((uint32_t)bcd((uint32_t)mes_de(d)) << 8) | bcd(dia);
    RTC->CR &= ~RTC_CR_FMT;                       /* 24 h */
    RTC->ISR &= ~RTC_ISR_INIT;                    /* sai: o calendário começa a contar */
    rtc_lock();

    RTC->BKPR[0] = BKP_MAGIC;
    kprintf("RTC inicializado com %s %s usando %s\n", d, t, lse ? "LSE" : "LSI");
}

static void rtc_alarm_a_every_minute(uint32_t segundo)
{
    rtc_unlock();
    RTC->CR &= ~RTC_CR_ALRAE;                     /* desliga para poder escrever */
    while (!(RTC->ISR & RTC_ISR_ALRAWF)) { }
    /* MSK4 (data), MSK3 (hora), MSK2 (minuto) = "não importa"; compara só os segundos */
    RTC->ALRMAR = (1U << 31) | (1U << 23) | (1U << 15) | bcd(segundo);
    RTC->CR |= RTC_CR_ALRAE;
    rtc_lock();
}

static void rtc_read(uint32_t *h, uint32_t *m, uint32_t *s, uint32_t *dd, uint32_t *mo, uint32_t *yy)
{
    /* Ler TR congela os registradores-sombra até ler DR: leia SEMPRE TR e depois DR */
    uint32_t tr = RTC->TR;
    uint32_t dr = RTC->DR;
    *h  = unbcd((tr >> 16) & 0x3FU);
    *m  = unbcd((tr >> 8) & 0x7FU);
    *s  = unbcd(tr & 0x7FU);
    *yy = unbcd((dr >> 16) & 0xFFU);
    *mo = unbcd((dr >> 8) & 0x1FU);
    *dd = unbcd(dr & 0x3FU);
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 20: RTC ===\n");

    backup_access();
    if (RTC->BKPR[0] != BKP_MAGIC) {
        rtc_full_init();
    } else {
        /* LSI não sobrevive a um reset do sistema: religue se for a fonte */
        if ((RCC->BDCR & RCC_BDCR_RTCSEL_Msk) == RCC_BDCR_RTCSEL_LSI) {
            RCC->CSR |= RCC_CSR_LSION;
            while (!(RCC->CSR & RCC_CSR_LSIRDY)) { }
        }
        kprintf("RTC ja estava rodando (%s)\n",
                (RCC->BDCR & RCC_BDCR_RTCSEL_Msk) == RCC_BDCR_RTCSEL_LSE ? "LSE" : "LSI");
    }

    /* Após reset, espera os registradores-sombra sincronizarem (RSF).
     * RSF fica fora dos bits [13:8] -> precisa da chave de escrita. */
    rtc_unlock();
    RTC->ISR &= ~RTC_ISR_RSF;
    rtc_lock();
    while (!(RTC->ISR & RTC_ISR_RSF)) { }

    rtc_alarm_a_every_minute(30);

    uint32_t last_s = 99;
    for (;;) {
        uint32_t h, m, s, dd, mo, yy;
        rtc_read(&h, &m, &s, &dd, &mo, &yy);
        if (s != last_s) {
            last_s = s;
            kprintf("20%02u-%02u-%02u %02u:%02u:%02u  (SSR=%3u)\n", yy, mo, dd, h, m, s, RTC->SSR);
        }
        if (RTC->ISR & RTC_ISR_ALRAF) {
            RTC->ISR &= ~RTC_ISR_ALRAF;           /* bits [13:8] não exigem desbloqueio */
            kprintf(">>> ALARME A (segundo 30) <<<\n");
        }
        if (uart_getc() == 'r') {
            RTC->BKPR[0] = 0;
            rtc_full_init();
        }
        delay_ms(50);
    }
}
