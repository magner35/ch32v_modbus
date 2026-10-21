/*
 * Created on Tue Oct 20 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "powermon.h"
#include "debug.h"

static volatile bool failed = false;       /* latch из PVD_ISR */
volatile uint8_t powermon_fail_events = 0; /* сколько раз входили в аварию */
volatile uint8_t powermon_debug = 0;

/*******************************************************************************
 * @brief PVD interrupt: оба фронта — вход в аварию и восстановление питания
 ******************************************************************************/
void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void PVD_IRQHandler(void)
{
    EXTI_ClearITPendingBit(EXTI_Line8);
    /* PVDO = SET — напряжение ниже порога */
    bool now_failed = (PWR_GetFlagStatus(PWR_FLAG_PVDO) != RESET);
    if (now_failed && !failed)
    {
        if (powermon_fail_events < 255)
            powermon_fail_events++;
        powermon_debug |= 0x02; /* диагностика: PVD-прерывание срабатывало */
    }
    failed = now_failed;
}

void powermon_init(void)
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    PWR_PVDLevelConfig(POWERMON_PVD_LEVEL);
    PWR_PVDCmd(ENABLE);

    /* выход PVD → EXTI Line8, прерывание по обоим фронтам */
    EXTI_InitStructure.EXTI_Line = EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* авария питания — высший приоритет */
    NVIC_InitStructure.NVIC_IRQChannel = PVD_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    if (PWR_GetFlagStatus(PWR_FLAG_PVDO) != RESET)
        powermon_debug |= 0x01; /* диагностика: PVDO установлен при старте */
}

bool powermon_failed(void)
{
    /* latch из прерывания + прямое чтение флага */
    if (PWR_GetFlagStatus(PWR_FLAG_PVDO) != RESET)
    {
        powermon_debug |= 0x04; /* диагностика: триггер — прямой PVDO */
        return true;
    }
    return failed;
}
