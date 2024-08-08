MODULE_NAME = manual_flush
obj-m += $(MODULE_NAME).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

uninstall:
	@lsmod | grep -q $(MODULE_NAME) && \
	{ \
		echo "Module is loaded, attempting to remove..."; \
		sudo rmmod $(MODULE_NAME); \
	} || \
	{ \
		echo "Module is not loaded"; \
	}

install: all uninstall
	sudo insmod $(MODULE_NAME).ko