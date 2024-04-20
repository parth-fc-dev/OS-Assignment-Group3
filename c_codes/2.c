#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/termios.h>
#include <linux/version.h>  // Added for kernel version check

#define DEVICE_NAME "serial_reader"
#define CLASS_NAME "serial"
#define SERIAL_PORT "/dev/ttyUSB0"  // Change this to your desired serial port

static dev_t dev_num;
static struct cdev serial_cdev;
static struct task_struct *serial_thread;
static int running = 1;

static int serial_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "Serial device opened\n");
    return 0;
}

static int serial_release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "Serial device closed\n");
    return 0;
}

static ssize_t serial_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    return 0;  // Not implemented
}

static ssize_t serial_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    return 0;  // Not implemented
}

static struct file_operations serial_fops = {
    .owner = THIS_MODULE,
    .open = serial_open,
    .release = serial_release,
    .read = serial_read,
    .write = serial_write,
};

static int serial_thread_func(void *data) {
    struct serial_struct serial_info;
    struct file *serial_file;
    char buffer[256];
    int bytesRead;

    serial_file = filp_open(SERIAL_PORT, O_RDWR | O_NOCTTY, 0);
    if (IS_ERR(serial_file)) {
        printk(KERN_ERR "Failed to open serial port %s\n", SERIAL_PORT);
        return PTR_ERR(serial_file);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
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
#endif

    while (running) {
        bytesRead = kernel_read(serial_file, buffer, sizeof(buffer), &serial_file->f_pos);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  // Null-terminate the buffer
            printk(KERN_INFO "Received from serial port %s: %s\n", SERIAL_PORT, buffer);
        }
        msleep(100);
    }

    filp_close(serial_file, NULL);
    return 0;
}

static int __init serial_reader_init(void) {
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate device number\n");
        return ret;
    }

    cdev_init(&serial_cdev, &serial_fops);
    serial_cdev.owner = THIS_MODULE;

    ret = cdev_add(&serial_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
    serial_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(serial_class)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&serial_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(serial_class);
    }

    device_create(serial_class, NULL, dev_num, NULL, DEVICE_NAME);
#endif

    serial_thread = kthread_run(serial_thread_func, NULL, "serial_reader_thread");
    if (IS_ERR(serial_thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        class_destroy(serial_class);
#endif
        cdev_del(&serial_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(serial_thread);
    }

    printk(KERN_INFO "Serial reader driver loaded\n");
    return 0;
}

static void __exit serial_reader_exit(void) {
    running = 0;
    kthread_stop(serial_thread);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
    device_destroy(serial_class, dev_num);
    class_destroy(serial_class);
#endif
    cdev_del(&serial_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Serial reader driver unloaded\n");
}

module_init(serial_reader_init);
module_exit(serial_reader_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Serial Reader Driver");