# 
# Makefile for libarcp.  Currently it is not essential to utilise this
# Makefile since a user can always integrate the .c file into their own
# build framework. However, in future this may not always be the case, so
# users are encouraged to use "make" to compile the library here, and then
# utilise arcp-config within individual projects.
#
# The default action is to compile natively (that is, for the i386 linux
# target).  This can be altered by passing an option on the command line:
#   make ARCH=i386-linux      Compile for Linux on an i386 processor
#   make ARCH=avr-nutos       Compile for NutOS on an Atmel AVR processor

CC = gcc
CFLAGS_I386_LINUX = -Wall -g
CFLAGS_AVR_NUTOS = -DARCP_NUTOS
CFLAGS_I386_WIN32 = -DARCP_WIN32 -Wall -O2
LFLAGS =

# The following settings are used when cross-compiling libarcp for AVR/NutOS
#
# The path to the top-level NutOS source tree
# NUTOS_PATH = /home/hoopy/jwoithe/avr/ethernut-3.9.8
NUTOS_PATH = /home/hoopy/jwoithe/avr/ethernut-4.4.1
#
# The path to the AVR cross-compiler tools
AVR_TOOL_PATH = /home/hoopy/jwoithe/avr/tools/bin

# The following settings apply when cross-compiling libarcp for win32
#
# The path to the win32 cross-compiler tools
WIN32_TOOL_PATH = /home/hoopy/jwoithe/xcompile/bin

############################################################################
# Nothing else should need changing below.
############################################################################

MODULES = arcp.o

# Set up the default target
ifndef ARCH
  ARCH=i386-linux
endif

ifeq ($(ARCH), i386-linux)
  CC = gcc
  CFLAGS = $(CFLAGS_I386_LINUX)
else
ifeq ($(ARCH), avr-nutos)
  # NutOS related setup.  This was modelled on nutapp/Makedefs from NutOS
  # 3.9.8.  Unfortunately we can't include this directly because it assumes
  # compilation occurs from within the nutapp tree and uses relative paths
  # from that.  Some of this may need adjustment for later versions of NutOS.
  top_srcdir = $(NUTOS_PATH)
  top_appdir = $(NUTOS_PATH)/nutapp
  LIBDIR = $(NUTOS_PATH)/nutbld/lib
  INCFIRST = -I$(NUTOS_PATH)/nutbld/include
  include $(top_appdir)/NutConf.mk
  include $(top_srcdir)/app/Makedefs.avr-gcc

  CFLAGS = $(CPFLAGS) $(INCFIRST) -I$(INCDIR) $(INCLAST) $(CFLAGS_AVR_NUTOS)
  # Override the CC setting provided by NutOS configuration to take into
  # account the full pathname.  This way avr-gcc doesn't need to be in the
  # path.
  CC = $(AVR_TOOL_PATH)/avr-gcc
else
ifeq ($(ARCH), i386-win32)
  CC = $(WIN32_TOOL_PATH)/gcc-win32
  CFLAGS = $(CFLAGS_I386_WIN32)
else
  $(error Unknown architecture $(ARCH) specified)
endif
endif
endif
 
OBJDIR=$(ARCH)

default:	$(OBJDIR)/arcp.o

all:	
	$(MAKE) ARCH=i386-linux
	$(MAKE) ARCH=avr-nutos
	$(MAKE) ARCH=i386-win32

# Intermediate C files
#
# Move the generated .lst file into the object directory if it exists.  This
# is necessary because the CPFLAGS set up by the NutOS environment is a
# recursive variable and there's no easy way to edit them via $(...:a=b)
# Makefile substitutions or sed in a $(shell ...) reference.  No, 
# $(value ...) doesn't seem to help.

$(OBJDIR)/%.o:	%.c
	$(CC) -o $@ -c $< $(CFLAGS)
	[ ! -f $(<F:.c=.lst) ] || mv $(<F:.c=.lst) $(OBJDIR)/

# Janitorial rules
clean:	
	rm -rf *.o arcptalk *~ core */*.o */*.lst
tidy:	
	rm -rf *~ core

# Extra dependancies
$(OBJDIR)/arcp.o:		arcp.h
