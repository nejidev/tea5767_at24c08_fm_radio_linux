KERN_DIR = /home/linux-4.1.36

all:
	arm-linux-gcc -o fm fm.c
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	rm fm
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
	

obj-m	+= at24cxx.o
obj-m	+= at24cxx_dev.o
obj-m	+= tea5767.o
obj-m	+= tea5767_dev.o
