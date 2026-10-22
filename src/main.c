/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "debug.h"
#include "Modbus.h"
#include "ModbusPort.h"
#include "modbus_registers.h"
#include "board.h"
#include "soft_timer.h"
#include "eeprom.h"
#include "meter.h"
#include "powermon.h"
#include "settings.h"
#include "interpolation.h"

/* Global Variable */
soft_timer_t timer_100ms;
soft_timer_t timer_10ms;

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    BoardGpioInit();
    BoardTimer2Init();

    // 1ms tick
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    powermon_init(); /* PVD: контроль питания, прерывание по обоим фронтам */
    meterInit();     /* загрузка настроек ДО InitModbus: адрес берётся из g_settings */

    /* адрес слейва из настроек (дефолт 1); вызывать строго после meterInit,
     * иначе g_settings ещё нулевая и адрес станет 0 = broadcast → молчание */
    InitModbus(g_settings.interfaceRS485_slave_address);

    /* автовосстановление критических данных из flash */
    critical_payload_t critical;
    if (critical_init(&critical))
    {
        runtime_device = critical.runtime_device;
        runtime_flowmeter = critical.runtime_flowmeter;
        impulses = critical.impulses;
        totalnl = critical.totalnl;
        gtotalnl = critical.gtotalnl;
        poweron_counter = critical.poweron_counter;
    }
    /* контрольная точка при старте: свежая запись в журнал при стабильном
     * питании — заодно проверка тракта записи (см. critical_debug) */
    critical.runtime_device = runtime_device;
    critical.runtime_flowmeter = runtime_flowmeter;
    critical.impulses = impulses;
    critical.totalnl = totalnl;
    critical.gtotalnl = gtotalnl;
    critical.poweron_counter = poweron_counter; /* uint16, см. critical_payload_t */
    critical_save(&critical, false);

    SoftTimerInit(&timer_100ms, 100, timer_100ms_callback);
    SoftTimerInit(&timer_10ms, 10, timer_10ms_callback);

    bool critical_saved = false;

    while (1)
    {
        if (powermon_failed())
        {
            if (!critical_saved)
            {
                /* разовая фиксация: сохранить и остановить всё */
                critical_payload_t critical = {
                    .runtime_device = runtime_device,
                    .runtime_flowmeter = runtime_flowmeter,
                    .impulses = impulses,
                    .totalnl = totalnl,
                    .gtotalnl = gtotalnl,
                    .poweron_counter = poweron_counter};
                critical_debug |= 0x10; /* бит4: вход в аварийный путь */
                if (critical_save(&critical, true))
                    critical_debug |= 0x20; /* бит5: аварийная запись легла */
                else
                    critical_debug |= 0x40; /* бит6: запись НЕ легла */
                critical_saved = true;

                USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); /* стоп обмен */
                NVIC_DisableIRQ(TIM2_IRQn);                     /* стоп расчёты */
                GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
            }
            /* ждём: либо питание окончательно пропадёт, либо PVD снимет аварию */
            continue;
        }

        if (critical_saved)
        {
            /* питание восстановилось — продолжаем работу */
            critical_saved = false;
            GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
            NVIC_EnableIRQ(TIM2_IRQn);
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
        }

        ProcessModbus();
        SoftTimerCheck(&timer_100ms);
        SoftTimerCheck(&timer_10ms);
    }
    return 0;
}

/* interrupts */

/*******************************************************************************
 * @brief System tick interrupt
 * @param None
 * @return None
 ******************************************************************************/
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SoftTimerInc(1);
    ModBus_TimerValues();
    SysTick->SR = 0;
}
