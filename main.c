#include "stm32f10x.h"
#include "delay.h"
#include <stdlib.h>

#define USARTx			USART1
#define PIN_RX			GPIO_Pin_10
#define PIN_TX			GPIO_Pin_9
#define UGPIO			GPIOA
#define GPIO_RCC		RCC_APB2Periph_GPIOA
#define USART_RCC		RCC_APB2Periph_USART1
#define AFIO_RCC		RCC_APB2Periph_AFIO

#define SGPIO			GPIOB
#define SGPIO_RCC		RCC_APB2Periph_GPIOB
#define PIN_TCLK		GPIO_Pin_11
#define PIN_TIO			GPIO_Pin_10
#define PIN_PD			GPIO_Pin_12


#define SCLK_HI()		GPIO_WriteBit(SGPIO,PIN_TCLK,Bit_SET)
#define SCLK_LO()		GPIO_WriteBit(SGPIO,PIN_TCLK,Bit_RESET)
#define SIO_HI()		GPIO_WriteBit(SGPIO,PIN_TIO,Bit_SET)
#define SIO_LO()		GPIO_WriteBit(SGPIO,PIN_TIO,Bit_RESET)
#define SIO_GET()		GPIO_ReadInputDataBit(SGPIO,PIN_TIO)
#define PD_HI()			GPIO_SetBits(SGPIO,PIN_PD);
#define PD_LO()			GPIO_ResetBits(SGPIO,PIN_PD);

// ADNS2610 registers
#define REG_CONF	0x00
#define REG_STATUS	0x01
#define REG_DELTAY	0x02
#define REG_DELTAX	0x03
#define REG_SQUAL	0x04
#define REG_MAXPX	0x05
#define REG_MINPX	0x06
#define REG_PIXSUM	0x07
#define REG_PIXDAT	0x08
#define REG_SHHI	0x09
#define REG_SHLO	0x0A
#define REG_INPROD	0x11

#define SENS_RES	18
#define PIXCOUNT 	SENS_RES*SENS_RES


void USend(char c);
void USendStr(char *str);
void MSensSend(uint8_t addr, uint8_t value);
uint8_t MSensReceive(uint8_t addr);

GPIO_InitTypeDef gis;
USART_InitTypeDef uis;

const char str_rn[] 	= "\r\n";
const char str_load[] = "Initialized";
const char str_sreg[]	= "Register: 0x";
const char str_sval[] = " value: ";

char strval[5];

uint8_t picture[PIXCOUNT];

int main(void)
{

	RCC_APB2PeriphClockCmd((GPIO_RCC | USART_RCC | AFIO_RCC | SGPIO_RCC), ENABLE);

	GPIO_StructInit(&gis);
	USART_StructInit(&uis);

	Delay_Init(72);


	// init rx pin as input weak pullup
	gis.GPIO_Pin = PIN_RX;
	gis.GPIO_Mode = GPIO_Mode_IPU;
	gis.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(UGPIO,&gis);

	// init tx pin as output alternate function open-drain pulled up
	gis.GPIO_Pin = PIN_TX;
	gis.GPIO_Mode = GPIO_Mode_AF_PP;
	gis.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(UGPIO,&gis);

	uis.USART_BaudRate = 115200;
	uis.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
	uis.USART_WordLength = USART_WordLength_8b;

	USART_Init(USARTx,&uis);

	USART_Cmd(USARTx,ENABLE);

	GPIO_StructInit(&gis);
	gis.GPIO_Pin = (PIN_TCLK | PIN_TIO | PIN_PD);
	gis.GPIO_Mode = GPIO_Mode_Out_PP;
	gis.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SGPIO,&gis);

	USendStr(str_rn);
	USendStr(str_load);
	USendStr(str_rn);

  	SCLK_LO();
	PD_HI();
	delay_us(1);
	PD_LO();

	uint16_t counter = 0;




    while(1) {


    	MSensSend(REG_CONF,0x01); // always on without sleep

		//RDReg(REG_CONF);
		//RDReg(REG_STATUS);
		//RDReg(REG_INPROD);
		//RDReg(0x20);
		//SendPixCount();
		SendPicture();
		//SendDelta();
		MSensSend(REG_CONF,0x00);

		delay_ms(90);
		counter++;

    }
}

void RDReg(uint8_t reg) {
	USendStr(str_sreg);
	itoa(reg,strval,16);
	USendStr(strval);
	uint8_t v = MSensReceive(reg);
	USendStr(str_sval);
	itoa(v,strval,10);
	USendStr(strval);
	USendStr(str_rn);
}

void SendDelta() {
	USendStr("X:");
	int8_t v = (int8_t)MSensReceive(REG_DELTAX);
	itoa(v,strval,10);
	USendStr(strval);
	USendStr("Y:");
	v = (int8_t)MSensReceive(REG_DELTAY);
	itoa(v,strval,10);
	USendStr(strval);
	USendStr(str_rn);

}

void SendPixCount() {
	MSensSend(REG_PIXDAT,0xFF);
	uint16_t i = 0;

	MSensReceive(REG_PIXDAT);
	delay_us(200);
	while ((MSensReceive(REG_PIXDAT) & 0x80) == 0x00) { // look for sof
		delay_us(200);
		i++;
	}
	USendStr("\r\nPixel count:");
	itoa(i,strval,10);
	USendStr(strval);
	USendStr(str_rn);
}

void SendPicture() {

	MSensSend(REG_PIXDAT,0xFF);
	delay_ms(100);

	uint16_t i;


	for (i = 0; i<PIXCOUNT; i++) {
		picture[i] = MSensReceive(REG_PIXDAT);
		delay_us(200);
	}

	for (i = 0; i<PIXCOUNT; i++) {
		uint8_t c = (picture[i] & 0x3F);
		USend(c+1);
	}

	USend(0xFF);
	USend(0xFF);

}



void USend(char c) {
	USART_SendData(USARTx,c);
	while(!USART_GetFlagStatus(USARTx,USART_FLAG_TC));
}

void USendStr(char *str) {
	while (*str)
		USend(*str++);
}




uint8_t MSensReceive(uint8_t addr) {
	uint8_t i;
	uint8_t value = 0;


	MSensSetAddr(addr);

	gis.GPIO_Pin = (PIN_TIO);
	gis.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gis.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SGPIO, &gis);


	delay_us(200); // handoff pause


	for (i=0; i<8; i++) {
		value = value<<1;

		SCLK_LO();
		delay_us(1);
		SCLK_HI();
		delay_us(1);

		if (SIO_GET()) {
			value |= (uint8_t)0x01;
		}


	}


	gis.GPIO_Pin = (PIN_TIO);
	gis.GPIO_Mode = GPIO_Mode_Out_PP;
	gis.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SGPIO, &gis);
	delay_us(200);
	return value;
}

void MSensSend(uint8_t addr, uint8_t value) {
	uint8_t i;
	MSensSetAddr((addr | 0x80));

	for (i=0; i<8; i++) {
		SCLK_LO();
		delay_us(1);

		if (value & (uint8_t)0x80)
			SIO_HI();
		else
			SIO_LO();

		value<<=1;

		SCLK_HI();
		delay_us(1);
	}
	delay_us(100);
}

void MSensSetAddr(uint8_t addr) {
	uint8_t i;

	for (i=0; i<8; i++) {
		SCLK_LO();
		delay_us(1);

		if (addr & (uint8_t)0x80) {
			SIO_HI();
		} else {
			SIO_LO();
		}

		addr<<=1;

		SCLK_HI();
		delay_us(1);
	}
}
