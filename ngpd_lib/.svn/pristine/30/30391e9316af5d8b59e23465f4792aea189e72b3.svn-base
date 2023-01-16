TARGET = xspress3test

CC = gcc
CFLAGS = -O3 -g -Wall -fmessage-length=0

VAR =  $(shell $(CXX) -dumpmachine)
PLATFORM = $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')

OBJ_DIR= objs.$(PLATFORM)
OBJS =	$(OBJ_DIR)/xspress3_test.o $(OBJ_DIR)/x3_args.o $(OBJ_DIR)/bram_test.o $(OBJ_DIR)/reg_test.o $(OBJ_DIR)/speed_test.o $(OBJ_DIR)/udp_tx.o $(OBJ_DIR)/scope_test_pat.o $(OBJ_DIR)/spi.o
OBJS +=	$(OBJ_DIR)/read_hist_test.o $(OBJ_DIR)/clear_hist_test.o $(OBJ_DIR)/hist_speed.o 
OBJS +=	$(OBJ_DIR)/scaler_read_test.o 
INCLUDES =  xspress3test.h

INCLUDE_DIRS = -I../include  -I../../../libs/include
CFLAGS += $(INCLUDE_DIRS)

LIBDIRS = -L../lib/objs.$(PLATFORM) 
LIBDIRS += -L../../../libs/libs.linux.$(PLATFORM)/lib
LIBS = -lxspress3 -ltutils -limg_mod -lrt -lm

LFLAGS =  $(LIBDIRS) $(LIBS)

BIN_DIR= ../bin.$(PLATFORM)

$(TARGET): $(OBJS)
	$(CC) $(OBJS)  -o $(BIN_DIR)/$(TARGET) $(LFLAGS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

threads: threads.o
	$(CC) threads.o -o $(BIN_DIR)/threads $(LFLAGS)

#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
