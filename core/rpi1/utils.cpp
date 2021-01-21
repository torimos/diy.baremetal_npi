#include "utils.h"

volatile unsigned int tim;
void delay_us(uint32_t us)
{
    // fake delay obviously, should be replaced by systimer
    for(tim = 0; tim < us*5; tim++)
            ;
}

void delay(uint32_t us)
{
    delay_us(us*1000);
}
