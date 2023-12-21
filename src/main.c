/******************************************************************************
* Christmas Ball - Franzininho
* 
* This code is part of Christmas Ball project for 2023 Christmas Projects by
* Franziinho Community.
*
* Microcontroller: CH32V003F4P6
* Framework: None SDK for CH32
* IDE: VSCode + PlatformIO
* 
* This code make animations on 30 LEDs, read a button and play a buzzer.
*
* This example code Creative Commons Attribution 4.0 International License.
* When using the code, you must keep the above copyright notice,
* this list of conditions and the following disclaimer in the source code.
* (http://creativecommons.org/licenses/by/4.0/)

* Author: Fábio Souza
* This code is for fun and learning purposes.
* No warranty of any kind is provided.
*******************************************************************************/

// Includes
#include <ch32v00x.h>
#include <debug.h>

//LEDs
#define LEDS_PORT GPIOC
#define LED1_PIN GPIO_Pin_0
#define LED2_PIN GPIO_Pin_1
#define LED3_PIN GPIO_Pin_2
#define LED4_PIN GPIO_Pin_3
#define LED5_PIN GPIO_Pin_4
#define LED6_PIN GPIO_Pin_5
#define LED7_PIN GPIO_Pin_6
#define LED8_PIN GPIO_Pin_7
#define ALL_LEDS GPIO_Pin_All
#define LEDS_CLOCK_ENABLE RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE)

//Transistors
#define T_PORT GPIOD
#define T1_PIN GPIO_Pin_3
#define T2_PIN GPIO_Pin_4
#define T3_PIN GPIO_Pin_5
#define T4_PIN GPIO_Pin_6
#define ALL_T (T1_PIN | T2_PIN | T3_PIN | T4_PIN)
#define T_CLOCK_ENABLE RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE)

//Button
#define BUTTON_PORT GPIOD
#define BUTTON_PIN GPIO_Pin_0
#define BUTTON_CLOCK_ENABLE RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE)


//Functions prototypes
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void Delay_Init(void);
void Delay_Ms(uint32_t n);

//Global variables
uint32_t leds = 0x01;
uint64_t time = 0;


// Function to invert the bits of a byte
unsigned char invertBits(unsigned char byte) 
{
    unsigned char result = 0;

    for (int i = 0; i < 8; i++) 				// Loop through all bits
	{			
        if (byte & (1 << i)) 					// If the i'th bit is set
		{					
            result |= 1 << (7 - i);				// Set the 7 - i'th bit
        }
    }
    return result;
}

// Function to write the leds
void leds_write(uint32_t leds)
{
	static uint8_t group = 0;

	switch (group)
	{	
		case 0:										//LEDS GROUP1
		group++;									//increment group
		GPIO_ResetBits(T_PORT, T4_PIN);				//turn off T4	
		GPIO_SetBits(T_PORT, T1_PIN);				//turn on T1
		GPIO_Write(LEDS_PORT, leds & 0x000000FF); 	//LED GROUP1 Write
		break;

		case 1:										//LEDS GROUP2							
		group++;									//increment group
		GPIO_ResetBits(T_PORT, T1_PIN);				//turn off T1
		GPIO_SetBits(T_PORT, T2_PIN);				//turn on T2
		GPIO_Write(LEDS_PORT, leds>>8);				//LED GROUP2 Write
		break;

		case 2:											//LEDS GROUP3
		group++;										//increment group
		GPIO_ResetBits(T_PORT, T2_PIN);					//turn off T2
		GPIO_SetBits(T_PORT, T3_PIN);				  	//turn on T3			
		GPIO_Write(LEDS_PORT, invertBits(leds>>15)); 	//LED GROUP3 Write - invertBits because 
													 	//the leds are in the opposite direction
														//in the PCB
		break;

		case 3:											//LEDS GROUP4
		GPIO_ResetBits(T_PORT, T3_PIN);					//turn off T3
		GPIO_SetBits(T_PORT, T4_PIN);					//turn on T4
		GPIO_Write(LEDS_PORT, invertBits(leds>>23));	//LED GROUP4 Write - invertBits because 
														//the leds are in the opposite direction
														//in the PCB
		group = 0;										//reset group
		break;
	}
}

void animation_1(void)
{
	leds <<= 1;
	if (leds == 0x80000000)
	{
		leds = 0x00000001;
	}
}

void animation_2(void)
{
	leds >>= 1;
	if (leds == 0x00000000)
	{
		leds = 0x80000000;
	}
}

void animation_3(void)
{
	static uint16_t invert = 0;

	if (invert == 0)
	{
		invert = 1;
		leds = 0xAAAAAAAA;
	}
	else
	{
		invert = 0;
		leds = 0x55555555;
	}

}

void animation_4(void)
{
	static uint16_t invert = 0;

	if (invert == 0)
	{
		invert = 1;
		leds = 0x00000000;
	}
	else
	{
		invert = 0;
		leds = 0xFFFFFFFF;
	}
}

void animation_5(void)
{
	static uint8_t RGB = 0;

	switch (RGB)
	{
		case 0:
		leds = 0b01001001001001001001001001001001;
		RGB++;
		break;

		case 1:
		leds = 0b10010010010010010010010010010010;
		RGB++;
		break;

		case 2:
		leds = 0b00100100100100100100100100100100;
		RGB = 0;
		break;
	}
}

void animation_6(void)
{
	static uint8_t direction = 0;
	
	if(direction == 0)
	{
		leds <<= 1;
		if (leds == 0x80000000)
		{
			direction = 1;
		}
	}
	else
	{
		leds >>= 1;
		if (leds == 0x000000001)
		{
			direction = 0;
		}
	}
}


// ISR for TIM2
__attribute__((interrupt("WCH-Interrupt-fast")))			// ISR in RAM
void TIM2_IRQHandler(void){									// ISR for TIM2
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {	// if TIM2 update interrupt flag is set
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			// clear TIM2 update interrupt flag

		leds_write(leds);									//update leds
    }
}

int main(void)
{
	// Init System
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	Delay_Init();

	// Init LEDS
	LEDS_CLOCK_ENABLE;
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = ALL_LEDS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LEDS_PORT, &GPIO_InitStructure);

	//init transistors
	T_CLOCK_ENABLE;
	GPIO_InitStructure.GPIO_Pin = ALL_T;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(T_PORT, &GPIO_InitStructure);

	/* 
	//init button
	BUTTON_CLOCK_ENABLE;
	GPIO_InitStructure.GPIO_Pin = BUTTON_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BUTTON_PORT, &GPIO_InitStructure);

	*/

	//turn off all transistors
	GPIO_ResetBits(T_PORT, ALL_T);


	//init timer
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitStructure.TIM_Period = 830;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 48-1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    NVIC_SetPriority(TIM2_IRQn, 0x80);
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM_Cmd(TIM2,ENABLE);
	
	uint8_t animation = 1;		//animation number

	while (1)
	{

		// // //read button with debounce
		// uint8_t button_state = 0;
		// if(GPIO_ReadInputDataBit(BUTTON_PORT, BUTTON_PIN) == 0)
		// {
		// 	button_state++;
		// }
		// else
		// {
		// 	button_state = 0;
		// }

		// //if button pressed
		// if (button_state > 5)
		// {
			
		// }

		//delay 20 ms
		Delay_Ms(1);
		time++;

		switch (animation)
			{
				case 1:
				if(time % 100 == 0) animation_1();
				break;

				case 2:
				if(time % 100 == 0) animation_2();
				break;

				case 3:
				if(time % 100 == 0)animation_3();
				break;

				case 4:
				if(time % 100 == 0 ) animation_4();
				break;

				case 5:
				if(time % 1000 == 0) animation_5();
				break;
				
				case 6:
				if(time % 10 == 0) animation_6();
				break;

				default:
				animation = 1;
				break;


			}


		if(time % 60000 == 0)
		{
			animation ++;
		}
	}
}

void NMI_Handler(void) {}
void HardFault_Handler(void)
{
	while (1)
	{
	}
}