#include <stdint.h>
#include <pic32mx.h>
#include "header.h"

void startaspel(uint32_t* return_info)
{
	// alignment = pixel alignment of pointer, current = pointer starts at line 0, max = total number of menu options - 1
	int alignment = 4, current = 0, max = 2;
	
	while(1)
	{
		clear_display_buffer();
		
		string_to_display_buffer( 0, "HARD", 10 );
		string_to_display_buffer( 1, "MEDIUM", 10 );
		string_to_display_buffer( 2, "EASY", 10 );
		pointer_to_display_buffer( current, alignment );
		display_buffer_to_display();
		menu( &current, max );
		
		if( getbtns() & 0x4 )
		{
			*return_info = (*return_info & ~0x7) | 0x2;	// 2 = spelet
			*return_info &= ~(0x3 << 20);			// rensar difficulty fältet
			*return_info |= current << 20;			// sätter difficulty till current(H=0,M=1,E=2)
			*return_info &= ~0x8;					// saved bit = 0
			clear_saved();
			while( getbtns() );				// ligger kvar tills knappen släpps
			return;
		}		
}
}
