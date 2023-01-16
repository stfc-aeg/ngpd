
CC = gcc
CFLAGS = -fPIC -g -Wall -fmessage-length=0 -DLINUX

VAR =  $(shell $(CXX) -dumpmachine)
PLATFORM = $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')
INCLUDES =  ../include/ngpd.h ../include/ngpd_cal.h

OBJ_DIR= objs.$(PLATFORM)
BIN_DIR = ../../../bin.$(PLATFORM)
OBJS =	$(OBJ_DIR)/ngpd_cal_setup.o $(OBJ_DIR)/ngpd_cal_offsets.o 
INCLUDE_DIRS =	-I../include  -I../../../libs/include 
CFLAGS += $(INCLUDE_DIRS)
LIB_DIR = ../../../libs/libs.linux.$(PLATFORM)/lib
LIB = $(OBJ_DIR)/libngpd_cal.a

$(LIB): $(OBJS)
	gcc -shared -o ${OBJ_DIR}/libngpd.so.1.0.0 -fPIC -Wl,-soname,libngpd_cal.so.1.0 $(OBJS)
	rm -f $@
	ar rc $@ $(OBJS)
	ranlib $@

all: $(LIB)
	
install: $(LIB)
	cp ../include/ngpd_cal.h  ../../../libs/include
	cp $(LIB) ../../../libs/libs.linux.$(PLATFORM)/lib
	cp ${OBJ_DIR}/libngpd_cal.so.1.0.0 ../../../libs/libs.linux.$(PLATFORM)/lib
	(cd ../../../libs/libs.linux.$(PLATFORM)/lib; rm -f libngpd_cal.so.1.0; rm -f libngpd_cal.so)
	(cd ../../../libs/libs.linux.$(PLATFORM)/lib; ln -s libngpd_cal.so.1.0.0  libngpd_cal.so.1.0; ln -s libngpd_cal.so.1.0 libngpd_cal.so)

clean:
	rm -f $(OBJS) $(LIB) ${OBJ_DIR}/libngpd_cal.so.1.0.0


#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
		
