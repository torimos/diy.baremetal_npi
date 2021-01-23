HOME := $(shell pwd)

CROSS_COMPILE=arm-none-eabi-

all: clean
	@$(CROSS_COMPILE)gcc -c $(HOME)/preloader.S -o $(HOME)/nsih.S.o
	@$(CROSS_COMPILE)ld -T$(HOME)/nsih.lds $(HOME)/nsih.S.o -o $(HOME)/nsih.elf
	@$(CROSS_COMPILE)objcopy -Obinary $(HOME)/nsih.elf $(HOME)/nsih.bin
	@$(CROSS_COMPILE)objdump -h -d -S $(HOME)/nsih.elf > $(HOME)/nsih.lst

clean:
	@rm -f *.o *.elf *.bin *.lst