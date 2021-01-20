#include <string.h>
#include <stdlib.h>

extern void app_main( void );

void boot_master(void)
{
	app_main();
}

void boot_slave(void)
{
}
