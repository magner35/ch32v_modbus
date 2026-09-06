#include "debug.h"
#include "Modbus.h"

#define MODBUS_ID 0x1

/* interrupts */
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

volatile uint32_t tim_tick = 0;

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    InitModbus(MODBUS_ID);

    while (1)
    {
        ProcessModbus();
    }

    return 0;
}

void USART1_IRQHandler(void)
{
    // receive interrupt
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        ReceiveInterrupt(USART_ReceiveData(USART1));
    }
}

void TIM1_UP_IRQHandler()
{
    if (TIM1->INTFR & TIM_FLAG_Update)
    {

        ModBus_TimerValues();

        tim_tick++;

        if (tim_tick % 1000 == 0)
        {
            Registers[0].ActValue++;
            Registers[1].ActValue++;
            Registers[2].ActValue++;
        }

        TIM1->INTFR = ~TIM_FLAG_Update;
    }
}
