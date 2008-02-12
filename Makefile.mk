
#-------------------------------------------------------------------------------
#       Parameters
#-------------------------------------------------------------------------------
#   TARGET:     target chip         (default: TARGET=AT91SAM7S64)
#   BOARD:      board used          (default: BOARD=AT91SAM7SEK)
#   REVISION:   Board revision      (default: REVISION=REV_B)
#   REMAP:      Run from SRAM       (default: REMAP=NO)
#   DEBUG:      Debug symbols       (default: DEBUG=NO)
#   LEDS:       Use leds            (default: LEDS=YES)
#   POWER:      Self/bus powered    (default: POWER=SELF)

TARGET    = AT91SAM7S256
BOARD     = AT91SAM7SEK
REMAP     = NO
DEBUG     = NO
LEDS      = YES
POWER     = BUS

#-------------------------------------------------------------------------------
#       Cross compiler target
#-------------------------------------------------------------------------------

CROSS=arm-elf-

#-------------------------------------------------------------------------------
#       Check target
#-------------------------------------------------------------------------------

VALIDTARGETS_ARM7 = \
  AT91SAM7S321 AT91SAM7S64   AT91SAM7S128 AT91SAM7S256 AT91SAM7S512 \
  AT91SAM7SE32 AT91SAM7SE256 AT91SAM7SE512 \
  AT91SAM7X128 AT91SAM7X256  AT91SAM7X512  \
  AT91SAM7A3

VALIDTARGETS_ARM9 =  AT91RM9200   AT91SAM9260  AT91SAM9261   AT91SAM9263

VALIDTARGETS = $(VALIDTARGETS_ARM7) $(VALIDTARGETS_ARM9)

ifeq (,$(filter $(TARGET), $(VALIDTARGETS)))
$(error Error: $(TARGET): Unknown target.)
endif

ifeq (,$(filter $(TARGET), $(VALIDTARGETS_ARM9)))
MCPU = arm7tdmi
else
#TODO: check this (must read - no AT91 ARM9 here for tests)
MCPU = arm9
endif

#-------------------------------------------------------------------------------
#       Check board
#-------------------------------------------------------------------------------
ifndef BOARD
BOARD = AT91SAM7SEK
endif

#-------------------------------------------------------------------------------
#       Check remap
#-------------------------------------------------------------------------------
ifndef REMAP
REMAP = NO
else
ifneq ($(REMAP),YES)
REMAP = NO
endif
endif

#-------------------------------------------------------------------------------
#       Check debug
#-------------------------------------------------------------------------------
ifndef DEBUG
DEBUG = NO
else
ifneq ($(DEBUG),YES)
DEBUG = NO
endif
endif

#-------------------------------------------------------------------------------
#       Check leds
#-------------------------------------------------------------------------------
ifndef LEDS
LEDS = YES
else
ifneq ($(LEDS),NO)
LEDS = YES
endif
endif

#-------------------------------------------------------------------------------
#       Check power
#-------------------------------------------------------------------------------
ifndef POWER
POWER = SELF
else
ifneq ($(POWER),BUS)
POWER = SELF
endif
endif

#-------------------------------------------------------------------------------
#       Check mode
#-------------------------------------------------------------------------------
ifndef MODE
MODE = NO
endif

#-------------------------------------------------------------------------------
#       Tools
#-------------------------------------------------------------------------------

CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
FE = $(CROSS)objcopy
STRIP = $(CROSS)strip
SIZE = $(CROSS)size
OBJDUMP = $(CROSS)objdump

#-------------------------------------------------------------------------------
#       Debug mode
#-------------------------------------------------------------------------------

ifeq ($(DEBUG),YES)
OPTIMIZATION = -O0 -gdwarf-2 -fno-inline
else
OPTIMIZATION = -O3 -gdwarf-2
OPTIMIZATION += -ffunction-sections -fdata-sections
endif

#-------------------------------------------------------------------------------
#       Compilation flags
#-------------------------------------------------------------------------------

DEFS = -D$(TARGET) -D$(BOARD) 

# FreeRTOS
DEFS += -DGCC_ARM7_ECLIPSE

ifeq ($(LEDS),NO)
DEFS += -DNOLEDS
endif

ifeq ($(POWER),SELF)
DEFS += -DUSB_SELF_POWERED
else
DEFS += -DUSB_BUS_POWERED
endif

ifdef MODE
ifneq ($(MODE),NO)
DEFS += -D$(MODE)
endif
endif

# TODO arm9
ASFLAGS  = -mcpu=$(MCPU) -mthumb -mthumb-interwork -x assembler-with-cpp $(DEFS) $(INC) 

ifeq ($(DEBUG),YES)
ASFLAGS += -gdwarf-2
endif

CCFLAGS = -mcpu=$(MCPU) -mthumb-interwork -DTHUMB_INTERWORK -std=gnu99 -Wall \
   -Wcast-align -Wpointer-arith -Wshadow \
   $(OPTIMIZATION) $(DEFS) $(INC) 

CXXFLAGS = -mcpu=$(MCPU) -mthumb-interwork -DTHUMB_INTERWORK -Wall \
   -Wcast-align -Wpointer-arith -Wshadow \
   $(OPTIMIZATION) $(DEFS) $(INC) \
   -fno-exceptions -fno-rtti 

ifeq ($(DEBUG),NO)
CCFLAGS += -fomit-frame-pointer
CXXFLAGS += -fomit-frame-pointer
endif


#-------------------------------------------------------------------------------
#       Linker flags
#-------------------------------------------------------------------------------

LDFLAGS = -mthumb -mthumb-interwork -nostartfiles 
#LDFLAGS += -nodefaultlibs -nostdlib
LDFLAGS += -Wl,--cref,--gc-sections 
LDFLAGS += -Tlink/$(TARGET).ld 
LDFLAGS += -Tlink/at91-ROM.ld

#-------------------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES: .cpp .c .o .java .class .jar .s .d

%.o: %.cpp

%.o: %.c

%.o: %.s

$(OBJ)%.o: %.cpp
	@$(if $(Q), echo "  C++    " $@, echo "" )
	$(Q)$(CXX) $(if $(filter $@,$(arm_objects)),,-mthumb) $(CXXFLAGS) -o $@ -c $<
	@echo -n "$(OBJ)" > $(OBJ)$*.d
	$(Q)$(CXX) -MM $(CXXFLAGS) $< >> $(OBJ)$*.d
	@echo "" >> $(OBJ)$*.d

$(OBJ)%.o: %.c
	@$(if $(Q), echo "  CC     " $@, echo "" )
	$(Q)$(CC) $(if $(filter $@,$(arm_objects)),,-mthumb) $(CCFLAGS) -o $@ -c $<
	@echo -n "$(OBJ)" > $(OBJ)$*.d
	$(Q)$(CC) -MM $(CCFLAGS) $< >> $(OBJ)$*.d
	@echo "" >> $(OBJ)$*.d

$(OBJ)%.o: %.s
	@$(if $(Q), echo "  AS     " $@, echo "" )
	$(Q)$(CC) $(ASFLAGS) -o $@ -c $< 
	@echo -n "$(OBJ)" > $(OBJ)$*.d
	$(Q)$(CC) -MM $(ASFLAGS) $< >> $(OBJ)$*.d
	@echo "" >> $(OBJ)$*.d

$(OBJ)%.elf $(OBJ)%.map: %.o
	@# VERSION BUILD --------------------------------------------------------------------
	@$(if $(Q), echo "  CC     " $(OBJ)version.o, echo "" )
	$(Q)$(CC) $(if $(filter $@,$(arm_objects)),,-mthumb) $(CCFLAGS) \
 -DVER_BUILD=$(VER_BUILD) \
 -o $(OBJ)version.o -c version.c
	@# ELF ------------------------------------------------------------------------------
	@$(if $(Q), echo "  LD     " $@, echo "" )
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(OBJ)version.o -Wl,-Map=$(basename $@).map
ifdef STRIP_EXE
	@$(if R(Q), echo "  STRIP  " $@ )
	$(Q)$(STRIP) -s --remove-section=.note --remove-section=.comment $@
endif
	@# INCREMENT VERSION BUILD ----------------------------------------------------------
	@echo "VER_BUILD = $$(($(VER_BUILD)+1))" > Version.mk

$(OBJ)%.bin $(OBJ)%.lss: %.elf
	@$(if $(Q), echo "  DUMP   " $(basename $@).lss, echo "" )
	$(Q)$(OBJDUMP) -h -S -C $^ > $(basename $@).lss
	@$(if $(Q), echo "  FE     " $@ )
	$(Q)$(FE) -O binary $^ $@

%.class: %.java
	@$(if $(Q), echo "  GCJ    " $@ )
	$(Q)gcj -C $<

%.jar:
	@$(if $(Q), echo "  JAR    " $@ )
	$(Q)jar cf $@ $^

# --------------------------------------------

ifndef CROSS

.PHONY: all

all: $(build_list)
	@echo ""
	@echo "**** Build Done ****"

else

.PHONY: all

all: report_cross_compiling $(build_list)
	@echo ""
	@echo "**** Build Done ****"

.PHONY: report_cross_compiling

report_cross_compiling:;
	@echo ""
	@echo "  VERSION $(VER_MAJOR).$(VER_MINOR), Build $(VER_BUILD)"
ifndef Q
	@echo "  Target  $(TARGET)/$(MCPU)"
	@echo "  Board   $(BOARD)"
	@echo "  Power   $(POWER)"
	@echo "  Remap   $(REMAP)"
	@echo "  Debug   $(DEBUG)"
	@echo "  LEDs    $(LEDS)"
else
	@echo "  CROSS  " $(CROSS)*
endif

endif

# --------------------------------------------

.PHONY: install

ifdef INSTALLDIR

ifdef Q

define INST_template
	@echo "  COPY   " $(1) "->" $(INSTALLDIR)
	$(Q)cp -a $(1) $(INSTALLDIR)

endef

else # !Q

define INST_template
	cp -a $(1) $(INSTALLDIR)

endef

endif # Q

else # ! INSTALLDIR

define INST_template
	@true

endef

endif # INSTALLDIR
	
install: all
	$(foreach prog, $(build_list), \
	    $(call INST_template, $(prog) ) )

#-------------------------------------------------------------------------------

.PHONY: clean

distclean: clean

clean: FORCE
	@echo ""
	@$(if $(Q), echo "  RM     " $(clean_list) )
	$(Q)rm -rf $(clean_list) 2>/dev/null ; true
ifdef clean_list2
	@$(if $(Q), echo "  RM     " $(clean_list2) )
	$(Q)rm -rf $(clean_list2) 2>/dev/null ; true
endif
ifdef clean_list3
	@$(if $(Q), echo "  RM     " $(clean_list3) )
	$(Q)rm -rf $(clean_list3) 2>/dev/null ; true
endif

#-------------------------------------------------------------------------------

FORCE:

#-------------------------------------------------------------------------------
