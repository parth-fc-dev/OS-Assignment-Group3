#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/fs.h>

#include <linux/serial.h>

#include <linux/tty.h>

#include <linux/termios.h>



#define SERIAL_PORT "/dev/ttyUSB0"  // Change this to your desired serial port



static struct file *serial_file;

static int serial_open_count = 0;



static int serial_open(struct inode *inode, struct file *filp) {

    if (serial_open_count) {

        printk(KERN_ERR "Serial port already open\n");

        return -EBUSY;

    }



    serial_file = filp_open(SERIAL_PORT, O_RDWR | O_NOCTTY, 0);

    if (IS_ERR(serial_file)) {

        printk(KERN_ERR "Failed to open serial port %s\n", SERIAL_PORT);

        return PTR_ERR(serial_file);

    }



    serial_open_count++;

    printk(KERN_INFO "Serial port opened\n");

    return 0;

}



static int serial_release(struct inode *inode, struct file *filp) {

    if (!serial_open_count) {

        printk(KERN_ERR "Serial port already closed\n");

        return -ENODEV;

    }



    filp_close(serial_file, NULL);

    serial_open_count--;

    printk(KERN_INFO "Serial port closed\n");

    return 0;

}



static ssize_t serial_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {

    char buffer[256];

    int bytesRead;



    bytesRead = kernel_read(serial_file, buffer, sizeof(buffer), &serial_file->f_pos);

    if (bytesRead <= 0) {

        return bytesRead;

    }



    if (copy_to_user(buf, buffer, bytesRead)) {

        printk(KERN_ERR "Failed to copy data to user space\n");

        return -EFAULT;

    }



    printk(KERN_INFO "Received from serial port %s: %.*s\n", SERIAL_PORT, bytesRead, buffer);

    return bytesRead;

}



static struct file_operations serial_fops = {

    .owner = THIS_MODULE,

    .open = serial_open,

    .release = serial_release,

    .read = serial_read,

};



static int __init serial_reader_init(void) {

    int ret;



    ret = register_chrdev(0, "serial_reader", &serial_fops);

    if (ret < 0) {

        printk(KERN_ERR "Failed to register character device\n");

        return ret;

    }



    printk(KERN_INFO "Serial reader driver loaded\n");

    return 0;

}

static int major_number;



static void __exit serial_reader_exit(void) {

    unregister_chrdev(major_number, "serial_reader");

    printk(KERN_INFO "Serial reader driver unloaded\n");

}

module_init(serial_reader_init);

module_exit(serial_reader_exit);



MODULE_LICENSE("GPL");

MODULE_AUTHOR("Your Name");

MODULE_DESCRIPTION("Serial Reader Driver");