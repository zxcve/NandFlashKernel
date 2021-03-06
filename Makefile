obj-m += project6.o
project6-objs := cache.o \
	garbage_collector.o \
	page_manager.o \
	keyval.o \
	meta_data.o \
	core.o \
	device.o

# Kernel source root directory :
KERN_DIR=/home/zxcve/Project6/VM/linux-4.0.9
# Target architecture :
ARCH=x86_64

TARGET=user@192.168.53.89:~

all:
	make -C $(KERN_DIR) M=$(PWD) ARCH=$(ARCH) modules
	# cleanup
	rm -rf *.o *.mod.c modules.order Module.symvers

install: all
	scp -r project6.ko insert_mod.sh  $(TARGET)
	
clean:
	make -C $(KERN_DIR) M=$(PWD) ARCH=$(ARCH) clean
