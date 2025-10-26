#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define LED1_PORT   GPIOB
#define LED1_PIN    GPIO_Pin_5

#define LED2_PORT   GPIOB
#define LED2_PIN    GPIO_Pin_11

#define BTN_PORT    GPIOA
#define BTN_PIN     GPIO_Pin_1


SemaphoreHandle_t xBtnSem = NULL;


void Task_LED1(void *pvParam);
void Task_LED2(void *pvParam);
static void GPIO_Config(void);
static void EXTI_Config(void);


int main(void)
{
    SystemInit();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    GPIO_Config();
    EXTI_Config();

    xBtnSem = xSemaphoreCreateBinary();

    xTaskCreate(Task_LED1, "Blink", 128, NULL, 1, NULL);
    xTaskCreate(Task_LED2, "Button", 128, NULL, 2, NULL);

    vTaskStartScheduler();
    while (1);
}


void Task_LED1(void *pvParam)
{
    for (;;)
    {
        LED1_PORT->ODR ^= LED1_PIN; 
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void Task_LED2(void *pvParam)
{
    for (;;)
    {
        if (xSemaphoreTake(xBtnSem, portMAX_DELAY) == pdTRUE)
        {
            GPIO_SetBits(LED2_PORT, LED2_PIN);
            vTaskDelay(pdMS_TO_TICKS(1000));
            GPIO_ResetBits(LED2_PORT, LED2_PIN);
        }
    }
}


static void GPIO_Config(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

  
    gpio.GPIO_Pin = LED1_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED1_PORT, &gpio);
    GPIO_SetBits(LED1_PORT, LED1_PIN); 


    gpio.GPIO_Pin = LED2_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED2_PORT, &gpio);
    GPIO_ResetBits(LED2_PORT, LED2_PIN); 

   
    gpio.GPIO_Pin = BTN_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BTN_PORT, &gpio);
}


static void EXTI_Config(void)
{
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

    exti.EXTI_Line = EXTI_Line1;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling; 
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    EXTI_ClearITPendingBit(EXTI_Line1);

    nvic.NVIC_IRQChannel = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 8;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}


void EXTI1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line1);
        xSemaphoreGiveFromISR(xBtnSem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}