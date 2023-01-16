TARGET = ngzmp

PLATFORM = arm64
CROSS_COMPILE = aarch64-linux-gnu-

CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -fmessage-length=0 -std=gnu99

INCLUDES =  ../include/ngzmp_driver.h ../../include/ngzmp.h ../../include/ngzmp_dma_protocol.h


OBJ_DIR= objs.$(PLATFORM)
BIN_DIR = ../../../../bin.$(PLATFORM)

OBJS = $(OBJ_DIR)/zynqmp_lib_driver.o $(OBJ_DIR)/ngzmp_lib.o $(OBJ_DIR)/ngzmp_lib_spi.o $(OBJ_DIR)/zynqmp_lib_i2c.o $(OBJ_DIR)/ngzmp_clock.o  $(OBJ_DIR)/ngzmp_adc.o $(OBJ_DIR)/zynqmp_lib_ams.o

#xspress4_lib_config.o    $(OBJ_DIR)/xspress4_lib_xadc.o

INCLUDE_DIRS =	-I../include -I../../include

CFLAGS += $(INCLUDE_DIRS)
CFLAGS += -I../../../../libs/include
LIB_DIR = ../../../../libs/libs.linux.$(PLATFORM)/lib
LIB = $(OBJ_DIR)/lib$(TARGET).a

$(LIB): $(OBJS)
	rm -f $@
	$(CROSS_COMPILE)ar rc $@ $(OBJS)
	$(CROSS_COMPILE)ranlib $@

all: $(LIB)

install: $(LIB)
	(cd ../include; chmod 644 *.h; cp *.h ../../../../libs/include)
	chmod 644 $(LIB)
	cp $(LIB) ../../../../libs/libs.linux.$(PLATFORM)/lib

clean:
	rm -f $(OBJS) $(LIB)

#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

	