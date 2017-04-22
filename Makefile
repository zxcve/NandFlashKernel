obj-m += prototype.o
prototype-objs :=core.o device.o mapper.o

# Kernel source root directory :
KERN_DIR=/home/sujay/Documents/LKP/flashproject/VM/linux-4.0.9
# Target architecture :
ARCH=x86_64

TARGET=user@127.0.0.1:~

all:
	make -C $(KERN_DIR) M=$(PWD) ARCH=$(ARCH) modules
	# cleanup
	rm -rf *.o *.mod.c modules.order Module.symvers

install: all
	scp -P 2222 prototype.ko insert_mod.sh  $(TARGET)
	
clean:
	make -C $(KERN_DIR) M=$(PWD) ARCH=$(ARCH) clean
