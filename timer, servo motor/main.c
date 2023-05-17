#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "lcd.h"
#include "touch.h"

#define LCD_TEAM_NAME_X 40
#define LCD_TEAM_NAME_Y 50

#define LCD_ON_X 40
#define LCD_ON_Y 70

#define LCD_REC_X 50
#define LCD_REC_Y 100

#define LCD_REC_LEN 50

#define LCD_BUT_X 62
#define LCD_BUT_Y 116

#define LCD_COUNT_X 40
#define LCD_COUNT_Y 240

#define LCD_LED1_X 40
#define LCD_LED1_Y 260
#define LCD_LED2_X 150
#define LCD_LED2_Y 260

void RCC_Configure(void);
void GPIO_Configure(void);
void TIM2_Configure(void);
void NVIC_Configure(void);
void TIM2_IRQHandler(void);
void LED_on(void);
void Delay(void);



// number of IRQ calls
u32 IRQ_counter = 0;

// Button flag
int btn=0;

// LED1/ LED2 flag
int LED1=0, LED2=0;

// location of coordinate values
uint16_t cur_x,cur_y,pixel_x,pixel_y;
//------- --------------------------------------------------------------------------------------------

/*
   Enable TIM2 to use timer
   Enable LED to use Port D
   Enable AF IO Clock
*/
void RCC_Configure(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
    
    //LED1,2(PD2,3) , PWM(PB0)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

/*
Since it is used as the output of LED in Port D 2,3, 
it is set as GPIO_PIN, the maximum output speed is 10MHz, 
and the general purpose output push-pull mode is set.

Finally, set GPIO Pin to control PWM.
*/

void GPIO_Configure(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
      
   // LED
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
   GPIO_Init(GPIOD, &GPIO_InitStructure);
   
   // PWM
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
         
}


void TIM2_Configure(void){
    TIM_TimeBaseInitTypeDef Timer;

    // input clock is used as it is.
    Timer.TIM_ClockDivision = TIM_CKD_DIV1;
    
    // Set up counting mode in which count is incremented by 1 for every 1clk.
    Timer.TIM_CounterMode = TIM_CounterMode_Up;
    
    // Since the period count starts from 0, set it to a value obtained by subtracting 1 from 7200.
    Timer.TIM_Prescaler = 7200 - 1;
    
    // Since the count of the prescaler starts from 0, set it to a value minus 1 from 10000.
    //==1s
    Timer.TIM_Period = 10000 - 1;
    
    TIM_TimeBaseInit(TIM2, &Timer);
    
    // Enable interrupt of TIM2.
    TIM_ARRPreloadConfig(TIM2,ENABLE);
    TIM_Cmd(TIM2,ENABLE);
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//interrupt enable
}

void TIM3_Configure(void){

    // declare the variable about TimeBase
    TIM_TimeBaseInitTypeDef Timer;

    uint16_t prescale= (uint16_t) (SystemCoreClock/ 1000000)-1;
    
    Timer.TIM_Period= 20000-1;
    Timer.TIM_Prescaler= prescale;
    Timer.TIM_ClockDivision= 0;
    Timer.TIM_CounterMode= TIM_CounterMode_Down;
    TIM_TimeBaseInit(TIM3, &Timer);
    
    
    // declare the variable about OCinit
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode= TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity= TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState= TIM_OutputState_Enable;
    
    TIM_OCInitStructure.TIM_Pulse= 1500; // us 1.5ms
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
    
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/*
NVIC_PriorityGroup_0 is set because no other pre-emption priority group is needed. 
Also, pre-emption priority and subpriority were given the same priority as 0.
*/
void NVIC_Configure(void) {
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    // Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'

    // UART1
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}



void TIM2_IRQHandler(void) {
  
  /*
  When the button on the LCD is pressed, the LED toggle becomes ON, 
  and the number of times TIM2 IRQ handler is called increases and is stored in IRQ_counter.
  */
   if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        if(btn==1){
            IRQ_counter++;
            LED1=!LED1;
            
            //Whenever TIM2 IRQ handler is called 5 times, the flag is switched to the opposite state of current LED2.
            if(IRQ_counter%5==0){
                LED2=!LED2;
            }
        }
        // Based on the current LED1,2 flag, the function to turn on and off LED1,2 is called.
        LED_on();
        TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
    }
}

void LED_on(void){
  
    /*
    If the flag of LED1 is 1, the LED of Port D2 is turned on, 
    and the character string ??ON?? is output in red letters on a white background at (90, 260). 
    In the opposite case, the LED is turned off and the string ??OFF?? is output in the same way.
    */
    if(LED1==1){
        GPIO_SetBits(GPIOD,GPIO_Pin_2);
    }
    else{
        GPIO_ResetBits(GPIOD,GPIO_Pin_2);
    }
    if(LED2==1){
        GPIO_SetBits(GPIOD,GPIO_Pin_3);
    }
    else{
        GPIO_ResetBits(GPIOD,GPIO_Pin_3);
    }
}

// A function that changes the duty cycle
void change_pwm_duty_cycle(int percentx10){
  int pwm_pulse;
  pwm_pulse = percentx10 * 20000 / 100;
  
  // declare the variable about OCinit
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  TIM_OCInitStructure.TIM_OCMode= TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OCPolarity= TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState= TIM_OutputState_Enable;
  
  TIM_OCInitStructure.TIM_Pulse = pwm_pulse;
  TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}

void Delay(void) {
    int i;

   for (i = 0; i < 2000000; i++);
}

int main(void) {

    SystemInit();

    RCC_Configure();
    GPIO_Configure();
    TIM2_Configure();
    TIM3_Configure();
    NVIC_Configure();

    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(WHITE);
    
    LCD_ShowString(LCD_TEAM_NAME_X, LCD_TEAM_NAME_Y, "THU_TEAM08", BLUE, WHITE);
    LCD_ShowString(LCD_ON_X, LCD_ON_Y, "OFF", RED, WHITE);
    LCD_ShowString(LCD_BUT_X, LCD_BUT_Y, "BTN", RED, WHITE);
    
    LCD_DrawRectangle(LCD_REC_X, LCD_REC_Y, LCD_REC_X+LCD_REC_LEN,LCD_REC_Y+LCD_REC_LEN);
    
    while (1) {
        // While waiting for touch to be recognized in the while loop, if it is recognized, 
        // location is stored in cur_x and cur_y.
        // It controls the servo motor by changing the duty cycle of PWM.
        change_pwm_duty_cycle(3);
        Delay();
        change_pwm_duty_cycle(12);
        Touch_GetXY(&cur_x, &cur_y, 1);
        
        // locations where the touch is recognized, cur_x and cur_y, are stored as pixel_x and pixel_y
        Convert_Pos(cur_x, cur_y, &pixel_x, &pixel_y);
        
        // When a touch is recognized within the position of the rectangle representing the button, 
        // the flag is switched to the opposite state of the current button.
        if(pixel_x>LCD_REC_X&&pixel_x<LCD_REC_X+LCD_REC_LEN){
            if(pixel_y>LCD_REC_Y&&pixel_y<LCD_REC_Y+LCD_REC_LEN){
                btn=!btn;
            }
        }
        if(btn){
            LCD_ShowString(LCD_ON_X, LCD_ON_Y, "ON  ", RED, WHITE);
        }
        else{
           LCD_ShowString(LCD_ON_X, LCD_ON_Y, "OFF", RED, WHITE);
        }        
        Delay();
        

    
    }
    return 0;
}