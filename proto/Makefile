obj-m += youku.o
#obj-m += qq.o

KBUILD_EXTRA_SYMBOLS += /home/appflid/mod/Module.symvers
export KBUILD_EXTRA_SYMBOLS

ccflags-y := -I$(src)/../include
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
