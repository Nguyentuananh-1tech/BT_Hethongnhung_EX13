#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "FreeRTOS.h"
#include "task.h"


static void USART2_Init(uint32_t baudrate)
{

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitTypeDef gpio;
    GPIO_StructInit(&gpio);

  
    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

   
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef usart;
    USART_StructInit(&usart);
    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &usart);

    USART_Cmd(USART2, ENABLE);
}

static inline void uart2_send_char(char c)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {}
    USART_SendData(USART2, (uint16_t)c);
}

static void uart2_send_crlf(void)
{
    uart2_send_char('\r');
    uart2_send_char('\n');
}


static void TaskA(void *pvParameters)
{
    const char *s1 = "hello";
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        for (const char *p = s1; *p; ++p)
        {
            uart2_send_char(*p);
            vTaskDelay(pdMS_TO_TICKS(1)); 
        }
        uart2_send_crlf();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000)); 
    }
}


static void TaskB(void *pvParameters)
{
    const char *s2 = "xinchao";
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        for (const char *p = s2; *p; ++p)
        {
            uart2_send_char(*p);
            vTaskDelay(pdMS_TO_TICKS(1)); 
        }
        uart2_send_crlf();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000)); 
    }
}


int main(void)
{
    USART2_Init(115200);

    
    xTaskCreate(TaskA, "TaskA", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskB, "TaskB", 256, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while (1) {}
}