#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"
#include "lcd.h"
#include "touch.h"

#define LCD_TEAM_NAME_X 40
#define LCD_TEAM_NAME_Y 50
#define LCD_COORD_X_X 100
#define LCD_COORD_X_Y 80
#define LCD_COORD_Y_X 100
#define LCD_COORD_Y_Y 100
#define LCD_LUX_VAL_X 70
#define LCD_LUX_VAL_Y 130

int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};

void RCC_Configure(void);
void GPIO_Configure(void);
void ADC_Configure(void);
void NVIC_Configure(void);
void ADC1_2_IRQHandler(void);
void Delay(void);

uint16_t cur_x, cur_y, pixel_x, pixel_y;
uint16_t value=0;       //value of photo resistor

//---------------------------------------------------------------------------------------------------

void RCC_Configure(void)
{
    //Enable the APB2 peripheral clock using the function 'RCC_APB2PeriphClockCmd'
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_ADC;

    //Initialize the GPIO pins using the structure 'GPIO_InitTypeDef' and the function 'GPIO_Init'
    GPIO_ADC.GPIO_Pin = GPIO_Pin_0;
    GPIO_ADC.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_ADC.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_ADC);
}
void NVIC_Configure(void) {
    NVIC_InitTypeDef NVIC_ADC;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'

    NVIC_EnableIRQ(ADC1_2_IRQn);
    NVIC_ADC.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_ADC.NVIC_IRQChannelPreemptionPriority = 0x0;
    NVIC_ADC.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_ADC.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_ADC);
}

void ADC_Configure(void) {
    ADC_InitTypeDef ADC;

    ADC.ADC_Mode = ADC_Mode_Independent ;
    ADC.ADC_ContinuousConvMode = ENABLE;
    ADC.ADC_DataAlign = ADC_DataAlign_Right;
    ADC.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC.ADC_NbrOfChannel = 1;
    ADC.ADC_ScanConvMode = DISABLE;
    
    ADC_Init(ADC1, &ADC);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);
    ADC_ITConfig(ADC1,  ADC_IT_EOC, ENABLE );
    ADC_Cmd(ADC1, ENABLE);
    
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1)) ;

    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1)) ;

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void ADC1_2_IRQHandler(void) {
    if(ADC_GetITStatus(ADC1, ADC_IT_EOC)!=RESET){
        value = ADC_GetConversionValue(ADC1);
  
        ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
    }
}


int main(void) {
    
    SystemInit();
    RCC_Configure();
    GPIO_Configure();
    ADC_Configure();
    NVIC_Configure();

    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(WHITE);
    
    LCD_ShowString(LCD_TEAM_NAME_X, LCD_TEAM_NAME_Y, "Tue_Team 08", BLACK, WHITE);
    LCD_ShowString(LCD_COORD_X_X - 17, LCD_COORD_X_Y, "X: ", BLACK, WHITE);
    LCD_ShowString(LCD_COORD_Y_X - 17, LCD_COORD_Y_Y, "Y: ", BLACK, WHITE);
    
    while (1) {
        Touch_GetXY(&cur_x, &cur_y, 1);
        Convert_Pos(cur_x, cur_y, &pixel_x, &pixel_y);

        LCD_DrawCircle(pixel_x, pixel_y, 3);
        LCD_ShowNum(LCD_COORD_X_X, LCD_COORD_X_Y, pixel_x, 4, BLACK, WHITE);
        LCD_ShowNum(LCD_COORD_Y_X, LCD_COORD_Y_Y, pixel_y, 4, BLACK, WHITE);
        LCD_ShowNum(LCD_LUX_VAL_X, LCD_LUX_VAL_Y, value, 4, BLUE, WHITE);


    }
    return 0;
}