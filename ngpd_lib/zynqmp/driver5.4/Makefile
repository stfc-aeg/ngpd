export ARCH = arm
#export CROSS_COMPILE = arm-xilinx-linux-gnueabi-
export CROSS_COMPILE = aarch64-linux-gnu-

ifneq ($(KERNELRELEASE),)
obj-m := ngzmp.o 
ngzmp-objs := modload.o open_close.o ioctl.o dma_control.o dma_debug.o display.o test_patterns.o ngpd_dma.o ngzmp_ioctl.o

EXTRA_CFLAGS +=  -I$(PWD)/../include -I$(PWD)/../../include

else
MODNAME = xspress4.ko

KDIR :=  /home/wih73/da-tech/trunk/linux/xilinx/zynqmp/linux-xlnx-5.4
#KDIR :=  /home/wih73/da-tech/trunk/linux/xilinx/zynqmp/linux-xlnx-5.10
#KDIR :=  /home/wih73/da-tech/trunk/linux/xilinx/zynqmp/linux-xlnx-5.4-memdebug
PWD	:= $(shell pwd)

$(MODNAME):
	$(MAKE) -C $(KDIR) M=$(PWD) modules

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

endif
