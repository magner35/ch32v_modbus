#ifndef _BOARD_H_
#define _BOARD_H

#include "ch32v00x_gpio.h"

// Output signal
#define LED1_GPIO_PORT (GPIOD)
#define LED1_GPIO_PIN (GPIO_Pin_3)

#define LED2_GPIO_PORT (GPIOD)
#define LED2_GPIO_PIN (GPIO_Pin_2)

#define GEN_OUT_PORT (GPIOC)
#define GEN_OUT_PIN (GPIO_Pin_4)

void BoardGpioInit(void);
void BoardTimer1Init(void);
void BoardTimer2Init(void);

#endif // _BOARD_H_