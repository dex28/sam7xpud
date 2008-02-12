
Q = @

VER_MAJOR = 0
VER_MINOR = 3

#-------------------------------------------------------------------------------
#       Build List
#-------------------------------------------------------------------------------

.DEFAULT_GOAL = all

VPATHOBJS = obj
OBJ = $(VPATHOBJS)/

#-------------------------------------------------------------------------------
# XPUD Part

VPATHSRCS = src:src/lib:src/rtos:src/fpga
INC = -I . -I src/inc -I lib/$(TARGET)

thumb_objects = \
    $(OBJ)startup.o $(OBJ)device.o \
    $(OBJ)sam7xpud.o $(OBJ)libcxa.o $(OBJ)stdio.o \
    $(OBJ)usbTasks.o $(OBJ)timerTasks.o \
    $(OBJ)xsvfTask.o $(OBJ)xsvfPlayer.o $(OBJ)fpga.o $(OBJ)xpi.o

arm_objects =

#-------------------------------------------------------------------------------
# USB Framework

VPATHSRCS := $(VPATHSRCS):src/usb

thumb_objects += \
    $(OBJ)usbUDP.o $(OBJ)usbSTD.o $(OBJ)usbCDC.o \
    $(OBJ)usbCallbacks.o

#-------------------------------------------------------------------------------
# FreeRTOS part

VPATHSRCS := $(VPATHSRCS):FreeRTOS:FreeRTOS/portable:FreeRTOS/portable/heap
INC += -I FreeRTOS/include -I FreeRTOS/portable

thumb_objects += \
	$(OBJ)tasks.o $(OBJ)croutine.o $(OBJ)list.o $(OBJ)queue.o $(OBJ)port.o \
	$(OBJ)heap_1.o $(OBJ)sema.o
	
arm_objects += \
	$(OBJ)portISR.o $(OBJ)ISR.o
	
#-------------------------------------------------------------------------------

build_list = $(OBJ)sam7xpud.bin
clean_list = $(build_list)
clean_list2 = $(OBJ)*.o $(OBJ)*.d
clean_list3 = $(OBJ)*.map $(OBJ)*.lss $(OBJ)*.elf $(OBJ)*.bin

#-------------------------------------------------------------------------------

vpath %.h    $(VPATHSRCS)
vpath %.c    $(VPATHSRCS)
vpath %.s    $(VPATHSRCS)
vpath %.cpp  $(VPATHSRCS)

vpath %.o    $(VPATHOBJS)
vpath %.elf  $(VPATHOBJS)
vpath %.bin  $(VPATHOBJS)

$(OBJ)sam7xpud.elf : $(thumb_objects) $(arm_objects)

$(OBJ)sam7xpud.bin : $(OBJ)sam7xpud.elf

-include $(thumb_objects:.o=.d)
-include $(arm_objects:.o=.d)

program : $(OBJ)sam7xpud.bin
	@$(if $(Q), echo "  PRGRM  " $<, echo "" )
	@$(if $(Q), echo "" )
	$(Q)cd ocd && openocd-ftd2xx.exe -d0 -f "at91sam7s256-jtagkey-flash-program.cfg"
	@echo ""
	@echo "*** Flash programming successfully completed. ***"
	@echo ""

unlockflash :
	@$(if $(Q), echo "Unlocking flash..." )
	$(Q)cd ocd && openocd-ftd2xx.exe -d2 -f "at91sam7s256-jtagkey-flash-unlock.cfg"
	@echo ""
	@echo "*** Flash programming successfully completed. ***"
	@echo ""

elf_size: all
	@echo ""
	$(Q)$(SIZE) -A $(OBJ)sam7xpud.elf

doxygen: 
	doxygen doxy.rc

#-------------------------------------------------------------------------------
#       Makefile template
#-------------------------------------------------------------------------------

include Makefile.mk
include Version.mk
