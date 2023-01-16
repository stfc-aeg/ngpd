
CC = gcc
CFLAGS = -fPIC -g -Wall -fmessage-length=0 -D__LINUX__

VAR ?=  $(shell $(CXX) -dumpmachine)
PLATFORM ?= $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')
INCLUDES =  ../include/ngpd.h ../include/ngpd_cal.h

OBJ_DIR ?= objs.$(PLATFORM)
BIN_DIR ?= ../../../bin.$(PLATFORM)
OBJS =	$(OBJ_DIR)/ngpd_cal_setup.o $(OBJ_DIR)/ngpd_cal_offsets.o 
INCLUDE_DIRS =	-I../include  -I../../../libs/include 
CFLAGS += $(INCLUDE_DIRS)
LIB_DIR ?= ../../../libs/libs.linux.$(PLATFORM)/lib
LIB = $(OBJ_DIR)/libngpd_cal.a

$(LIB): $(OBJS)
ifdef MAKE_SHARED
	gcc -shared -o ${OBJ_DIR}/libngpd_cal.so.1.0.0 -fPIC -Wl,-soname,libngpd_cal.so.1.0 $(OBJS)
endif
	rm -f $@
	ar rc $@ $(OBJS)
	ranlib $@

all: $(LIB)
	
install: $(LIB)
	cp ../include/ngpd_cal.h  ../../../libs/include
	cp $(LIB) $(LIB_DIR)
ifdef MAKE_SHARED
	cp ${OBJ_DIR}/libngpd_cal.so.1.0.0 $(LIB_DIR)
	(cd $(LIB_DIR); rm -f libngpd_cal.so.1.0; rm -f libngpd_cal.so)
	(cd $(LIB_DIR); ln -s libngpd_cal.so.1.0.0  libngpd_cal.so.1.0; ln -s libngpd_cal.so.1.0 libngpd_cal.so)
endif
clean:
	rm -f $(OBJS) $(LIB) ${OBJ_DIR}/libngpd_cal.so.1.0.0


#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
		
