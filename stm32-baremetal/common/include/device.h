/*
 * device.h - seleciona o header de registradores conforme o MCU.
 * O Makefile define -DSTM32F0 ou -DSTM32F4.
 */
#ifndef DEVICE_H
#define DEVICE_H

#if defined(STM32F0)
#  include "stm32f0.h"
#elif defined(STM32F4)
#  include "stm32f4.h"
#else
#  error "Defina STM32F0 ou STM32F4 (o Makefile faz isso com MCU=f0/f4)"
#endif

#endif
