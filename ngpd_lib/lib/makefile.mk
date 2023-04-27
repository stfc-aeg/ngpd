CFLAGS ?= -fPIC -g -Wall -fmessage-length=0 -DLINUX

VAR =  $(shell $(CXX) -dumpmachine)
PLATFORM ?= $(shell awk 'BEGIN { print substr("$(VAR)", 1, index("$(VAR)", "-")-1)}')
INCLUDES =  ../include/$(TARGET).h
ifdef NGPD_CONF_ADQ14
CFLAGS += -DNGPD_CONF_ADQ14=1
else
CFLAGS += -DNGPD_CONF_ADQ14=0
endif

OBJ_DIR ?= objs.$(PLATFORM)
TARGET = ngpd

CC ?= gcc


#BIN_DIR ?= ../../../bin.$(PLATFORM)
INSTALL_DIR ?= ../../install
BIN_DIR ?= $(INSTALL_DIR)/bin
LIB_DIR ?= $(INSTALL_DIR)/lib
INC_DIR ?= $(INSTALL_DIR)/include

OBJS =	$(OBJ_DIR)/ngpd_adq14.o $(OBJ_DIR)/ngpd_adq14_regs.o  $(OBJ_DIR)/ngpd_adq14_dma.o $(OBJ_DIR)/ngpd_scope_mode.o $(OBJ_DIR)/ngpd_adq14_scope_regs.o
OBJS += $(OBJ_DIR)/ngpd_playback.o  $(OBJ_DIR)/ngpd_filter.o $(OBJ_DIR)/ngpd_histogram.o  $(OBJ_DIR)/ngpd_adq14_adc.o $(OBJ_DIR)/zynqmp_api.o $(OBJ_DIR)/ngpd_zynqmp.o  $(OBJ_DIR)/ngzmp_hardware_conf.o
OBJS +=  $(OBJ_DIR)/ngzmp_spi.o $(OBJ_DIR)/ngzmp_dma.o $(OBJ_DIR)/ngzmp_hist.o $(OBJ_DIR)/ngzmp_i2c.o $(OBJ_DIR)/ngzmp_clock.o $(OBJ_DIR)/ngzmp_adc.o $(OBJ_DIR)/ngzmp_ams.o
OBJS += $(OBJ_DIR)/ngpd_dummy.o $(OBJ_DIR)/ngpd_python_hooks.o

INCLUDE_DIRS =	-I../include  -I$(INC_DIR) -I../../libadq/include -I../../det_sw_include
CFLAGS += $(INCLUDE_DIRS)
#LIB_DIR ?= ../../../libs/libs.linux.$(PLATFORM)/lib

LIB_NAME ?= lib$(TARGET)
LIB = $(OBJ_DIR)/$(LIB_NAME).a

$(LIB): $(OBJS)
ifdef MAKE_SHARED
	gcc -shared -o $(OBJ_DIR)/${LIB_NAME}.so.1.0.0 -fPIC -Wl,-soname,$(LIB_NAME).so.1.0 $(OBJS)
endif
	rm -f $@
	ar rc $@ $(OBJS)
	ranlib $@

all: $(LIB)
	
install: $(LIB)
#	cp ../include/ngpd.h  ../../../libs/include
#	cp ../include/ngzmp.h  ../../../libs/include
#	cp ../include/ngzmp_dma_protocol.h  ../../../libs/include
#	cp ../include/axi_hist.h  ../../../libs/include
#	cp $(LIB) $(LIB_DIR)
	install -p ../include/ngpd.h  $(INC_DIR)
	install -p ../include/ngzmp.h  $(INC_DIR)
	install -p ../include/ngzmp_dma_protocol.h  $(INC_DIR)
	install -p ../include/axi_hist.h  $(INC_DIR)
	install -p ../include/ngpd_dummy.h  $(INC_DIR)
	install -p $(LIB) $(LIB_DIR)
ifdef MAKE_SHARED
#	cp ${OBJ_DIR}/libngpd.so.1.0.0 $(LIB_DIR)
	@echo "MAKE SHARED"
	install -p ${OBJ_DIR}/libngpd.so.1.0.0 $(LIB_DIR)
	(cd $(LIB_DIR); rm -f libngpd.so.1.0; rm -f libngpd.so)
	(cd $(LIB_DIR); ln -s libngpd.so.1.0.0  libngpd.so.1.0; ln -s libngpd.so.1.0 libngpd.so)
endif
clean:
	rm -f $(OBJS) $(LIB) ${OBJ_DIR}/libngpd.so.1.0.0


$(OBJ_DIR)/zynqmp_api.o: ../libzynqmp/zynqmp_api.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
#--  rule: ./*.c
#------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c $(INCLUDES)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
		
