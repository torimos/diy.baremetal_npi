OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
	RAM(rxw) : ORIGIN = 0, LENGTH = 508,
    NSIH(r) : ORIGIN = 0x1fc, LENGTH = 4
}

SECTIONS
{
	. = ALIGN(4);
	.text :
	{
		*(.text)
	} > RAM
	
	.marker :
    {
        /* SIGNATURE - 0x4e534948 - NSIH; */
        BYTE(0x4e)
        BYTE(0x53)
        BYTE(0x49)
        BYTE(0x48)
    } > NSIH
	
}
