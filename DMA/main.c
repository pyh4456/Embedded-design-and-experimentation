#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"

//include lcd, touch header files
#include "lcd.h"
#include "touch.h"

#define LCD_TEAM_NAME_X 40
#define LCD_TEAM_NAME_Y 50

void RCC_Configure(void);
void GPIO_Configure(void);
void ADC_Configure(void);
void DMA_Configure(void);
void Delay(void);

// Declare variable to receive ADC value input from illuminance sensor
volatile uint32_t ADC_Value = 0;
//---------------------------------------------------------------------------------------------------

// Clock setting part
void RCC_Configure(void)
{
    // DMA
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // ADC1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

}

// Set the pin number, mode, and speed.
void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_Struct;

    GPIO_Struct.GPIO_Pin = GPIO_Pin_0;
    GPIO_Struct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_Struct);
}

/*
Set the mode of the ADC to be connected to the illuminance sensor, whether it is used for external triggers, and the speed of the cycle.
Since one illuminance sensor uses ADC12_IN10, it is set to 10 channels.
In addition, since DMA is used, the ADC_DMACmd() function is used,
After setting Calibration to Reset, the operation was configured to start Calibration.
*/
void ADC_Configure(void) {
    ADC_InitTypeDef ADC;

    ADC.ADC_Mode = ADC_Mode_Independent;
    ADC.ADC_ContinuousConvMode = ENABLE;
    ADC.ADC_DataAlign = ADC_DataAlign_Right;
    ADC.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC.ADC_NbrOfChannel = 1;           // Use one channel.
    ADC.ADC_ScanConvMode = DISABLE;

    // Enable ADC1 
    ADC_Cmd(ADC1, ENABLE);
    
    // ADC1 regualr Channel Configuration
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);
    ADC_Init(ADC1, &ADC);    

    //Enable ADC1 DMA           
    ADC_DMACmd(ADC1, ENABLE);

    // Enable ADC1 reset calibration register
    ADC_ResetCalibration(ADC1);
    
    // Check the end of ADC1 reset calibratoin register
    while(ADC_GetResetCalibrationStatus(ADC1)) ;

    ADC_StartCalibration(ADC1);                 // Start ADC1 calibration
    while(ADC_GetCalibrationStatus(ADC1)) ;     // Check the end of ADC1 calibation

    // Start ADC1 Software Conversion
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}


void DMA_Configure(void) {
    DMA_InitTypeDef DMA_InitType;

    DMA_DeInit(DMA1_Channel1);
    DMA_StructInit(&DMA_InitType);

    // Determines the memory address from which register to receive information
    DMA_InitType.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;

    // memory base address of the variable
    DMA_InitType.DMA_MemoryBaseAddr = (uint32_t)&ADC_Value;

    // amount of memory to store in the variable
    // It is set to 1 because data of one channel needs to be saved.
    DMA_InitType.DMA_BufferSize = 1;

    // Increases the memory address and sets whether to write information to the next memory.
    // It is set to disable because it is not used.
    DMA_InitType.DMA_MemoryInc = DMA_MemoryInc_Disable;

    // Set the bit unit of the sent peripheral.
    // It can be set to 8/16/32 bits.
    DMA_InitType.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;

    // The amount of data to be stored in the variable
    DMA_InitType.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;

    // Circular mode is used for circular buffers or continuous data processing.
    // The number of data to be transmitted is automatically reloaded in the channel setting stage, 
    // and DMA requests are continuously generated.
    DMA_InitType.DMA_Mode = DMA_Mode_Circular;

    // Determines the priority of the DMA channel.
    DMA_InitType.DMA_Priority = DMA_Priority_High;

    DMA_Init(DMA1_Channel1, &DMA_InitType);

    // Enable DMA1 Channel1
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

void Delay(void) {
  int i;

  for (i = 0; i < 1000000; i++);
}

/*
Declare a flag to change the background of the TFT LCD, 
and the team name ‘THU_Team08’ in the TFT LCD (90, 50) coordinates,
The (90, 100) coordinates indicate the illuminance value in white text on a gray background.

Normally, an illuminance value of about 3600 is measured.
However, when the flash is lit, it increases to 4095,
and it can be seen that the maximum is 4096.

This can be considered as not exceeding the maximum range because 4096 = 4M.

Therefore, the threshold value is set to 4050, 
and when the illuminance value reaches the corresponding point, 
the LCD background is changed to gray and the flag variable value is changed to 0.
and team name and illuminance value are printed on a gray background with white text at each designated coordinate

Conversely, when the illuminance sensor detects less than the threshold, the LCD background is changed to white, 
and the value of the flag variable is changed to 1.

Then, the team name and illuminance value are printed in gray text on a white background at each designated location, 
and the LED is turned off.
*/



int main(void) {
    
    SystemInit();
    RCC_Configure();
    GPIO_Configure();
    ADC_Configure();
    DMA_Configure();


    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(GRAY);
    
    int flag = 0;
    LCD_ShowString(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y, "THU_Team08", WHITE, GRAY);
    LCD_ShowNum(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y+50, ADC_Value, 4, WHITE, GRAY);


    while (1) {
       // The value of the analog data read by the illuminance sensor, that is, when the illuminance value is 4050 or higher,
       // Flag variable value change.
       if(ADC_Value >= 4050){
         if(flag == 1){
           // When the illuminance value is 4050 or higher, the background color of TFT_LCD changes.
           LCD_Clear(GRAY);
           flag = 0;
         }
         // Since the TFT-LCD has a gray background color, Text is output in white.
         LCD_ShowString(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y, "THU_Team08", WHITE, GRAY);
         LCD_ShowNum(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y+50, ADC_Value, 4, WHITE, GRAY);
       }
       // The value of the analog data read by the illuminance sensor, that is, when the illuminance value is less than 4050,
       // Flag variable value change.
       else{
         if(flag == 0){
           // If the illuminance value is less than 4050, keep the TFT_LCD's background color white.
           LCD_Clear(WHITE);
           flag = 1;
         }
         // Since the background color of the TFT-LCD is white, the letters are output in gray.
         LCD_ShowString(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y, "THU_Team08", GRAY, WHITE);
         LCD_ShowNum(LCD_TEAM_NAME_X+50, LCD_TEAM_NAME_Y+50, ADC_Value, 4, GRAY, WHITE);       
       }

       // Delay
       Delay();
    }
    return 0;
}
