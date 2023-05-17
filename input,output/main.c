#include "stm32f10x.h"

const int S1 = 0x800;   //S1 Button
const int S2 = 0x1000;  //S2 Button

const int LED = 0x80;   // PD7 LED
const int PC8 = 0x100;  //Give signal to relay module

/*
 RCC BASE --> 0x4002 1000
 RCC_APB2ENR -> APB2 peripheral clock enable register address --> 0x18
 RCC_APB2ENR FINAL ADDRESS --> 0x4002 1000 + 0x18 = 0x40021018
*/
#define RCC_apb2 (*(volatile unsigned *)0x40021018)

/*
 PC8 is connected to PORT C and PORT C is connected to APB2 BUS.
 PORT C BASE --> 0x4001 1000
 
 Since it uses pins 8 of PORT C, it must be set via GPIOx_CRH.

 GPIOx_CRL address offset --> 0x04
 PORT C configuration --> 0x4001 1000 + 0x04 = 0x4001 1004
 
 Output the OUTPUT value to the PC8 through the port output data register.
 GPIOx_ODR address offset --> 0x0c

 PORT C output data register(GPIOx_ODR) --> 0x4001 1000 + 0x0c = 0x4001 100c

*/
#define PORT_C_CRH_config (*(volatile unsigned *)0x40011004)
#define PORT_C_CRH_set 0x44444445;

#define PORT_C_ODR (*(volatile unsigned *)0x4001100c)

/*
 Button S1, S2 and LED are connected to PORT D and PORT D is connected to APB2 BUS.
 PORT D BASE --> 0x4001 1400

 Since we are using pins 7, 11, 12 of PORT D, 
 we need to set the pins through GPIOx_CRL, and GPIOx_CRH.

 GPIOx_CRL address offset --> 0x00
 PORT D CRL configuration --> 0x4001 1400 + 0x00 = 0x4001 1400
 
 GPIOx_CRH address offset --> 0x04
 PORT D CRH configuration --> 0x4001 1400 + 0x04 = 0x4001 1404
 
 Get INPUT value by Button S1, S2 through the port input data register.
 GPIOx_IDR address offset --> 0x08

 PORT D input data register(GPIOx_IDR) --> 0x4001 1400 + 0x08 = 0x4001 1408

 Output the OUTPUT value to the LED through the port output data register.
 GPIOx_ODR address offset --> 0x0c

 PORT D output data register(GPIOx_ODR) --> 0x4001 1400 + 0x0c = 0x4001 140c
 
*/
#define PORT_D_CRL_config (*(volatile unsigned *)0x40011400)
#define PORT_D_CRL_set 0x50000000

#define PORT_D_CRH_config (*(volatile unsigned *)0x40011404)
#define PORT_D_CRH_set 0x444cc444

#define PORT_D_ODR (*(volatile unsigned *)0x4001140c)

#define PORT_D_IDR (*(volatile unsigned *)0x40011408)   //button input



void delay(){           //delay function
  int i;
  for (i = 0; i < 10000000; i++){}
}


int main(void)
{
  // Apply the clock to the GPIO you want to use using RCC (reset and clock control).
  RCC_apb2 |= 0x30;
  
  //pin setting
  PORT_C_CRH_config ^= PORT_C_CRH_set;

  PORT_D_CRH_config ^= PORT_D_CRH_set;
  
  PORT_D_CRL_config ^= PORT_D_CRL_set;

  while(1){
    if(~PORT_D_IDR&S1){
      PORT_C_ODR = PC8;
      delay();
    }
    else if(~PORT_D_IDR&S2){
      PORT_D_ODR = LED;
      delay();
    }
    else{
      PORT_D_ODR = !LED;
      PORT_C_ODR = !PC8;
    }
  }
  return 0;
}