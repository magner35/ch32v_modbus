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

/* Прежняя реализация на SPL — для справки/отката, в прошивку не линкуется. */
void powermon_init_SPL(void)
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

/* Инициализация PVD — прямые записи в регистры. Эквивалент
 * powermon_init_SPL, экономит ~250 Б flash (EXTI_Init, NVIC_Init,
 * PWR_*, RCC_APBxPeriphClockCmd). */
void powermon_init(void)
{
    /* тактирование PWR (APB1) и AFIO (APB2) */
    RCC->APB1PCENR |= RCC_APB1Periph_PWR;
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO;

    /* PVD: порог POWERMON_PVD_LEVEL (PLS[2:0], биты 7:5) + включение PVDE */
    PWR->CTLR = (PWR->CTLR & ~((uint32_t)PWR_CTLR_PLS)) | POWERMON_PVD_LEVEL | PWR_CTLR_PVDE;

    /* выход PVD → EXTI Line8: прерывание (не событие) по обоим фронтам */
    EXTI->INTENR |= EXTI_INTENR_MR8; /* маска прерывания линии 8 */
    EXTI->RTENR |= EXTI_INTENR_MR8;  /* триггер по фронту */
    EXTI->FTENR |= EXTI_INTENR_MR8;  /* триггер по спаду */

    /* авария питания — высший приоритет */
    NVIC_SetPriority(PVD_IRQn, 0x00);
    NVIC_EnableIRQ(PVD_IRQn);

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
