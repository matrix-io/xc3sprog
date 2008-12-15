# Spartan3 JTAG programmer and other utilities

# Copyright (C) 2004 Andrew Rogers

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

PREFIX?=/usr/local
DEVLIST=devlist.txt
DEVLIST_INSTALL=share/XC3Sprog
DEFS=-DDEVICEDB=\"${PREFIX}/${DEVLIST_INSTALL}/${DEVLIST}\"
PROGS=detectchain debug bitparse xc3sprog
GCC=g++ -g -Wall -I/usr/local/include/ `libftdi-config --cflags`
LIBS=-lstdc++ `libftdi-config --libs`

all: ${PROGS}

install: all
	install ${PROGS} ${PREFIX}/bin
	install -d ${PREFIX}/${DEVLIST_INSTALL}
	install ${DEVLIST} ${PREFIX}/${DEVLIST_INSTALL}

debug: debug.o iobase.o ioftdi.o ioparport.o iodebug.o
	${GCC} ${LIBS} $^ -o $@

bitparse: bitparse.o bitfile.o
	${GCC} ${LIBS} $^ -o $@

detectchain: detectchain.o jtag.o iobase.o ioftdi.o ioparport.o iodebug.o devicedb.o
	${GCC} ${LIBS} $^ -o $@

xc3sprog: xc3sprog.o jtag.o iobase.o ioftdi.o ioparport.o iodebug.o bitfile.o devicedb.o progalgxcf.o progalgxc3s.o
	${GCC} ${LIBS} $^ -o $@

debug.o: debug.cpp iobase.h ioftdi.h iodebug.h
	${GCC} ${DEFS} -c $< -o $@

bitparse.o: bitparse.cpp bitfile.h
	${GCC} ${DEFS} -c $< -o $@

detectchain.o: detectchain.cpp iobase.h ioftdi.h jtag.h iodebug.h devicedb.h
	${GCC} ${DEFS} -c $< -o $@

xc3sprog.o: xc3sprog.cpp iobase.h ioftdi.h jtag.h iodebug.h bitfile.h devicedb.h progalgxcf.h progalgxc3s.h
	${GCC} ${DEFS} -c $< -o $@

iobase.o: iobase.cpp iobase.h
	${GCC} ${DEFS} -c $< -o $@

iodebug.o: iodebug.cpp iodebug.h iobase.h
	${GCC} ${DEFS} -c $< -o $@

ioftdi.o: ioftdi.cpp ioftdi.h iobase.h
	${GCC} ${DEFS} -c $< -o $@ 

ioparport.o: ioparport.cpp ioparport.h iobase.h
	${GCC} ${DEFS} -c $< -o $@ 

bitfile.o: bitfile.cpp bitfile.h
	${GCC} ${DEFS} -c $< -o $@

jtag.o: jtag.cpp jtag.h
	${GCC} ${DEFS} -c $< -o $@

devicedb.o: devicedb.cpp devicedb.h
	${GCC} ${DEFS} -c $< -o $@

progalgxcf.o: progalgxcf.cpp progalgxcf.h iobase.h jtag.h bitfile.h
	${GCC} ${DEFS} -c $< -o $@

progalgxc3s.o: progalgxc3s.cpp progalgxc3s.h iobase.h jtag.h bitfile.h
	${GCC} ${DEFS} -c $< -o $@

clean:
	rm -f debug.o iobase.o ioftdi.o ioparport.o iodebug.o bitfile.o jtag.o xc3sprog.o 
	rm -f devicedb.o bitparse.o detectchain.o progalgxcf.o progalgxc3s.o
	rm -f debug bitparse detectchain xc3sprog
