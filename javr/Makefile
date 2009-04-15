#
#  Makefile for javr
#
#  $Id: makefile,v 1.5 2003/09/28 14:31:04 anton Exp $
#  $Log: makefile,v $
#  Revision 1.5  2003/09/28 14:31:04  anton
#  Added --help command, display GPL
#
#  Revision 1.4  2003/09/28 12:07:49  anton
#  Verify Fixed
#
#  Revision 1.3  2003/09/27 19:38:02  anton
#  makefile works on Linux
#
#  Revision 1.2  2003/09/27 19:21:59  anton
#  Added support for Linux compile
#
#  Revision 1.1.1.1  2003/09/24 15:35:57  anton
#  Initial import into CVS
#

# TARGET = CYGWIN to compile under cygwin
# TARGET = MINGW to compile under mingw.
# TARGET = LINUX to compile under linux

#	TARGET	= CYGWIN
	TARGET	= LINUX
#	TARGET	= MINGW


ifeq ($(TARGET),CYGWIN)
	TOOLPATH	= /usr/bin/
	EXE	= .exe
else
ifeq ($(TARGET),LINUX)
	TOOLPATH	=
	EXE	=
else
	TOOLPATH	= c:/MinGW/bin/
	EXE	= .exe
endif
endif
	CC	= $(TOOLPATH)g++
	AS	= $(TOOLPATH)g++ -x assembler-with-cpp	
	RM	= $(TOOLPATH)rm -f
	RN	= $(TOOLPATH)mv
	BIN	= $(TOOLPATH)objcopy
	ELFCOF  = $(TOOLPATH)elfcoff
	SIZE	= $(TOOLPATH)size
	INCDIR	= /spare/bon/jtagprog/XC3Sprog
	LIBDIR	= $(GCC)/lib/gcc-lib

#compiler flags
#	CPFLAGS	= -g -O2 -Wall  -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst)
#	CPFLAGS	=  -Os -Wall  -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst)
	CPFLAGS	=  -g -Wall


#assembler flags
	ASFLAGS = -gstabs

#linker flags
	LDFLAGS = -Wl,-Map=$(TRG).map,--cref



#put the name of the target file here (without extension)
	TRG	= javr

#put your C sourcefiles here
	SRC	= $(TRG).c  srecdec.c  command.c  parse.c  menu.c  avr_jtag.c \
       fuse.c jtag.c

#put additional assembler source file here
	ASRC    =

#additional libraries and object files to link (Use -l<libname>
ifeq ($(TARGET),CYGWIN)
	LIB = -lioperm
else
	LIB = -lstdc++ -lftdi
endif

#additional includes to compile
	INC	=


#define all project specific object files
	OBJ	= $(ASRC:.s=.o) $(SRC:.c=.o)
	JOBJ	= /spare/bon/jtagprog/XC3Sprog/ioparport.o /spare/bon/jtagprog/XC3Sprog/io_ft2232.o /spare/bon/jtagprog/XC3Sprog/io_jtag_bitbang.o /spare/bon/jtagprog/XC3Sprog/io_jtag_mpsse.o /spare/bon/jtagprog/XC3Sprog/jtag.o /spare/bon/jtagprog/XC3Sprog/devicedb.o

#this defines the aims of the make process
all:	link

$(TRG)$(EXE):  $(OBJ) $(JOBJ)


#compile: instructions to create assembler and/or object files from C source
%o : %c
#	$(CC) -c $(CPFLAGS) -I$(INCDIR) $< -o $@
	$(CC) -c $(CPFLAGS) -I$(INCDIR)  $< -o $@

%s : %c
	$(CC) -S $(CPFLAGS) -I$(INCDIR) $< -o $@


#assemble: instructions to create object file from assembler files
%o : %s
	$(AS) -c $(ASFLAGS) -I$(INCDIR) $< -o $@


#link: instructions to create executable output file from object files
link: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(LDFLAGS) $(JOBJ)  -o $(TRG)$(EXE)



#If all other steps compile ok then echo "Errors: none".
#Necessary for AVR Studio to understand that everything went ok.
%ok:
	@echo "Errors: none"

#make instruction to delete created files
clean:
	$(RM) $(OBJ)
	$(RM) $(SRC:.c=.s)
	$(RM) $(SRC:.c=.lst)
	$(RM) $(TRG).map
	$(RM) $(TRG).o
	$(RM) $(TRG)$(EXE)
	$(RM) *.bak
	$(RM) *.log

size:
	$(SIZE) $(TRG).elf

	DEP = makefile javr.h
###### dependecies, add any dependencies you need here ###################
$(TRG).o   : $(TRG).c $(TRG).h  $(DEP)
srecdec.o  : srecdec.c $(DEP)
command.o  : command.c $(DEP)
parse.o    : parse.c $(DEP)
menu.o     : menu.c  $(DEP)
avr_jtag.o : avr_jtag.c $(DEP)
fuse.o     : fuse.c fuse.h $(DEP)


FORCE:


# Listing of phony targets.
.PHONY : all link

