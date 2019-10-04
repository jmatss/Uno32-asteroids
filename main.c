#include <stdint.h>
#include <pic32mx.h>
#include <stdbool.h>
#include "header.h"

// default(inkl. 0)=start, 1=startaspel, 2=spelet, 3=efterspel, 4=highscore
// return_info innehåller return information som man fått från dom olika sidorna
uint32_t return_info = 0;

int main() 
{
	init();
	
	return_info |= check_saved() << 3;	// kollar om saved(=1) eller ej(=0) och lägger in på rätt position
	
	while(1)
	{
		switch(return_info & 0x7)	// maskar bitar 2-0(sidnummret)
		{
			case(1) :;	// startaspel.c
				startaspel(&return_info);
				break;
			case(2) :;	// spelet.c
				spelet(&return_info);
				break;
			case(3) :;	// efterspel.c
				efterspel(&return_info);
				break;
			case(4) :;	// highscore.c
				highscore(&return_info);
				break;
			default :;	// start.c (använder 0)
				start(&return_info);
		}
	}
	return 0;
}
