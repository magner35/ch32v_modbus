#ifndef _BOARD_H_
#define _BOARD_H

#include "ch32v00x_gpio.h"

// Output signal
#define LED1_GPIO_PORT (GPIOD)
#define LED1_GPIO_PIN (GPIO_Pin_3)
#define LED2_GPIO_PORT (GPIOD)
#define LED2_GPIO_PIN (GPIO_Pin_2)

// Serial port transceiver enable
#define RS485_DE_GPIO_PORT (GPIOD)
#define RS485_DE_GPIO_PIN (GPIO_Pin_4)

void BoardGpioInit(void);

#endif // _BOARD_H_