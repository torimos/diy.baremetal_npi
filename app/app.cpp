#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#if defined( NANOPIM3 )
#define GPIO_BASE   0xC001B000
#define LED_PIN     12
#elif defined( RPI0 ) || defined( RPI1 )
    #define GPIO_BASE       0x20200000UL
    #define LED_PIN     16
#elif defined( RPI2 ) || defined( RPI3 )
    #define GPIO_BASE       0x3F200000UL
    #define LED_PIN     16
#elif defined( RPI4 )
    #define GPIO_BASE       0xFE200000UL
    #define LED_PIN     16
#else
    #error Unknown RPI Model!
#endif

volatile unsigned int* gpio = (unsigned int*)GPIO_BASE;

void mem_test()
{
    int loop;
    unsigned int* buffer;
    buffer = (unsigned int*)malloc( 1024 * sizeof( unsigned int ) );
    for( loop=0; loop<1024; loop++ )
        buffer[loop] = 0;
    free(buffer);
}

extern "C" void app_main()
{
    // GPIO init for state LED
    #if defined( NANOPIM3 )
    gpio[8] = (gpio[8] & ~(3 << 24)) | (2 << 24);
    gpio[1] = 1 << LED_PIN;
    #else
    gpio[1] |= (1 << 18); 
    #endif

	mem_test();
	while (1) {
        
        #if defined( NANOPIM3 )
        gpio[0] |= (1 << LED_PIN); // Turn LED on
        delay(500);
        gpio[0] &= ~(1 << LED_PIN); // Turn LED off
        delay(500);
        #else
        gpio[7] |= (1 << LED_PIN); // Turn LED on
        delay(500);
        gpio[10] |= (1 << LED_PIN); // Turn LED off
        delay(500);
        #endif
	}
}