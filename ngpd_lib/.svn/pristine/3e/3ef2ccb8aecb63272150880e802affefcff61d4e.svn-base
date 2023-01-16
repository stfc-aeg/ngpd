TARGET = ngpdtest

CC = gcc
CFLAGS = -O3 -g -Wall -fmessage-length=0 -DLINUX

VAR =  $(shell $(CXX) -dumpmachine)
PLATFORM = $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')

OBJ_DIR= objs.$(PLATFORM)
OBJS =	 $(OBJ_DIR)/ngpdtest.o $(OBJ_DIR)/ngpd_args.o $(OBJ_DIR)/reg_test.o $(OBJ_DIR)/dma_test.o
#OBJS += $(OBJ_DIR)/bram_test.o  $(OBJ_DIR)/speed_test.o $(OBJ_DIR)/udp_tx.o $(OBJ_DIR)/scope_test_pat.o $(OBJ_DIR)/spi.o
INCLUDES =  ngpdtest.h

INCLUDE_DIRS = -I../include  -I../../../libs/include
CFLAGS += $(INCLUDE_DIRS)

LIBDIRS = -L../lib/objs.$(PLATFORM) 
LIBDIRS += -L../../../libs/libs.linux.$(PLATFORM)/lib
LIBS = -lngpd 
LIBS += $(shell pkg-config --libs libadq)
LIBS += -ltutils -limg_mod -lrt -lpthread -lm

LFLAGS =  $(LIBDIRS) $(LIBS)

BIN_DIR= ../../../bin.$(PLATFORM)

$(TARGET): $(OBJS)
	$(CC) $(OBJS)  -o $(BIN_DIR)/$(TARGET) $(LFLAGS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
