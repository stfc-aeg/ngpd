TARGET = ngzmptest

CC = gcc
CFLAGS = -O3 -g -Wall -fmessage-length=0 -DLINUX

VAR =  $(shell $(CXX) -dumpmachine)
PLATFORM = $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')

OBJ_DIR= objs.$(PLATFORM)
OBJS =	 $(OBJ_DIR)/ngzmptest.o $(OBJ_DIR)/ngpd_args.o $(OBJ_DIR)/reg_test.o  $(OBJ_DIR)/bram_test.o 
OBJS += $(OBJ_DIR)/spi_reg_test.o $(OBJ_DIR)/i2c_tests.o
# $(OBJ_DIR)/dma_test.o
#OBJS += $(OBJ_DIR)/speed_test.o $(OBJ_DIR)/udp_tx.o $(OBJ_DIR)/scope_test_pat.o $(OBJ_DIR)/spi.o
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

$(OBJ_DIR)/reg_test.o: ../zynqmp/test/reg_test.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/bram_test.o: ../zynqmp/test/bram_test.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/spi_reg_test.o: ../zynqmp/test/spi_reg_test.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/i2c_tests.o: ../zynqmp/test/i2c_tests.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
