extern int __bss_start__;
extern int __bss_end__;

extern void app_main( void );

void kernel_main( unsigned int r0, unsigned int r1, unsigned int atags )
{
    int* bss = &__bss_start__;
    int* bss_end = &__bss_end__;
    while( bss < bss_end )
        *bss++ = 0;
    app_main();
    while(1)
    {
    }
}