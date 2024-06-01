/*
I2C project for lesson
по кнопке S1 происходит запись данных в I2C EEPROM
по кнопке S2 происходит изменение адреса в EEPROM циклически между 4-мя адресами
по кнопке s3 происходт чтение данных из выбранной ячейки EEPROM и запись данных в переменную 

*/

#include <stdio.h>
#include <stdlib.h>
#include "stm32f407xx.h"
#define  BTN_CHECK_TIME 100000000/10000
#define  TIME1 500000
#define BTN_PRESS_CNT 4 

#define COUNT_1MS 1000

void RCC_Init(void);


void GPIO_Init(void){
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
  
  //-------- GPIO for buttons -------------------
  GPIOE->PUPDR |= GPIO_PUPDR_PUPD10_0;
  GPIOE->PUPDR |= GPIO_PUPDR_PUPD11_0;
  GPIOE->PUPDR |= GPIO_PUPDR_PUPD12_0;
  
  //------ GPIO settings for LED1 LED2 LED3 --------
  GPIOE->MODER |=GPIO_MODER_MODE13_0;
  GPIOE->MODER |=GPIO_MODER_MODE14_0;
  GPIOE->MODER |=GPIO_MODER_MODE15_0;

}


void I2C_Init(void){
  
}

void TIM1_Init(void){  // for APB2 frequency test
  RCC->APB2ENR	|= RCC_APB2ENR_TIM1EN;	   // TIM1 bus clocking enable
  TIM1->PSC		= 83;						// TIM1 clock freq = 1 MHz
  TIM1->ARR		= 999;					  //  TIM1 work time = 1 ms
  TIM1->CR1		|= TIM_CR1_CEN;			  // TIM1 counter enabled
}




void TIM2_Init(void){ // for APB1 frequency test
  RCC->APB2ENR |= RCC_APB1ENR_TIM2EN;	// TIM2 bus clocking enable
}

int main(void) {

  uint32_t i = 0;
  uint16_t ms_counter = 0;
  
  uint16_t S1_cnt = 0;  // button S1 press couter
  uint16_t S2_cnt = 0;  // button S2 press couter
  uint16_t S3_cnt = 0;  // button S3 press couter
  
  uint16_t S1_state = 0;   // S1 state: 1 = pressed, 0 = released
  uint16_t S2_state = 0;   // S2 state: 1 = pressed, 0 = released
  uint16_t S3_state = 0;   // S3 state: 1 = pressed, 0 = released
  
  
  
  //SystemInit();
  RCC_Init();
  GPIO_Init();
  TIM1_Init();  

  
  //---- turn off leds ---------- 
  GPIOE->BSRR |= GPIO_BSRR_BS13;
  GPIOE->BSRR |= GPIO_BSRR_BS14;
  GPIOE->BSRR |= GPIO_BSRR_BS15;
  

//-------- GPIO settings for buttons S1 S2 S3 ------------



  
	while (1){
	  if ((TIM1->SR & TIM_SR_UIF) != 0){  // if TIM1 flag UIF = 1
		TIM1->SR &= ~(TIM_SR_UIF);  // clear flag UIF
        if (ms_counter < COUNT_1MS) ms_counter++;
        else{
			ms_counter = 0;
			if (S1_state) S1_state = 0;
			else S1_state = 1;
		}
	  }
	  
	  if(S1_state) GPIOE->BSRR |= GPIO_BSRR_BR13;  // turn on LED1
	  else GPIOE->BSRR |= GPIO_BSRR_BS13;		   // turn off LED1

	}

	//	if ((GPIOE->IDR & GPIO_IDR_ID10) == 0) {  // if S1 pressed
	//		if(S1_cnt < BTN_PRESS_CNT){  
	//			S1_cnt++;
	//			S1_state = 0;	// считаем кнопку S1 не нажатой
	//		}
	//		else S1_state = 1;	// считаем кнопку S1 нажатой
	//	}
	//	else{                   // if S1 released
	//		S1_state = 0;	// считаем кнопку S1 не нажатой
	//		S1_cnt = 0;
	//	}
	//	if(S1_state) GPIOE->BSRR |= GPIO_BSRR_BR13;  // turn on LED1
	//	else GPIOE->BSRR |= GPIO_BSRR_BS13;  // turn off LED1

	//	// аналогичто для кнопки S2
	//	if ((GPIOE->IDR & GPIO_IDR_ID11) == 0) {  // if S2 pressed
	//		if(S2_cnt < BTN_PRESS_CNT){
	//			S2_cnt++;
	//			S2_state = 0;
	//		}
	//		else S2_state = 1;
	//	}
	//	else{                   // if S2 released
	//		S2_state = 0;
	//		S2_cnt = 0;
	//	}
	//	if(S2_state) GPIOE->BSRR |= GPIO_BSRR_BR14;  // turn on LED2
	//	else GPIOE->BSRR |= GPIO_BSRR_BS14;  // turn off LED2
		

	//	if ((GPIOE->IDR & GPIO_IDR_ID12) == 0) {  // if S3 pressed
	//		if(S3_cnt < BTN_PRESS_CNT){
	//			S3_cnt++;
	//			S3_state = 0;
	//		}
	//		else S3_state = 1;
	//	}
	//	else{                   // if S3 released
	//		S3_state = 0;
	//		S3_cnt = 0;
	//	}
	//	if(S3_state) GPIOE->BSRR |= GPIO_BSRR_BR15;  // turn on LED3
	//	else GPIOE->BSRR |= GPIO_BSRR_BS15;  // turn off LED3

	//	for(i=0; i<BTN_CHECK_TIME; i++){}  // wait for 10 ms
		
	//}
  
}




/*************************** End of file ****************************/
