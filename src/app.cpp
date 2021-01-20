#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils.h>

#define GPIO_BASE   0xC001B000
#define LED_PIN     12
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
    gpio[8] = (gpio[8] & ~(3 << 24)) | (2 << 24);
    gpio[1] = 1 << LED_PIN;
	mem_test();
	while (1) {
        
        gpio[0] |= 1 << LED_PIN; // Turn LED on
        delay(500);
        gpio[0] &= ~(1 << LED_PIN); // Turn LED off
        delay(500);
	}
}