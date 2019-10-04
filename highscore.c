#include <stdint.h>
#include <pic32mx.h>
#include <stdbool.h>
#include "header.h"	

void highscore(uint32_t* return_info)
{
	int i, memory_placering, alignment = 4, current = 0, max = 3;
	uint8_t received[12];
	uint8_t names[3][3];			// sparar names 3*3 i ascii
	uint8_t score_chars[3][17];		// lägger score i ascii format i dessa, 16+null byte
	uint16_t memory_char_buffer, score_buffer;
	
	while(1)
	{
		memory_placering = 0; // sätts till 0 inför varje iteration
		while(1)	// väljer svårighetsgrad för score
		{
			clear_display_buffer();
			string_to_display_buffer(0, "HARD", 10);
			string_to_display_buffer(1, "MEDIUM", 10);
			string_to_display_buffer(2, "EASY", 10);
			string_to_display_buffer(3, "BACK TO START", 10);
			pointer_to_display_buffer(current, alignment);
			display_buffer_to_display();
			menu(&current, max);
			
			if((getbtns() & 0x4) && (current == 3))
			{
				*return_info &= ~0x7;
				*return_info |= 0;			// ska tillbaka till start
				while(getbtns() & 0x4) {}
				return;
			} else if((getbtns() & 0x4))	// ska titta på score, gå vidare till switchen
			{	
				while(getbtns() & 0x4) {}
				break;
			}
		}
		
		receive_highscore(current, received);
		
		for (i = 0; i < 3; i++)
		{
			// lagrar name i buffer
			memory_char_buffer = (received[memory_placering] << 8) | received[memory_placering+1];
			memory_char_to_char(memory_char_buffer, &names[i][0]);	// skickar adressen till start punkten i arrayn
			
			// lagrar score i buffer
			score_buffer = (received[memory_placering+2] << 8) | received[memory_placering+3];
			score_to_string(score_buffer, &score_chars[i][0]);
			
			memory_placering += 4;		// 0->4->8
		}
		
		while(1)	// kollar på scores
		{
			clear_display_buffer();
			for(i = 0; i < 3; i++)
			{
				chars_to_display_buffer(i, &names[i][0], 10, 3);
				string_to_display_buffer(i, &score_chars[i][0], 40);	
			}
			string_to_display_buffer(3, "BACK", 10);
			pointer_to_display_buffer(3, alignment);	// lägger pointern på "BACK"
			display_buffer_to_display();
			
			if((getbtns() & 0x4))	// loopar om till början av highcsore
			{
				while(getbtns() & 0x4){}
				break;
			}	
		}
	}
}
