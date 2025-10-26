#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xUartMutex;


void USART2_Init(uint32_t baudrate)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_2; 
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_3; 
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef usart;
    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &usart);
    USART_Cmd(USART2, ENABLE);
}

void USART2_SendChar(char c)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, c);
}

void USART2_SendString(const char *s)
{
    while (*s)
    {
        USART2_SendChar(*s++);
    }
    USART2_SendChar('\r');
    USART2_SendChar('\n');
}


void TaskA(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        if (xSemaphoreTake(xUartMutex, portMAX_DELAY))
        {
            USART2_SendString("hello");
            xSemaphoreGive(xUartMutex);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000));
    }
}

void TaskB(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        if (xSemaphoreTake(xUartMutex, portMAX_DELAY))
        {
            USART2_SendString("xinchao");
            xSemaphoreGive(xUartMutex);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000));
    }
}


int main(void)
{
    USART2_Init(115200);

    xUartMutex = xSemaphoreCreateMutex();

    xTaskCreate(TaskA, "TaskA", 256, NULL, 1, NULL);
    xTaskCreate(TaskB, "TaskB", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1);
}