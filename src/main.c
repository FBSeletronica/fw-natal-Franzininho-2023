/******************************************************************************
* Christmas Ball - Franzininho
* 
* This code is part of Christmas Ball project for 2023 Christmas Projects by
* Franzininho Community.
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


#define TIME_BT_ENTER_STANDBY 2500

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
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

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
		GPIO_Write(LEDS_PORT, leds & 0x000000FF); 	//LED GROUP1 Write
		GPIO_SetBits(T_PORT, T1_PIN);				//turn on T1
		break;

		case 1:										//LEDS GROUP2							
		group++;									//increment group
		GPIO_ResetBits(T_PORT, T1_PIN);				//turn off T1
		GPIO_Write(LEDS_PORT, leds>>8);				//LED GROUP2 Write
		GPIO_SetBits(T_PORT, T2_PIN);				//turn on T2
		break;

		case 2:											//LEDS GROUP3
		group++;										//increment group
		GPIO_ResetBits(T_PORT, T2_PIN);					//turn off T2
		GPIO_Write(LEDS_PORT, invertBits(leds>>15)); 	//LED GROUP3 Write - invertBits because 
														//the leds are in the opposite direction
														//in the PCB
		GPIO_SetBits(T_PORT, T3_PIN);				  	//turn on T3			
		
													 	
		break;

		case 3:											//LEDS GROUP4
		GPIO_ResetBits(T_PORT, T3_PIN);					//turn off T3
		GPIO_Write(LEDS_PORT, invertBits(leds>>23));	//LED GROUP4 Write - invertBits because 
														//the leds are in the opposite direction
														//in the PCB
		GPIO_SetBits(T_PORT, T4_PIN);					//turn on T4
		
														
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
		leds = 0b00100100100100100010010010010010;
		RGB++;
		break;

		case 2:
		leds = 0b10010010010010010100100100100100;
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


void init_system(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	Delay_Init();
}

void init_gpio()
{
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


	// Set button pin as input
	BUTTON_CLOCK_ENABLE;
    GPIO_InitStructure.GPIO_Pin  = BUTTON_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUTTON_PORT, &GPIO_InitStructure);
}

void deinit_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;

    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void init_timer_update_leds(void)
{
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
}

void init_ext_int(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	EXTI_InitTypeDef EXTI_InitStructure = {0};
	NVIC_InitTypeDef NVIC_InitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

void IWDG_Feed_Init(u16 prer, u16 rlr)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(prer);
    IWDG_SetReload(rlr);
    IWDG_ReloadCounter();
    IWDG_Enable();
}

void enter_standBy_mode(void)
{
	deinit_gpio(); 
	init_ext_int();
    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFI);
	IWDG_Feed_Init( IWDG_Prescaler_128, 500 );  //reset
}

int main(void)
{
	// Init System
	init_system();
	init_gpio();
	
	//turn off all transistors
	GPIO_ResetBits(T_PORT, ALL_T);
	init_timer_update_leds();
	
	uint8_t animation = 1;		//animation number
	uint8_t button_state = 1;  //variable to store button state
	uint16_t button_pressed = 0; //variable to store button pressed time

	while (1)
	{
		Delay_Ms(1);
		time++;

		int new_state = GPIO_ReadInputDataBit(BUTTON_PORT, BUTTON_PIN);		//read button state

		if(new_state != button_state)				//if button state changed
        {
			button_state = new_state; 				//update button state
            if (button_state == 0)                  //if button is pressed
            {
				button_pressed = 0;					//reset button pressed time
				time = 0;							//reset time count
            }
            else                                    //if button is not pressed
            {
				if(button_pressed <TIME_BT_ENTER_STANDBY)			//if button pressed time is less than 5 seconds
				{
					animation++;						//change animation
					if(animation > 6) animation = 1;	//if animation is bigger than 6, reset animation
				}
            }
        }
		else										//if button state not changed
		{
			if (new_state == 0)                  	//if button is pressed
			{
				button_pressed++;					//increment button pressed time
				if(button_pressed > TIME_BT_ENTER_STANDBY)			//if button pressed time is bigger than 5 seconds
				{
					leds = 0x00;
					Delay_Ms(500);
					enter_standBy_mode();
				}
			}
		}

		//animations
		switch (animation)
		{
			case 1:
			if(time % 100 == 0) animation_1();		//4,24 mA
			break;

			case 2:
			if(time % 100 == 0) animation_2();		//4,24 mA	
			break;

			case 3:
			if(time % 100 == 0)animation_3();		//6,57 mA
			break;

			case 4:
			if(time % 100 == 0 ) animation_4();		//6,57 mA
			break;

			case 5:
			if(time % 1000 == 0) animation_5();		//5,76 mA
			break;
			
			case 6:
			if(time % 10 == 0) animation_6();		//4,24 mA
			break;

			default:
			animation = 1;
			break;
		}


		if(time % 30000 == 0)
		{

			GPIO_InitTypeDef GPIO_InitStructure = {0};

			EXTI_InitTypeDef EXTI_InitStructure = {0};

			RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

			EXTI_InitStructure.EXTI_Line = EXTI_Line9;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
			EXTI_InitStructure.EXTI_LineCmd = ENABLE;
			EXTI_Init(&EXTI_InitStructure);

			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;

			GPIO_Init(GPIOA, &GPIO_InitStructure);
			GPIO_Init(GPIOC, &GPIO_InitStructure);
			GPIO_Init(GPIOD, &GPIO_InitStructure);
			

			RCC_LSICmd(ENABLE);
			while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
			PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_10240);
			PWR_AWU_SetWindowValue(25);
			PWR_AutoWakeUpCmd(ENABLE);
			PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);

			init_gpio();
			animation++;
		}

		if(time>= 3600000)
		{
			enter_standBy_mode();
		}
		
	}
}


//Interrupts

void NMI_Handler(void) 
{


}
void HardFault_Handler(void)
{
	while (1)
	{
	}
}

// ISR for TIM2
void TIM2_IRQHandler(void){									// ISR for TIM2
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {	// if TIM2 update interrupt flag is set
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			// clear TIM2 update interrupt flag

		leds_write(leds);									//update leds
    }
}


/*********************************************************************
 * @fn      EXTI0_IRQHandler
 *
 * @brief   This function handles EXTI0 Handler.
 *
 * @return  none
 */
void EXTI7_0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0)!=RESET)
    {
        printf("EXTI0 Wake_up\r\n");
        EXTI_ClearITPendingBit(EXTI_Line0);     /* Clear Flag */
    }
}
