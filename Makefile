#
# Makefile
#

CIRCLEHOME = circle

MEM_TYPE ?= 2
SPI_FREQ ?= 3120000

CPPFLAGS += -DMEM_TYPE=$(MEM_TYPE) -DSPI_FREQ=$(SPI_FREQ)

OBJS      = main.o kernel.o spi_memory.o mt19937ar.o

LIBS      = $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
            $(CIRCLEHOME)/addon/Properties/libproperties.a \
            $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
            $(CIRCLEHOME)/lib/fs/libfs.a \
            $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/app/Rules.mk

-include $(DEPS)

boot: kernel8.img
	cp -R circle/boot/* boot/
	cp kernel*.img boot/
	cp boot/config64.txt boot/config.txt
