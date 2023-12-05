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

//buzzer
#define BUZZER_PORT GPIOD
#define BUZZER_PIN GPIO_Pin_2
#define BUZZER_CLOCK_ENABLE RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE)

//Functions prototypes
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void Delay_Init(void);
void Delay_Ms(uint32_t n);

//Global variables
uint32_t leds = 0x01;
uint64_t time = 0;

// Frequências para notas musicais (em Hz)
#define NOTE_C4 168
#define NOTE_D4 150
#define NOTE_E4 133
#define NOTE_F4 126
#define NOTE_G4 112
#define NOTE_A4 100
#define NOTE_B4 89
#define NOTE_C5 84
#define NOTE_D5 75
#define NOTE_E5 67
#define NOTE_F5 63
#define NOTE_G5 56
#define NOTE_REST 0

// Durações das notas (em milissegundos)
#define DURATION_QUARTER 250
#define DURATION_EIGHTH 125
#define DURATION_HALF 500
#define DURATION_WHOLE 1000

/* PWM Output Mode Definition */
#define PWM_MODE1   0
#define PWM_MODE2   1

/* PWM Output Mode Selection */
//#define PWM_MODE PWM_MODE1
#define PWM_MODE PWM_MODE1

// Matriz com as notas musicais e suas durações
const int melodyDuration[][2] = {
  {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_HALF},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_HALF},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_G4, DURATION_QUARTER}, {NOTE_C4, DURATION_QUARTER}, {NOTE_D4, DURATION_QUARTER},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER},
  {NOTE_F4, DURATION_HALF}, {NOTE_F4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER},
  {NOTE_E4, DURATION_HALF}, {NOTE_D4, DURATION_QUARTER}, {NOTE_D4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER},
  {NOTE_D4, DURATION_HALF}, {NOTE_G4, DURATION_HALF},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_HALF},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_HALF},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_G4, DURATION_QUARTER}, {NOTE_C4, DURATION_QUARTER}, {NOTE_D4, DURATION_QUARTER},
  {NOTE_E4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER}, {NOTE_F4, DURATION_QUARTER},
  {NOTE_F4, DURATION_HALF}, {NOTE_F4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER},
  {NOTE_E4, DURATION_HALF}, {NOTE_D4, DURATION_QUARTER}, {NOTE_D4, DURATION_QUARTER}, {NOTE_E4, DURATION_QUARTER},
  {NOTE_D4, DURATION_HALF}, {NOTE_G4, DURATION_HALF}
};


// Função para inicializar o PWM
void TIM1_PWMOut_Init(u16 arr, u16 psc, u16 ccp)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    TIM_OCInitTypeDef TIM_OCInitStructure={0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure={0};

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOD, ENABLE );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_TIM1, ENABLE );
    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit( TIM1, &TIM_TimeBaseInitStructure);

#if (PWM_MODE == PWM_MODE1)
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

#elif (PWM_MODE == PWM_MODE2)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;

#endif

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = ccp;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init( TIM1, &TIM_OCInitStructure );

    TIM_CtrlPWMOutputs(TIM1, ENABLE );
    TIM_OC1PreloadConfig( TIM1, TIM_OCPreload_Disable );
    TIM_ARRPreloadConfig( TIM1, ENABLE );
    TIM_Cmd( TIM1, ENABLE );
}

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

	//init buzzer
	BUZZER_CLOCK_ENABLE;
	GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);

	*/

	//turn off all transistors
	GPIO_ResetBits(T_PORT, ALL_T);


	//init timer
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Period = 5000;
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
	uint8_t x = 0;				//melody note
	uint8_t x_ant = -1;			//melody note ant
	uint8_t play_melody = 0;	//play melody

	while (1)
	{

		// //read button with debounce
		// uint8_t button_state = 0;
		// for (uint8_t i = 0; i < 10; i++)
		// {
		// 	button_state += GPIO_ReadInputDataBit(BUTTON_PORT, BUTTON_PIN);
		// 	Delay_Ms(1);
		// }

		// //if button pressed
		// if (button_state > 5)
		// {
			
		// }

		//delay 20 ms
		Delay_Ms(1);
		time++;

		//play melody
		if(play_melody)
		{
			if(x != x_ant)
			{
			x_ant = x;
			TIM_SetAutoreload(TIM1, melodyDuration[x][0]);
			}
			if(time % melodyDuration[x][1] == 0) 
			{
				x++;				
				if(x == 48) x = 0;
			}	
		}

		switch (animation)
			{
				case 1:
				if(time % 20 == 0) animation_1();
				break;

				case 2:
				if(time % 20 == 0) animation_2();
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


		if(time % 5000 == 0)
		{
			animation ++;
		}

		if(time % 60000 == 0)								//toca a melodia a cada 1 minuto
		{
			play_melody = !play_melody;
			if(play_melody)									//se play_melody = 1
			{
				//init TIM1 PWM
				TIM1_PWMOut_Init( NOTE_E4, 1090-1, 50);		//inicia o TIM1 para PWM
				x = 0;				
				x_ant = -1;			

			}
			else
			{
				TIM_Cmd(TIM1,DISABLE);
			}

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