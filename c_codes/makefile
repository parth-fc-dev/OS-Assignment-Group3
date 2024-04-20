

obj-m += brightness_control.o

all:

	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:

	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean



obj-m := module_name.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all default: modules
install: modules_install

modules modules_install help clean:
    $(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@


if (ioctl(serial_file->private_data, TIOCGSERIAL, &serial_info) < 0) {

        printk(KERN_ERR "Failed to get serial info\n");

        filp_close(serial_file, NULL);

        return -1;

    }



    serial_info.flags |= ASYNC_LOW_LATENCY;

    if (ioctl(serial_file->private_data, TIOCSSERIAL, &serial_info) < 0) {

        printk(KERN_ERR "Failed to set low latency mode\n");

        filp_close(serial_file, NULL);

        return -1;

    }