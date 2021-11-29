#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define FOSC 1843200 // Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

#include <avr/io.h>
#include <avr/eeprom.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UBRRVAL 103

void usart_init(void);
void sendbyte(unsigned char);
void sendstr(unsigned char *);
unsigned char receivebyte(void);
unsigned char receivestr(void);
int ADC_Read(void);
void ADC_Init(void);
char * toArray(int);
void ADC_measure(uint8_t channel);
float make_measure(void);
void save_measures(float,int);
void read_measures();

unsigned char onmsg[] = "ON\n";
unsigned char offmsg[] = "OFF\n";
unsigned char defaultmsg[] = "LED Status:";
unsigned char rxdata;

int main(){

	DDRB |= (1<<PB1);
	DDRB |= (1<<PB2);
	usart_init();
	ADC_Init();
	char array[10];
	int adc_value;
	float moisture;
	float moisture_values[24];
	int i = 0;
	
	while(1){
				
		rxdata = receivebyte();

		if(rxdata == 'y'){
			PORTB |= (1<<PB1);
			sendstr(defaultmsg);
			sendstr(onmsg);
		}
		else if(rxdata =='n'){
			PORTB &= ~(1<<PB1);
			sendstr(defaultmsg);
			sendstr(offmsg);
		}
		else if(rxdata == 's'){
			while (1)
			{
				moisture = make_measure();
				save_measures(moisture,46);
				eeprom_read_float((uint8_t *)46);
			}
		}else if(rxdata == 'a'){
			for( i = 0; i <=23; i++)
			{
				adc_value = ADC;
				ADC_measure(0);
				if(adc_value <= 0)
				{
					sendstr("Wilgotnosc gleby: 0%\n");
					moisture_values[i] = 0;
				}
				else
				{
					moisture = (adc_value*100.00)/550.00;
					moisture_values[i] = moisture;
					dtostrf(moisture_values[i],3,2,array);
					strcat(array,"%\n");	/* Concatenate unit of % */
					sendstr("Wilgotnosc gleby: ");
					sendstr(array);
				}
				_delay_ms(500);
			}
			
			eeprom_write_float((uint8_t*)46,moisture_values[6]);
			dtostrf(eeprom_read_float((uint8_t*)46),3,2,array);
			strcat(array,"%\n");
			sendstr(array);
			/*
			//wydrukuj ca³a liste float
			for( i = 0; i <=23; i++)
			{
				dtostrf(moisture_values[i],3,2,array);
				strcat(array,"%, ");
				sendstr(array);
			}
			*/
		}
		else if(rxdata == 'm')
		{
			for( i = 0; i <=23; i++)
			{
				moisture = make_measure();
				dtostrf(moisture,3,2,array);
				strcat(array,"%\n");	/* Concatenate unit of % */
				sendstr("Wilgotnosc gleby: ");
				sendstr(array);
				save_measures(moisture, i);
				_delay_ms(500);
			}
			sendstr("WYPISANE POMIARY");
			read_measures();
		}
		else if(rxdata == 'q')
		{
			read_measures();
		}
	}
	return 0;
}


void usart_init(void){
	UBRR0H= (unsigned char)(UBRRVAL>>8);   //high byte
	UBRR0L=(unsigned char)UBRRVAL;     			//low byte
	UCSR0B |= (1<<TXEN0) | (1<<RXEN0);		//Enable Transmitter and Receiver
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00); 	//Set data frame format: asynchronous mode,no parity, 1 stop bit, 8 bit size
}

void sendbyte(unsigned char MSG){
	while((UCSR0A&(1<<UDRE0)) == 0);     // Wait if a byte is being transmitted
	UDR0 = MSG;
}

void sendstr(unsigned char *s){
	unsigned char i = 0;
	while(s[i] != '\0'){
		sendbyte(s[i]);
		i++;
	}
}

unsigned char receivebyte(void){
	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

/*
unsigned char receivestr(void){
	static char input[15];
	uint8_t i;
	char c;
	
	for(i = 0; i <= 15; i++) {
		c = receivebyte();
		if (c == '\0') break;
		input[i] = c;
	}
}
*/
void ADC_Init(void)
{
	DDRC |= 0x0;		/*  Make ADC port as input  */
	ADCSRA = 0x87;		/*  Enable ADC, fr/128  */
}

void ADC_measure(uint8_t channel)
{
	ADMUX |= (0<<REFS1) | (1<<REFS0);
	ADCSRA |= (1<<ADEN) | (0<<ADIE) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);
	ADMUX = ((ADMUX&0xE0)+channel);
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
};

float make_measure()
{
	int adc_value = ADC;
	float moisture;
	ADC_measure(0);
	
	if(adc_value <= 0) moisture = 0;
	else moisture = (adc_value*100.00)/550.00;
	
	return moisture;
}

void save_measures(float moisture, int numberOfMeasure)
{
	eeprom_write_float((uint8_t*)46 + (4*numberOfMeasure), moisture);
}

void read_measures()
{
	uint8_t i;
	char array[10];
	
	for (i = 0; i <= 23; i++)
	{
		dtostrf(eeprom_read_float((uint8_t*)46+(4*i)),3,2,array);
		strcat(array,"%\n");
		sendstr(array);
	}
}

