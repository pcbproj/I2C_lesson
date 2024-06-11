/*
I2C project for lesson
по кнопке S1 происходит запись массива данных из 8-х  байт в I2C EEPROM, начиная с выбранной ячейки памяти.
по кнопке S2 происходит изменение адреса в EEPROM циклически между 8-мью адресами. 
	-- Индикация смещения на LED1-LED3.
по кнопке S3 происходит чтение данных из выбранной ячейки EEPROM и запись данных в переменную, чтобы проверить в Debuger
по кнопке S4 происходит чтение массива данных (4 байта) из EEPROM, начиная с выбранной ячейки памати и сохранение в массив

*/


/*======== План написания ПО ==========
0. Программа в видео конечного флагового автомата. 
	Каждое состояние - это определенная операция. 
	Автомат крутится циклически. 
	При появлении нужного флага, автомат переходит в определенное состояние и выполняет определенные действия.

1. написать ф-ию записи в EEPROM. 
	проверить ее в железе через логический анализатор
2. написать ф-ию чтения из EEPROM. 
	проверить ее в железе через логический анализатор
3. написать ф-ию опроса кнопок и выставление флагов операции.

*/

#include <stdio.h>
#include <stdlib.h>
#include "stm32f407xx.h"

#define	BTN_CHECK_TIME 100000000/10000
#define	TIME1 500000
#define	BTN_PRESS_CNT 4 

#define COUNT_1MS 1000


// принцип формирования адреса устройства на I2C: 
#define I2C_DEV_ADDR	0xA0 // адрес микросхемы EEPROM = 1010_0000 в бинарном виде. Используются старшие 7 бит
#define I2C_WR_BIT		0x00 // запрос на запись данных в I2C-устройство (в EEPROM)
#define I2C_RD_BIT		0x01 // запрос на чтение данных из I2C-устройство (в EEPROM)
#define I2C_DEV_ADDR_RD	 (I2C_DEV_ADDR + I2C_RD_BIT)	// младший бит выставляем в RD = 1
#define I2C_DEV_ADDR_WR  (I2C_DEV_ADDR + I2C_WR_BIT)	// младший бит выставляем в WR = 0


#define EEPROM_WR_START_ADDR 0x08	// запись с 1 ячейки в страницу 2
#define EEPROM_PAGE_LEN_BYTES 8
#define EEPROM_RD_START_ADDR 0x08	// чтение с 1 ячейки в страницу 2

char i2c_tx_array[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
char i2c_rx_array[8] = {};



void RCC_Init(void);


void GPIO_Init(void){
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
  
  //-------- GPIO for buttons -------------------
  GPIOE -> PUPDR |= GPIO_PUPDR_PUPD10_0;
  GPIOE -> PUPDR |= GPIO_PUPDR_PUPD11_0;
  GPIOE -> PUPDR |= GPIO_PUPDR_PUPD12_0;
		   
  //-------- GPIO settings for LED1 LED2 LED3 --------
  GPIOE -> MODER |=GPIO_MODER_MODE13_0;
  GPIOE -> MODER |=GPIO_MODER_MODE14_0;
  GPIOE -> MODER |=GPIO_MODER_MODE15_0;

}


void I2C_Init(void){
	
  RCC -> APB1ENR |= RCC_APB1ENR_I2C1EN;	// включение тактирования модуля I2C1 
  RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN;	// включение тактирования порта GPIOB (PB8 = SCL, PB9 = SDA)
  
  // настройка выводов PB8 и PB9 для работы с модулем I2C1
  GPIOB -> MODER 	|= 	GPIO_MODER_MODE8_1;		// PB8 в режиме альтернативной функции
  GPIOB -> MODER 	|= 	GPIO_MODER_MODE9_1;		// PB9 в режиме альтернативной функции
  GPIOB -> OTYPER	|=	(GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);	// включение выводов PB8 PB9 в режим open-drain
		   
  //GPIOB -> PUPDR	&=	~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD9);	// явно прописываем отключение всех подтягивающих резисторов
  GPIOB -> PUPDR	|=	(GPIO_PUPDR_PUPD8_0 | GPIO_PUPDR_PUPD9_0);	// включаем подтягивающие pull_up резисторы
															// хотя по умолчанию они и так отключены
  
  GPIOB -> AFR[1]	|=	GPIO_AFRH_AFRH0_2;	// для PB8 выбрана альтернативная ф-ия AF4 = I2C1
  GPIOB -> AFR[1]	|=	GPIO_AFRH_AFRH1_2;	// для PB9 выбрана альтернативная ф-ия AF4 = I2C1
  
/*======== настройка модуля I2C1 ============

По умолчанию I2C работает в подчиненном режиме. Интерфейс автоматически переключается с подчиненного на
мастер, после того как он сгенерирует условие START и от ведущего к ведомому переключается, в случае 
арбитражного проигрыша или если на шине происходит генерация STOP-условия, что обеспечивает возможность 
работы с несколькими ведущими устройствами.

  режим работы					  	  = мастер
  скорость передачи				  	  = 100 кбит/сек
  адресация устройств на шине I2C 	  = 7 битная
  DMA не используется			  	  = Эти биты по умолчанию равны 0
  прерывания не используются	  	  = Эти биты по умолчанию равны 0
  адрес микросхемы памяти на шине I2C = 0xA0 = 0b1010_0000. Используются старшие 7 бит!

*/  
  // настройка частоты тактирования I2C1 = частота шины APB1 = 42 МГц
  
  I2C1 -> CR1	= 0x0000; // default value
  I2C1 -> CR2	|=	(42 << I2C_CR2_FREQ_Pos);  // CR2_FREQ = 42 т.к. Freq_APB1 = 42MHz
  //I2C1 -> CR2	|=	(I2C_CR2_FREQ_5 | I2C_CR2_FREQ_3 | I2C_CR2_FREQ_1);  // CR2_FREQ = 42

 
  

  /*====== CCR вычисления: ======
	I2C работает на частоте 100 кГц - Standard mode
	Thigh = CCR * T_plck1
	Tlow = CCR * T_pclk1
	Tsm = 1/(I2C_freq) = 1/100000 = Thigh + Tlow;
	1/100000 = 2 * CCR * T_pclk1
	CCR = 1 / (2*100000*T_pclk1)
	T_pclk1 = 1 / Freq_APB1; 
	Freq_APB1 = 42 MHz
	T_Pclk1 = 1 / 42000000
	CCR = 42000000 / (2*100000) = 210;
  */
  I2C1 -> CCR	|=	(210 << I2C_CCR_CCR_Pos);		// 100 КГц
  I2C1 -> CCR	&=	~(I2C_CCR_FS);					// явный сброс бита FS = работа на чатоте 100 кГц (Standard Mode)	
  
  I2C1 -> TRISE |=  (43 << I2C_TRISE_TRISE_Pos);	// значение поля I2C1->CR2_FREQ + 1 = 42+1 = 43
  I2C1 -> OAR2  &=  ~(I2C_OAR1_ADDMODE);			// использование 7-ми битного адреса устройства на шине I2C
  I2C1 -> CR1	|=	I2C_CR1_PE;						// I2C1 enabled. 
  I2C1 -> CR1	|=	I2C_CR1_ACK;					// разрешение генерации ACK (NACK) после приема байтов.
  /* бит I2C_CR1_ACK можно выставлять в 1 только после включения бита I2C_CR1_PE. 
	 иначе бит I2C_CR1_ACK всегда будет сбрасываться аппаратно.
  */

}

void I2C1_StartGen(void){
	I2C1->CR1  |=  I2C_CR1_START;
	while((I2C1 -> SR1 & I2C_SR1_SB) == 0){};	// дождаться START-условия на шине I2C
}


void I2C1_StopGen(void){
	I2C1->CR1  |=  I2C_CR1_STOP;
}


void I2C1_ACK_Gen_Enable(void){
	I2C1->CR1  |=  I2C_CR1_ACK;
}

void I2C1_ACK_Gen_Disable(void){
	I2C1->CR1  &=  ~(I2C_CR1_ACK);
}


void I2C1_Tx_DeviceADDR(char device_address, char RW_bit){
	I2C1 -> DR = (device_address + RW_bit);				// отправить в I2C_DR адрес устройства и бит WR
	while((I2C1 -> SR1 & I2C_SR1_ADDR) == 0){};	// ждем флаг I2C_SR1_ADDR = 1. Пока завершится передача байта адреса
	(void)I2C1 -> SR1; 
	(void)I2C1 -> SR2;	// очистка бита ADDR чтением регистров SR1 SR2

}

void I2C_Soft_EEPROM_Reset(void){
	I2C1_StartGen();	// START-условие
	I2C1->DR = 0xFF;	// 9 тактов SCL при SDA = 1 
	I2C1_StartGen();	// STAR-условие
	I2C1_StopGen();		// STOP - условие
}


void LED_ON(char led_number){
	switch (led_number){
	case NONE_LEDS:		// All leds turned OFF
		GPIOE -> BSRR |= GPIO_BSRR_BS13;  
		GPIOE -> BSRR |= GPIO_BSRR_BS14;
		GPIOE -> BSRR |= GPIO_BSRR_BS15;
		break;

	case 1: // LED1 включен, остальные выключены
		GPIOE -> BSRR |= GPIO_BSRR_BR13;
		GPIOE -> BSRR |= GPIO_BSRR_BS14;
		GPIOE -> BSRR |= GPIO_BSRR_BS15;
		break;
	
	case 2: // LED2 включен, остальные выключены
		GPIOE -> BSRR |= GPIO_BSRR_BS13;
		GPIOE -> BSRR |= GPIO_BSRR_BR14;
		GPIOE -> BSRR |= GPIO_BSRR_BS15;
		break;

	case 3:	// LED3 включен, остальные выключены
		GPIOE -> BSRR |= GPIO_BSRR_BS13;
		GPIOE -> BSRR |= GPIO_BSRR_BS14;
		GPIOE -> BSRR |= GPIO_BSRR_BR15;
		break;
	
	case ALL_LEDS: // Все светодиоды включены
		GPIOE -> BSRR |= GPIO_BSRR_BR13;
		GPIOE -> BSRR |= GPIO_BSRR_BR14;
		GPIOE -> BSRR |= GPIO_BSRR_BR15;
		break;
	}
	
	
}


/* в случае EEPROM AT24C02B можно за раз записать не более 8 байт данных. 
т.к. размер страницы всего 8 байт. И если запись доходит до конца страницы,
то следующий байт пишется в первый адрес текущей страницы. 
Таким образом, данные там могут быть повреждены / перезаписаны.
*/
void I2C_Write(char start_addr, char data[], uint16_t data_len){ // запись в EEPROM указанного массива, указанной длинны, с указанного адреса  
	
	I2C1_ACK_Gen_Enable();						// включение генерации ACK
	
	while((I2C1 -> SR2 & I2C_SR2_BUSY) != 0){};	// проверить занятость шины I2C по флагу I2C_SR2_BUSY
	
	I2C1_StartGen();							// генерация START-условия
	
	I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_WR_BIT);
	
	I2C1 -> DR = start_addr;	// отправить в I2C_DR адрес начальной ячейки памяти, куда хотим писать данные
	while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных

  // цикл сколько байт нужно передать: 
	// отправляем в I2C_DR байты данных, 
	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных
	for(uint16_t i = 0; i < data_len; i++){
		I2C1 -> DR = data[i];
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};
	}

	I2C1_StopGen();	// генерация STOP-условия
}

void I2C_Read(char start_addr, char rd_data[], uint16_t data_len){  // чтение из EEPROM из указанной ячейки, указанной длины массив, сохранение в указагнный масссив
	
	while((I2C1 -> SR2 & I2C_SR2_BUSY) != 0){};	// проверить занятость шины I2C по флагу I2C_SR2_BUSY
	
	I2C1_ACK_Gen_Enable();						// включение генерации ACK
	
	I2C1_StartGen();							// генерация START-условия
  
	I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_WR_BIT);	// передача адреса устройства и бита WR

   /*
   передача адреса ячейки памяти с которой хотим читать
   так как у нас память всего 256 байт, то адрес ячейки памяти имеет разрядность 8-бит = 1 байт
   если объем памяти будет больше, то для передачи адреса ячейки памяти нужно будет использовать 2 байта 
	*/

	I2C1 -> DR = start_addr;					// отправить в I2C_DR адрес начальной ячейки памяти, откуда хотим читать данные
	while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных

	
	/*========= Пример отправки 2-х байтного адреса ячейки памяти ===============

		I2C1 -> DR = (start_addr >> 8 );			// передача старшего байта адреса			
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ожидание отправки байта
		I2C1 -> DR = (start_addr);					// отправка младшего байта адреса	
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ожидание отправки байта
	
	*/

	
	I2C1_StartGen();	// повторная генерация START-условия

	I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_RD_BIT);		// передача адреса устройства и бита RD


  // цикл чтения данных (кол-во байт - 1):
	  // ожидание флаг I2C_SR1_RXNE = 1. - принят новый байт данных
	  // чтение регистра I2C_DR
	for(uint16_t i = 0; i < data_len-1; i++){
		while((I2C1 -> SR1 & I2C_SR1_RXNE) == 0){};
		rd_data[i] = I2C1 -> DR;
		
	}
	// отключение генерации ACK-бита после принятого байта, чтобы в конце отправить NACK
	I2C1_ACK_Gen_Disable();
	
	while((I2C1 -> SR1 & I2C_SR1_RXNE) == 0){};	// ожидание принятия последнего байта
	rd_data[data_len] = I2C1 -> DR;				// чтение регистра I2C_DR - чтение последнего принятого байта

	I2C1_StopGen();	// генерация STOP-условия	

 }


void TIM1_Init(void){  // for APB2 frequency test
  RCC -> APB2ENR	|= RCC_APB2ENR_TIM1EN;	   // TIM1 bus clocking enable
  TIM1 -> PSC		= 83;						// TIM1 clock freq = 1 MHz
  TIM1 -> ARR		= 999;					  //  TIM1 work time = 1 ms
  TIM1 -> CR1		|= TIM_CR1_CEN;			  // TIM1 counter enabled
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
  
  
  
  RCC_Init();

  GPIO_Init();
  
  TIM1_Init();
  
  I2C_Init();

  
  //---- turn off leds ---------- 
  GPIOE -> BSRR |= GPIO_BSRR_BS13;
  GPIOE -> BSRR |= GPIO_BSRR_BS14;
  GPIOE -> BSRR |= GPIO_BSRR_BS15;
  


  I2C_Write(EEPROM_WR_START_ADDR, i2c_tx_array, 4);	// запишем 4 байта в EEPROM c адреса EEPROM_WR_START_ADDR


//-------- GPIO settings for buttons S1 S2 S3 ------------


	//I2C_SoftReset();	// предварительный сброс I2C устройств

  
	while (1){
	  if ((TIM1 -> SR & TIM_SR_UIF) != 0){  // if TIM1 flag UIF = 1
		TIM1 -> SR &= ~(TIM_SR_UIF);  // clear flag UIF
        if (ms_counter < COUNT_1MS) ms_counter++;
        else{
			ms_counter = 0;
			if (S1_state) S1_state = 0;
			else S1_state = 1;

			I2C_Read(EEPROM_RD_START_ADDR, i2c_rx_array, 4);
		}
	  }
	  
	  if(S1_state) GPIOE -> BSRR |= GPIO_BSRR_BR13;  // turn on LED1
	  else GPIOE -> BSRR |= GPIO_BSRR_BS13;		   // turn off LED1
		
		

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
