#include <stdint.h>
#include <pic32mx.h>
#include <stdbool.h>
#include "header.h"

int count1 = 0;		// timer1
int count2 = 0;		// timer2
int count3 = 0;		// timer3

const uint8_t ASTEROID_ARRAY_LENGTH = 32;
const uint8_t BULLET_ARRAY_LENGTH = 8;

uint32_t asteroid_array[32];
uint32_t bullet_array[8];

uint8_t asteroid_counter = 0;
uint8_t bullet_counter = 0;

const int PAGE_HEIGHT = 8;
const int PLAYER_LENGTH = 8;
const int PLAYER_HEIGHT = 6;
const int BULLET_LENGTH = 2;
const int BULLET_HEIGHT = 3;
const int ASTEROID_LENGTH = 8;
const int ASTEROID_HEIGHT = 8;

uint8_t liv_counter = 200;		// används för att se till att man inte förlorar flera liv samtidigt
uint32_t speltid = 0;			// i sekunder
uint16_t score = 0;
uint8_t liv = 3;

uint32_t playerint = 0x00078001;

uint8_t difficulty;

void score_to_display_buffer()
{
	int i, j, nuvarande_siffra;
	uint16_t score_buffer;
	score_buffer = score;
	i = 0;
	while(score_buffer > 0)		// loopar tills man kommit till sista siffran, kollar om sista siffran i score är 0
	{
		nuvarande_siffra = score_buffer % 10;	// maskar ut minsta siffran i talet
		for(j = 0; j < 4; j++)
			display_buffer[0][124 + j - (i * 4)] |= siffror[(nuvarande_siffra * 4) + j];	// nuvarande_siffra * 4 bestämmer rad, j bestämmer byte på raden
		score_buffer /= 10;						// delar på 10, som att skifta allting 1 steg till höger
		if (score_buffer == 0 && score % 10 == 0)	// talet hade en 0 som sista siffra, skriv ut den 
		{
			display_buffer[0][124 + j - (i * 4)] |= siffror[0 + j];
		}
		i++;		// håller kolla på positioneringen
	}
}

void spawn_bullet(void)
{
	if(count2 >= 20)	// så att man inte spawnar för mycket kulor, count2 styrs av timer2
	{
		int x_koordinat = ((playerint >> 13) & 0x7f) + 3;		// kollar x-koordinat för player
										// +3 så att det blir i mitten av player x_koordinat
										// kommer användas som x_koordinat för bullet när den spawnar
		bullet_array[bullet_counter++] = (x_koordinat << 13) | 0xe11;	// skapar en ny bullet i arrayn(kolla "protokoll" för värden)
		
		if (bullet_counter == BULLET_ARRAY_LENGTH)	// resetar om countern nått slutet på bullet_array
			bullet_counter = 0;
		count2 = 0;
	}
}

void spawn_asteroid(void)
{
	int rand_x = (rand() % 121) << 13;					// mellan 0-120
	int rand_speed = ((rand() % 3) + 1) << 1;				// blir 1,2 eller 3
	asteroid_array[asteroid_counter++] = (rand_x | rand_speed | 0x021);	// kolla "protokoll" för värden
										// visibility = 1, typ = 2, y = 0
	
	if (asteroid_counter == ASTEROID_ARRAY_LENGTH)	// resetar om countern nått slutet på asteroid_array
		asteroid_counter = 0;
}

void move_player(uint32_t* return_info)
{
	int i, x_koordinat;
	x_koordinat = (playerint >> 13) & 0x7f;
	
	if (getbtns())		// kollar om en knapp blir tryckt
	{
		switch(getbtns())
		{
			case(8):;	// vänster
				if (x_koordinat > 0)	// flyttas till vänster så länge x > 0
				{
					playerint &= ~(0x7f << 13);	// "maskar bort" x-koordinat bitar
					playerint |= ((x_koordinat - 1) << 13);	// nya x = gamla x-1 
				}
			break;
			case(1):;	// höger
				if (x_koordinat < 120)	// flyttas till höger så länge x < 120
				{
					playerint &= ~(0x7f << 13);	// "maskar bort" x-koordinat bitar
					playerint |= ((x_koordinat + 1) << 13);	// nya x = gamla x-1 
				}
			break;
			case(4):;	// skjut
				spawn_bullet();
			break;
		}
	}
}

void move_vertical(uint8_t ARRAY_LENGTH, uint32_t array[], int move_value)
{
	int i, x_koordinat, y_koordinat, typ, speed, slut_pa_skarmen = 40;
	if (move_value < 0)			// om move_value är negativt, föremålet rör sig uppåt
		slut_pa_skarmen = 0;		// slut_pa_skarmen ändras därför till övre kanten av displayen
		
	for(i = 0; i < ARRAY_LENGTH; i++)	// kollar igenom arrayen, en i taget
	{
		x_koordinat = (array[i] >> 13) & 0x7f;
		y_koordinat = (array[i] >> 7) & 0x3f;
		speed = (array[i] >> 1) & 7;
		
		/* Funkar endast om move_value är +1 eller -1.
		   Om move_value är +1 kommer den inte göra någon skillnad på statementet
		   Om move_value är -1 så kommer slut_pa_skarmen vara 0,
		   vi måste därför "vända" på jämförelse tecknet genom att multiplicera med -1
		   så att y_koordinat blir negativa, vilket leder till att y_koordinat blir mindre än slut_pa_skarmen */
		if((array[i] & 1) && (y_koordinat*move_value < slut_pa_skarmen*move_value))	// visibility = 1 och är någonstans på skärmen
		{
			array[i] &= ~(0x3f << 7); 			// sätter y-koordinat till 0
			array[i] |= (y_koordinat + move_value) << 7;	// sätter nya y till gamla y+move_value
		} else if (array[i] & 1)  			// visibilty = 1 men är inte på skärmen
			array[i] &= ~0x1;			// visibility = 0
	}
}

uint8_t hit_check(uint32_t* asteroid)
{
	int i, asteroid_x_koordinat, asteroid_y_koordinat, bullet_x_koordinat, bullet_y_koordinat, player_x_koordinat;
	asteroid_x_koordinat = ((*asteroid >> 13) & 0x7f);	
	asteroid_y_koordinat = ((*asteroid >> 7) & 0x3f);
	
	for(i = 0; i < BULLET_ARRAY_LENGTH ; i++)	// kollar hit av kula på asteroid
	{
		if(bullet_array[i] & 1)		// visibility = 1
		{
			bullet_x_koordinat = ((bullet_array[i] >> 13) & 0x7f) - 8;
			bullet_y_koordinat = ((bullet_array[i] >> 7) & 0x3f);	// y-koordinat -4 så att det blir i början av kulan
			asteroid_x_koordinat = ((*asteroid >> 13) & 0x7f);	
			asteroid_y_koordinat = ((*asteroid >> 7) & 0x3f);
			
			// tänker mig asteroiden som en 8*8 kvadrat !!!!!!!!!!!!!!!!!!!!
			// måste kunna träffa om bullet är en pixel till vänster eftersom bullet är 2 pixel bred
			//  0 <= (ast_x - bul_x) <= 10 samt 0 < (bul_y - ast_y) <= 8 (Y-axis omvänd)
			// om alla statements true = HIT
			if (((asteroid_x_koordinat - bullet_x_koordinat) < 10) && (asteroid_x_koordinat - bullet_x_koordinat) > 0
				&& (bullet_y_koordinat - asteroid_y_koordinat <= 8) && (bullet_y_koordinat - asteroid_y_koordinat) > 0)
			{
				*asteroid &= ~0x1;	// HIT, visibility asteroid = 0
				bullet_array[i] &= ~0x1;	// visibility bullet = 0
				score += 5;			//  5 poäng för träff av asteroid
				return 1;
			}
		}
	}
	
	// kollar hit av asteroid på player
	// ast_y >= 28 är då den kan nå player i y-led. -8 <= (ast_x - pla_x) <= 8
	// om alla statements true = HIT
	player_x_koordinat = (playerint >> 13) & 0x7f;
	if (asteroid_y_koordinat >= 28 && (asteroid_x_koordinat - player_x_koordinat) >= -8
		&& asteroid_x_koordinat - player_x_koordinat <= 8)
	{
		*asteroid &= ~0x1;	// visibility på asteroid = 0
		if (liv_counter >= 200)	// används somdelay så att man inte förlorar alla liv samtidigt
		{
			PORTE = PORTE  >> 1;	// skiftar leds så att en lampa släcks
			liv_counter = 0;	// resetar liv_counter, den räknar +1 i work funktionen
			T3CONSET = 0x8000;	// startar timer 3
			liv -= 1;
		}						// else så blir man inte träffad
	}
	return 0;	// 1 = hit, 0 = miss
}

void player_to_display_buffer()
{
	int i, x_koordinat;
	
	x_koordinat = (playerint >> 13) & 0x7f;

	for(i = 0; i < PLAYER_LENGTH; i++)
		display_buffer[3][x_koordinat + i] |= player[i];
}


void array_to_display_buffer(uint8_t ARRAY_LENGTH, uint32_t array[], int LENGTH, const uint8_t data[], bool hit_check_bool)
{
	int i, j, x_koordinat, y_koordinat, rest;
	for(i = 0; i < ARRAY_LENGTH; i++)
	{	
		if (array[i] & 1)	// visibility fält = 1
		{
			if (hit_check_bool && (hit_check(&array[i])))	// asteroid(om hit_check_bool=1) har blivit träffad eller träffat player
				break;					// den ska därför inte skrivas ut, break
			x_koordinat = (array[i] >> 13) & 0x7f;
			y_koordinat = (array[i] >> 7) & 0x3f;
		
			rest = (y_koordinat % 8);	// resten ger oss hur många pixels som går över till nästa page
			if(y_koordinat <= 7)		// y_koordinat mellan 0-7
			{
				for(j = 0; j < LENGTH; j++)
						display_buffer[0][x_koordinat + j] |= data[j] >> (PAGE_HEIGHT - rest);
			} else if (rest) {
				for(j = 0; j < LENGTH; j++)				// skriver på första pagen
					display_buffer[(y_koordinat / 8) - 1][x_koordinat + j] |= data[j] << rest;
				for(j = 0; j < LENGTH; j++)				// skriver på andra pagen
				{
					if ((y_koordinat / 8) == 4)			// dock inte om vi precis skrev på page 3
						break;
					display_buffer[(y_koordinat / 8)][x_koordinat + j] |= data[j] >> (PAGE_HEIGHT - rest);
				}
			} else {	// rest = 0, y_koordinat = 8,16,24 eller 32
				for(j = 0; j < LENGTH; j++)
					display_buffer[(y_koordinat / 8) - 1][x_koordinat + j] |= data[j];
			}
		}
	}
}

void spelet(uint32_t* return_info)
{	

    if(*return_info & 0x8)
    {
        resume_game(asteroid_array, bullet_array, &playerint, &speltid, &score, &asteroid_counter, &bullet_counter, &liv, &difficulty,
		ASTEROID_ARRAY_LENGTH, BULLET_ARRAY_LENGTH);
    }
    else
    {   

        int i;
        for(i = 0; i < ASTEROID_ARRAY_LENGTH; i++)
            asteroid_array[i] = 0;
    
        for(i = 0; i < BULLET_ARRAY_LENGTH; i++)
            bullet_array[i] = 0;

        asteroid_counter = 0;
        bullet_counter = 0;

        speltid = 0;
        score = 0;
        liv = 3;

        playerint = 0x00078001;
    
        difficulty = (*return_info >> 20) & 0x3;
    }
    
    clear_saved();
    
    count1 = 0;
    count2 = 0;
    count3 = 0;
    
    liv_counter = 200;
	
	const uint8_t ASTEROID_ARRAY_LENGTH = 32;
	const uint8_t BULLET_ARRAY_LENGTH = 8;

	switch(liv)
	{
		case(1):;
			PORTE = 0x1;
		break;
		case(2):;
			PORTE = 0x3;
		break;
		case(3):;
			PORTE = 0x7;
		break;
	}
	
	while(1)
	{
		/* TIMER1, går kontinuerligt */
		if (IFS(0) & 0x10)		// flaggan för timer1 satt, gått 10 ms
		{
			count1++;
			IFSCLR(0) = 0x10;	// resetar flaggan för timer1
		
			move_vertical(BULLET_ARRAY_LENGTH, bullet_array, -1);
			move_player(return_info);
		
			if ((20 - (speltid/5) + (difficulty*10) <= 0 || !(count1 % (20 - (speltid/5) + (difficulty*10)))))	// ökar med -1 var 5e sekund(speltid/5)
			{													// difficulty kmr vara start värdet
				move_vertical(ASTEROID_ARRAY_LENGTH, asteroid_array, 1);
			}
			if ((60 - (speltid/5) + (difficulty*20)) <= 0 || !(count1 % (60 - (speltid/5) + (difficulty*20))))	// ökar med -1 var 5e sekund(speltid/5)
			{													// difficulty kmr vara start värdet
				spawn_asteroid();
			}
			
		
			if (count1 == 100)	// en sekund har gått, count == 100
			{
				score++;		// +1 score per sekund
				speltid++;		// +1 sek i speltid
				count1 = 0;		// resetar count1
			}
		}
	
		/* TIMER 2, används av spawn_bullet */
		if (IFS(0) & 0x100)		// flaggan för timer2 satt, gått 10 ms
		{
			IFSCLR(0) = 0x100;	// resetar flaggan för timer2
			if(count2 < 21)		// maxvärde, behöver inte räkna mer än till 20
				count2++;
		}
	
		/* TIMER 3, används för att se om player är immun, kollas i hit_check() */
		if (IFS(0) & 0x1000)		// flaggan för timer3 satt, gått 10 ms
		{
			IFSCLR(0) = 0x1000;	// resetar flaggan för timer3
			if(liv_counter >= 199)	// timer3 stoppas vid 199, blir en till liv_counter++ längre ner(=200)
			{
				T3CONCLR = 0x8000;	// timer3 stoppas
				TMR3 = 0;		// resetas
			}
			liv_counter++;
		}
		
		
		if(getbtns() & 0x2)
		{
			save_game(asteroid_array, bullet_array, &playerint, &speltid, &score, &asteroid_counter, &bullet_counter, &liv, &difficulty,
			ASTEROID_ARRAY_LENGTH, BULLET_ARRAY_LENGTH);
			*return_info &= ~0x7;
			*return_info |= 0x8;
			while(getbtns() & 0x2){}
			return;
		}
		
	
		if(liv == 0)
		{
			*return_info = difficulty << 20 | score << 4 | 3;	// lägger in score+difficulty+sidnr(efterspelet)
			return;
		}
		
		int i, j;
		for(i = 0; i < 4; i++)
			for(j = 0; j < 128; j++)
				display_buffer[i][j] = 0;	// rensar buffern
		
		/* SKRIVER ALLTING TILL DISPLAY_BUFFER */
		if (liv_counter % 10 < 5)		// används för blinkade effekt när player är immun
			player_to_display_buffer();
		
		array_to_display_buffer(BULLET_ARRAY_LENGTH, bullet_array, BULLET_LENGTH, bullet, 0);
		array_to_display_buffer(ASTEROID_ARRAY_LENGTH, asteroid_array, ASTEROID_LENGTH, asteroid, 1);
								
		score_to_display_buffer();
	
		display_buffer_to_display();
	}
}
