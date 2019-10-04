#include <stdint.h>
#include <pic32mx.h>
#include <stdbool.h>
#include "header.h"

void start( uint32_t* return_info )
{	
	/* START INIT */
    int alignment = 4, current = 0, max = 1;    // alignment = pixel alignment of pointer, current = pointer starts at line 0, max = total number of menu options - 1
    bool saved = check_saved();			// kollar om man har ett sparat spel i minnet
	if (saved)
	{
		*return_info |= 1 << 3;		// sätter saved bit till 1
	}
	
    while(1)
    {
        clear_display_buffer();
		
        if ( saved )
		{
			string_to_display_buffer( 2, "RESUME GAME", 10 );
			max = 2;
		}
		
		string_to_display_buffer( 0, "NEW GAME", 10 );
		string_to_display_buffer( 1, "HIGH SCORES", 10 );
		string_to_display_buffer( 3, "KNAPP2 - EFTERSPEL", 10 );
		pointer_to_display_buffer( current, alignment );
		display_buffer_to_display();
		menu( &current, max );
		
		if( getbtns() & 0x4 )
		{
			switch( current )		// översätter från current till sidnr
			{
				case( 0 ):;
					*return_info = ( *return_info & ~0x7 ) | 0x1;	// 1 = startaspel
				break;
				case( 1 ):;
					*return_info = ( *return_info & ~0x7 ) | 0x4;	// 4 = highscore
				break;
				case( 2 ):;
					// saved satt sen tidigare i return_info, så spelet ser att det är sparat
					*return_info = ( *return_info & ~0x7 ) | 0x2;	// 2 = spelet
				break;
			}
			while( getbtns() & 0x4){}	// ligger kvar tills knappen släpps
			return;
		}
		
		// ANVÄNDS FÖR TEST !!!!!!!!!!!!!!!!!!!
		if( getbtns() & 0x2 )
		{	
			uint16_t score = 1;
			*return_info = 0;	// rensar för test
			*return_info |= (0 << 20) | (score << 4) | 0x3;	// 3 = efterspelet, tar bort saved också, score+difficulty
			while( getbtns() & 0x2){}	// ligger kvar tills knappen släpps
			return;
		}	
	}
}
