#include <stdint.h>
#include <pic32mx.h>
#include "header.h"

void efterspel(uint32_t* return_info) 
{	
	// 0x41 = A, startvärde vid input av highscore
	char name[] = { 0x41, 0x41, 0x41};
	int i = 0, alignment = 10, current = 0, max = 1;
	char score_string[17];
	score_to_string((*return_info >> 4) & 0xffff, score_string);
	
	// kollar om highscore eller ej, jämförs med highscore för denna difficulty i minnet
	uint32_t check_highscore_return = check_highscore(return_info);
	
	while(1) 
	{
		clear_display_buffer();
		string_to_display_buffer(0, "GAME OVER", 10);
		string_to_display_buffer(1, "SCORE: ", 10);
		string_to_display_buffer(1, score_string, 70);
		display_buffer_to_display();
		
		if(getbtns() & 0x4) {
			while(getbtns());
			break;
		}
	}
	
	if(check_highscore_return & 0x1)
	{	
		uint8_t difficulty = (check_highscore_return >> 19) & 0x3;		// maskar ut difficulty
		uint16_t score = (*return_info >> 4) & 0xffff;					// maskar ut score
		int placering = (check_highscore_return >> 1) & 0x3;			// maskar ut placering
		
		while(i <= 2) 
		{
			clear_display_buffer();
			chars_to_display_buffer(1, name[0], 10, 1);
			chars_to_display_buffer(1, name[1], 18, 1);
			chars_to_display_buffer(1, name[2], 26, 1);
			string_to_display_buffer(1, score_string, 40);
			string_to_display_buffer(3, "RANK:", 10);
			chars_to_display_buffer(3, (char) (placering+0x30), 52, 1);	// 0x30 = 0 ascii
			char_pointer_to_display_buffer(alignment);
			display_buffer_to_display();
			
			// byter char med btn1+4, plussar på 1 till i och 8 till alignment när man trycker btn3
			change_char(&name[i], &i, &alignment);
		}
		move_old_highscore(difficulty, placering);
		send_highscore(difficulty, name, score, placering);	 // skickar till eeprom
	}
	
	
	while(1) 
	{
		clear_display_buffer();
		string_to_display_buffer(0, "HIGH SCORES", 10);
		string_to_display_buffer(1, "BACK TO START", 10);
		
		if(check_highscore_return & 0x1)
			chars_to_display_buffer(3, name, 10, 3);
		
		string_to_display_buffer(3, score_string, 40);
		pointer_to_display_buffer(current, 4);
		menu(&current, max);
		display_buffer_to_display();
		
		if(getbtns() & 0x4) 
		{
			// översätter från "current" till sidnummer
			switch(current)	
			{
				case(0):;
					*return_info = 0x4;
				break;
				case(1):;
					*return_info = 0x0;
				break;
			}
			while( getbtns() & 0x4 );
			return;
		}
	}
}
