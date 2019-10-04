#include <stdint.h>
#include <pic32mx.h>
#include <stdbool.h>
#include "header.h"

#define DISPLAY_CHANGE_TO_COMMAND_MODE (PORTFCLR = 0x10)
#define DISPLAY_CHANGE_TO_DATA_MODE (PORTFSET = 0x10)

#define DISPLAY_ACTIVATE_RESET (PORTGCLR = 0x200)
#define DISPLAY_DO_NOT_RESET (PORTGSET = 0x200)

#define DISPLAY_ACTIVATE_VDD (PORTFCLR = 0x40)
#define DISPLAY_ACTIVATE_VBAT (PORTFCLR = 0x20)

#define DISPLAY_TURN_OFF_VDD (PORTFSET = 0x40)
#define DISPLAY_TURN_OFF_VBAT (PORTFSET = 0x20)

/* Prevent compiler errors */
void *stdin, *stdout, *stderr;

void quicksleep(int cyc) {
	int i;
	for(i = cyc; i > 0; i--);
}

uint8_t spi_send_recv(uint8_t data) {
	while(!(SPI2STAT & 0x08));
	SPI2BUF = data;
	while(!(SPI2STAT & 1));
	return SPI2BUF;
}

void init()
{
	/* Set up peripheral bus clock */
    /* OSCCONbits.PBDIV = 1; */
    OSCCONCLR = 0x100000; /* clear PBDIV bit 1 */
	OSCCONSET = 0x080000; /* set PBDIV bit 0 */
	
	/* Set up output pins */
	AD1PCFG = 0xFFFF;
	ODCE = 0x0;
	TRISECLR = 0xFF;
	PORTE = 0x0;
	
	/* Output pins for display signals */
	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;

	
	/* Set up SPI as master */
	SPI2CON = 0;
	SPI2BRG = 4;
	/* SPI2STAT bit SPIROV = 0; */
	SPI2STATCLR = 0x40;
	/* SPI2CON bit CKP = 1; */
    SPI2CONSET = 0x40;
	/* SPI2CON bit MSTEN = 1; */
	SPI2CONSET = 0x20;
	/* SPI2CON bit ON = 1; */
	SPI2CONSET = 0x8000;
	
	/* Set up i2c */
	I2C1CON = 0x0;
	/* I2C Baud rate should be less than 400 kHz, is generated by dividing
	the 40 MHz peripheral bus clock down */
	I2C1BRG = 0x0C2;
	I2C1STAT = 0x0;
	I2C1CONSET = 1 << 13; //SIDL = 1
	I2C1CONSET = 1 << 15; // ON = 1
	uint16_t temp = I2C1RCV; //Clear receive buffer
	
	TRISDSET = 0xfe0;	// sätter knappar 2-4 + switches till input
	TRISFSET = 0x2;		// sätter knapp 1 som input
	
	
    DISPLAY_CHANGE_TO_COMMAND_MODE;
	quicksleep(10);
	DISPLAY_ACTIVATE_VDD;
	quicksleep(1000000);
	
	spi_send_recv(0xAE);
	DISPLAY_ACTIVATE_RESET;
	quicksleep(10);
	DISPLAY_DO_NOT_RESET;
	quicksleep(10);
	
	spi_send_recv(0x8D);
	spi_send_recv(0x14);
	
	spi_send_recv(0xD9);
	spi_send_recv(0xF1);
	
	DISPLAY_ACTIVATE_VBAT;
	quicksleep(10000000);
	
	spi_send_recv(0xA1);
	spi_send_recv(0xC8);
	
	spi_send_recv(0xDA);
	spi_send_recv(0x20);
	
	spi_send_recv(0xAF);
	
 	T1CONSET = 0x70;	// sätter bit 4-6 till 111, prescale 1:256
	T2CONSET = 0x70;	// sätter bit 4-6 till 111, prescale 1:256
	T3CONSET = 0x70;	// sätter bit 4-6 till 111, prescale 1:256
	PR1 = 3125;		// 31250=((80*10^6)/256)/100 - ((clockspeed)/prescale)/"100 per sek"
	PR2 = 3125;		// 31250=((80*10^6)/256)/100 - ((clockspeed)/prescale)/"100 per sek"
	PR3 = 3125;		// 31250=((80*10^6)/256)/100 - ((clockspeed)/prescale)/"100 per sek"
	TMR1 = 0;		// resetar timer1
	TMR2 = 0;		// resetar timer2
	TMR3 = 0;		// resetar timer3
	IFS(0) = 0; 		// clearar flaggor
	T1CONSET = 0x8000;	// startar timer1
	T2CONSET = 0x8000;	// startar timer2

  	srand(TMR1);
}

/* Wait for I2C bus to become idle */
void i2c_idle() {
	while(I2C1CON & 0x1F || I2C1STAT & (1 << 14)); //TRSTAT
}

/* Send one byte on I2C bus, return ack/nack status of transaction */
bool i2c_send(uint8_t data) {
	i2c_idle();
	I2C1TRN = data;
	i2c_idle();
	return !(I2C1STAT & (1 << 15)); //ACKSTAT
}

/* Receive one byte from I2C bus */
uint8_t i2c_recv() {
	i2c_idle();
	I2C1CONSET = 1 << 3; //RCEN = 1
	i2c_idle();
	I2C1STATCLR = 1 << 6; //I2COV = 0
	return I2C1RCV;
}

/* Send acknowledge conditon on the bus */
void i2c_ack() {
	i2c_idle();
	I2C1CONCLR = 1 << 5; //ACKDT = 0
	I2C1CONSET = 1 << 4; //ACKEN = 1
}

/* Send not-acknowledge conditon on the bus */
void i2c_nack() {
	i2c_idle();
	I2C1CONSET = 1 << 5; //ACKDT = 1
	I2C1CONSET = 1 << 4; //ACKEN = 1
}

/* Send start conditon on the bus */
void i2c_start() {
	i2c_idle();
	I2C1CONSET = 1 << 0; //SEN
	i2c_idle();
}

/* Send restart conditon on the bus */
void i2c_restart() {
	i2c_idle();
	I2C1CONSET = 1 << 1; //RSEN
	i2c_idle();
}

/* Send stop conditon on the bus */
void i2c_stop() {
	i2c_idle();
	I2C1CONSET = 1 << 2; //PEN
	i2c_idle();
}

// för high_address använder vi alltid 0x00
// för difficulty gäller: hard = 0, medium = 1, easy = 2
// received[] ska ha 12 platser(received[12])
void receive_highscore(uint8_t difficulty, uint8_t* received)
{
	int i, EEPROM_ADDRESS = 80;	// 80 = 1010000
	
	/* Send start condition and address of the eeprom with
	write flag (lowest bit = 0) until the eeprom sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	/* Send register number we want to access */
	i2c_send(0x00);				// address high byte
	i2c_send(difficulty << 4);	// address low byte, skiftar till vänstra nibblen, högra nibblen börjar alltid på 0
	
	/* Now send another start condition and address of the eeprom with
	read mode (lowest bit = 1) until the eeprom sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send((EEPROM_ADDRESS << 1) | 1));
	
	/* Now we can start receiving data from the data register */
	for(i = 0; i < 11; i++)
	{
		received[i] = i2c_recv();	// hämtar en byte från eepromen och lägger i arrayen
		i2c_ack();			// ackar att vi fått data
	}
	received[11] = i2c_recv();
	/* To stop receiving, send nack and stop */
	i2c_nack();
	i2c_stop();
}

// flyttar ner/tar bort gamla highscores 
void move_old_highscore(uint8_t difficulty, int placering)
{
	int i, j, memory_placering, move_to_pos = 3, amount_of_moves = 0;	// amount_of_moves = hur många highscores som ska flyttas
	uint8_t received[12], name[3];
	uint16_t memory_char_buffer, score_buffer;
	// memory_placering börjar alltid på 4, då första flytten kommer alltid vara pos2->pos3
	memory_placering = 4;	// används för att översätta från placering(1,2,3) till minnesadresserna(0,4,8)
	// hämtar alla highscores så att dom kan flyttas till rätt position
	receive_highscore(difficulty, received);	// received = &received[0]
	
	switch(placering)	// placering 1,2,3
	{
		case(1):;
			amount_of_moves = 2;	// pos2->pos3, pos1->pos2, efter det läggs det nya highscoret in
		break;
		case(2):;
			amount_of_moves = 1;	// pos2->pos3
		break;
	}
	
	for(i = 0; i < amount_of_moves; i++)
	{
		// lagrar name och score i buffrar
		memory_char_buffer = (received[memory_placering] << 8) | received[memory_placering+1];
		score_buffer = (received[memory_placering+2] << 8) | received[memory_placering+3];
		
		memory_char_to_char(memory_char_buffer, name);	// name = &name[0], får tillbaka memory_char_buffer i name[]
		send_highscore(difficulty, name, score_buffer, move_to_pos);	// läggs in i en placering högre
		
		// flyttar från pos2 till pos1 inför den andra loopen om den ska köras
		move_to_pos--;
		memory_placering -= 4;
	}
}

// för high_address använder vi alltid 0x00
// för difficulty gäller: hard = 0, medium = 1, easy = 2
void send_highscore(uint8_t difficulty, uint8_t* name, uint16_t score, int placering)
{
	int i, memory_placering, EEPROM_ADDRESS = 80;	// 80 = 1010000
	memory_placering = (placering-1) * 4;	// används för att översätta från placering(1,2,3) till minnesadresserna(0,4,8)
	
	/* Send start condition and address of the temperature sensor with
	write mode (lowest bit = 0) until the temperature sensor sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	/* Send register number we want to access */
	i2c_send(0x00);					// address high byte
	i2c_send((difficulty << 4) | memory_placering);	// address low byte
	
	// översätter från ascii A=0x41, ..., Z=0x5a till A=0, ..., Z=25 som kan sparas på 5 bitar i minnet
	uint16_t name_memory_char = char_to_memory_char(name);	// name = &name[0]
	
	// lägger in dom 2 första bytesen i eepromen, innehållande validbit+bokstäver
	i2c_send((name_memory_char >> 8) & 0xff);	// byte1, valid bit sätts i char_to_memory_char()
	i2c_send(name_memory_char & 0xff);		// byte2
	// lägger in scoren
	i2c_send((score >> 8) & 0xff);			// byte3
	i2c_send(score & 0xff);				// byte4 - läggs in på rad i eepromen
	
	/* Send stop condition */
	i2c_stop();
}

void chars_to_display_buffer(int line, uint8_t* s, int alignment, int antal)
{
	if(line < 0 || line >= 4)
		return;
	if(!s)
		return;
	if (alignment >= 127)
		return;
	
	int i, j, count_pixels = 0;
	
	for(i = 0; i < antal; i++)
	{
		for(j = 0; j < 8; j++)
			display_buffer[line][alignment + count_pixels + j] |= font[(*s)*8 + j];
		s++;				// byter till nästa char
		count_pixels += 8;	// 1 char = 8 pixlar, har skrivits ut i x-led
	}
}

void memory_char_to_char(uint16_t name_memory_char, uint8_t* name)
{
	int i, shamt = 0;
	for(i = 2; i >= 0; i--)
	{
		name[i] = ((name_memory_char >> shamt) & 0x1f) + 0x41;
		shamt += 5;
	}
	return;
}

uint16_t char_to_memory_char(uint8_t* name)
{
	int i, shamt = 0;
	uint16_t name_memory_char = 0x8000;	// sätter första biten(valid biten) till 1
	for(i = 2; i >= 0; i--)
	{
		name_memory_char |= (name[i] - 0x41) << shamt;	// får ett tal mellan 0(=a), ..., 25(=z)
		shamt += 5;
	}
	return name_memory_char;
}

uint32_t check_highscore(uint32_t* return_info)
{
	int i, current_index = 10;	// current_index börjar på 10 då det är där scoren på placering 3 börjar
	uint8_t received[12], placering, difficulty = (*return_info >> 20) & 0x3;
	uint16_t score_buffer, score;
	score = (*return_info >> 4) & 0xffff;
	
	// scoresen kommer ligga i received[2]+received[3], received[6]+received[7] och received[10]+received[11]
	receive_highscore(difficulty, received);	// received = &received[0]
	for(i = 2; i >= 0; i--)
	{
		// kollar scores nerifrån och upp, placering 3->2->1
		score_buffer = received[current_index] << 8 | received[current_index+1];
		if ((score < score_buffer) && (received[current_index-2] & 0x80))	// kollar om score < highscore och att validbiten är 1
		{
			if (i == 2)		// första loopiterationen, score mindre än placering 3, alltså inget highscore
				return 0;	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			placering = i+2;	// high score placering 2 eller 3 eftersom man hamnat i denna if och i != 2
			return (uint32_t) (difficulty << 19) | (score << 3) |  (placering << 1) | 1;// seprotokoll för positioneringar
		}
		current_index -= 4;	// 10->6->2 (=startindex för scorsen i received[])
	}
	// betyder att score > alla highscores, placering 1
	return (uint32_t) (difficulty << 19) | (score << 3) |  (1 << 1) | 1; 	// se protokoll för positioneringar
}

void score_to_string(uint16_t score, char* score_char)
{
	int i = 0, score_buffer, nuvarande_siffra;
	score_buffer = score;
	while(score_buffer > 0)
	{
		score_buffer /= 10;	// delar på 10, som att skifta allting 1 steg till höger
		i++;				// räknar antalet tal som finns i score
	}
	
	score_char[i--] = 0x00;		// lägger in NULL byte längst bak och minskar i med 1
								// ascii för siffror: 0 = 0x30, 1 = 0x31, ... , 9 = 0x39
	while(score > 0)
	{
		nuvarande_siffra = score % 10;				// maskar ut minsta siffran i talet
		score_char[i] = 0x30 | nuvarande_siffra;	// gör om siffran till ascii-kod
		score /= 10;								// delar på 10, som att skifta allting 1 steg till höger
		i--;										// håller koll på positioneringen
	}
}

void menu(int* current, int max)
{
	if ( getbtns() & 0x1 )				// if button 1 is pressed
	{
		if( *current < max )			// and if pointer is above max
			*current += 1;				// move down
		else							// and if pointer is at max
			*current = 0;				// move to top
	}														
	
	if ( getbtns() & 0x8 )				// if button 4 is pressed 
	{
		if( *current > 0 )				// and if pointer is below line 0
			*current -= 1;				// move up
		else							// and if pointer is at line 0
			*current = max;				// move to max
	}
	
	while( getbtns() & 0x9 );			// ligger kvar tills knappen släpps
}

void pointer_to_display_buffer(int line, int alignment)
{
	int i;
	for (i = 0; i < 5; i++)
		display_buffer[line][alignment + i] |= pekare[i];
}

void char_pointer_to_display_buffer(int alignment)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		display_buffer[0][alignment + i] |= ovre_pekare[i];
		display_buffer[2][alignment + i] |= nedre_pekare[i];
	}
}

void change_char(char* c, int* i, int* alignment)
{
	switch(getbtns())
	{
		case(1):;
			if(*c < 'Z')		// så länge c < 0x5a = Z, ökar man
				*c+=1;
			else				// börjar om från A om man ökar Z
				*c = 0x41;
		break;
		case(8):;
			if(*c > 'A')		// så länge c > 0x41 = A, minskar man
				*c-=1;
			else				// går till Z om man minskar A
				*c = 0x5a;
		break;
		case(4):;
			*i+=1;				// "byter till nästa char i arrayen"
			*alignment += 8;	// flyttar char pekaren 8 steg, alltså till nästa char
		break;
	}
		while(getbtns()){};
}

void string_to_display_buffer(int line, char *s, int alignment)	
{
	if(line < 0 || line >= 4)
		return;
	if(!s)
		return;
	if (alignment >= 127)
		return;
	
	int i, count_pixels = 0;
	while ((*s != '\0') && (count_pixels + 8) < (127 - alignment))
	{
		for(i = 0; i < 8; i++)
			display_buffer[line][alignment + count_pixels + i] |= font[(*s)*8 + i];
		
		s++;				// byter till nästa char
		count_pixels += 8;	// 1 char = 8 pixlar, har skrivits ut i x-led
	}
}

void display_buffer_to_display(void)
{
	DISPLAY_CHANGE_TO_COMMAND_MODE;
  	spi_send_recv(0x00);	// lower 8bit column adress
  	spi_send_recv(0x10);	// upper 8bit column adress
  
  	spi_send_recv(0x20);	// addressingmode
  	spi_send_recv(0x00);	// page = 10, horizontal = 00, vertical = 01
  
  	spi_send_recv(0x22);	// set page address
  	spi_send_recv(0x00);	// start
  	spi_send_recv(0x03);	// stop
  
  	DISPLAY_CHANGE_TO_DATA_MODE;
	int i, j;
  	for(i = 0; i < 4; i++)
	  	for(j = 0; j < 128; j++)
			spi_send_recv(display_buffer[i][j]);	// skriver ut display_buffer till skärmen
}

void clear_display_buffer(void)
{
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 128; j++)
			display_buffer[i][j] = 0;
}

void save_game(uint32_t* asteroid_array, uint32_t* bullet_array, uint32_t* playerint, uint32_t* speltid, uint16_t* score,
		uint8_t* asteroid_counter, uint8_t* bullet_counter, uint8_t* liv, uint8_t* difficulty,
		uint8_t ASTEROID_ARRAY_LENGTH, uint8_t BULLET_ARRAY_LENGTH)
{
	int i, j, EEPROM_ADDRESS = 80;	// 80 = 1010000
	
	/* ASTEROID_ARRAY1(0-15) = 64 BYTES , adress: 0x0100*/
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	i2c_send(0x01);		// address high byte
	i2c_send(0x00);		// address low byte
	
	for(i = 0; i < ASTEROID_ARRAY_LENGTH/2; i++)	// 0-15= 64 bytes
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
			i2c_send((asteroid_array[i] >> j) & 0xff);
			
	i2c_stop();
	
	/* ASTEROID_ARRAY1(16-31) = 64 BYTES, adress: 0x0200 */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	i2c_send(0x02);		// address high byte
	i2c_send(0x00);		// address low byte
	
	// lägger in alla asteroider (32*ASTEROID_ARRAY_LENGTH)
	for(i = ASTEROID_ARRAY_LENGTH/2; i < ASTEROID_ARRAY_LENGTH; i++)	// 16-31 = 64 bytes
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
			i2c_send((asteroid_array[i] >> j) & 0xff);
			
	i2c_stop();
	
	/* RESTEN, med valid bit på första platsen(0x0300) */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	i2c_send(0x03);		// address high byte
	i2c_send(0x00);		// address low byte
	
	i2c_send(0xff);		// valid byte != 0
			
	// lägger in alla bullets  = 32 bytes
	for(i = 0; i < BULLET_ARRAY_LENGTH; i++)
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
			i2c_send((bullet_array[i] >> j) & 0xff);
	
	// lägger in playerint = 4 bytes
	for(i = 24; i >= 0; i-=8)	// 24,16,8,0
		i2c_send((*playerint >> i) & 0xff);
		
	// lägger in speltid = 4 bytes
	for(i = 24; i >= 0; i-=8)	// 24,16,8,0
		i2c_send((*speltid >> i) & 0xff);
		
	// lägger in score = 2 bytes
	for(i = 8; i >= 0; i-=8)	// 8,0
		i2c_send((*score >> i) & 0xff);
	
	// 1 bytes
	i2c_send(*asteroid_counter);
	i2c_send(*bullet_counter);
	i2c_send(*liv);	
	i2c_send(*difficulty);	

	i2c_stop();
}

void resume_game(uint32_t* asteroid_array, uint32_t* bullet_array, uint32_t* playerint, uint32_t* speltid, uint16_t* score,
		uint8_t* asteroid_counter, uint8_t* bullet_counter, uint8_t* liv, uint8_t* difficulty,
		uint8_t ASTEROID_ARRAY_LENGTH, uint8_t BULLET_ARRAY_LENGTH)
{
	int i, j, EEPROM_ADDRESS = 80;	// 80 = 1010000
	
	/* ASTEROID_ARRAY DEL 1 */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	/* Send register number we want to access */
	i2c_send(0x01);			// address high byte
	i2c_send(0x00);			// address low byte

	do {
		i2c_start();
	} while(!i2c_send((EEPROM_ADDRESS << 1) | 1));

	for(i = 0; i < ASTEROID_ARRAY_LENGTH/2; i++)
	{
		asteroid_array[i] = 0;		// rensar på gamla värdet innan orar in
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
		{
			asteroid_array[i] |= i2c_recv() << j;
			i2c_ack();
		}
	}
	i2c_nack();
	i2c_stop();
	
	/* ASTEROID_ARRAY DEL 2 */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	/* Send register number we want to access */
	i2c_send(0x02);			// address high byte
	i2c_send(0x00);			// address low byte

	do {
		i2c_start();
	} while(!i2c_send((EEPROM_ADDRESS << 1) | 1));
	
	for(i = ASTEROID_ARRAY_LENGTH/2; i < ASTEROID_ARRAY_LENGTH; i++)
	{
		asteroid_array[i] = 0;		// rensar på gamla värdet innan orar in
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
		{
			asteroid_array[i] |= i2c_recv() << j;
			i2c_ack();
		}
	}
	i2c_nack();
	i2c_stop();
	
	/* RESTEN, hoppar över valid byte(0x0300) */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	/* Send register number we want to access */
	i2c_send(0x03);			// address high byte
	i2c_send(0x01);			// address low byte

	do {
		i2c_start();
	} while(!i2c_send((EEPROM_ADDRESS << 1) | 1));
			
	// lägger in alla bullets (32*BULLET_ARRAY_LENGTH)
	for(i = 0; i < BULLET_ARRAY_LENGTH; i++)
	{
		bullet_array[i] = 0;		// rensar på gamla värdet innan orar in
		for(j = 24; j >= 0; j-=8)	// 24,16,8,0
		{
			bullet_array[i] |= i2c_recv() << j;
			i2c_ack();
		}
	}
	
	// lägger in playerint (32)
	*playerint = 0;			// rensar på gamla värdet innan orar in
	for(i = 24; i >= 0; i-=8)	// 24,16,8,0
	{
		*playerint |= i2c_recv() << i;
		i2c_ack();
	}
		
	// lägger in speltid (32), speltid redan 0
	for(i = 24; i >= 0; i-=8)	// 24,16,8,0
	{
		*speltid |= i2c_recv() << i;
		i2c_ack();
	}
		
	// lägger in score (16), score redan 0
	for(i = 8; i >= 0; i-=8)	// 8,0
	{
		*score |= i2c_recv() << i;
		i2c_ack();
	}
		
	*asteroid_counter = i2c_recv();	// (8)
	i2c_ack();
	*bullet_counter = i2c_recv();	// (8)
	i2c_ack();
	*liv = i2c_recv();		// (8)
	i2c_ack();
	*difficulty = i2c_recv();	// (8)
	
	/* To stop receiving, send nack and stop */
	i2c_nack();
	i2c_stop();
	
	// ska sätta valid byten till 0 också! (0x0030)
	/* Send start condition and address of the temperature sensor with
	write mode (lowest bit = 0) until the temperature sensor sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	/* Send register number we want to access */
	// sparade spelet börjar vid adress 0x0030
	i2c_send(0x03);		// address high byte
	i2c_send(0x00);		// address high byte
	
	i2c_send(0x00);		// valid byte sätts till 0
	
	i2c_stop();
}

// kollar om valid byte är satt eller inte, returnerar saved(valid != 0) = 1 eller notsaved(valid == 0) = 0
bool check_saved(void)
{
	int EEPROM_ADDRESS = 80;	// 80 = 1010000
	uint8_t temp;
	
	/* Send start condition and address of the eeprom with
	write flag (lowest bit = 0) until the eeprom sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	/* Send register number we want to access */
	i2c_send(0x03);			// address high byte
	i2c_send(0x00);			// address low byte
	
	/* Now send another start condition and address of the eeprom with
	read mode (lowest bit = 1) until the eeprom sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send((EEPROM_ADDRESS << 1) | 1));
	
	temp = i2c_recv();
	i2c_nack();
	i2c_stop();
	
	return (temp) ? 1 : 0;	// if temp != 0 returnas 1, annars returnas 0
}

void clear_saved(void)
{
	int EEPROM_ADDRESS = 80;	// 80 = 1010000

	/* Send start condition and address of the temperature sensor with
	write mode (lowest bit = 0) until the temperature sensor sends
	acknowledge condition */
	do {
		i2c_start();
	} while(!i2c_send(EEPROM_ADDRESS << 1));
	
	/* Send register number we want to access */
	// sparade spelet börjar vid adress 0x0030
	i2c_send(0x03);		// address high byte
	i2c_send(0x00);		// address low byte
	
	i2c_send(0x00);		// valid byte sätts till != 0

	/* Send stop condition */
	i2c_stop();
}

/* SWITCHES */
int getsw(void)
{
	return (PORTD & 0xf00) >> 8;	// maskar bitar 11-8 och shiftar dom till LSB
}

/* BUTTONS */
int getbtns(void)
{
	return ((PORTD & 0xe0) >> 4) | (PORTF & 0x2) >> 1;	// maskar bitar 7-5 och shiftar dom till LSB+1 och orar med btn 1
}
