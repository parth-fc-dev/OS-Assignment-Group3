#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>

// Serial communication headers
#include <linux/serial.h>
#include <linux/serial_core.h>

#define DEVICE_NAME "brightness_control"
#define SERIAL_PORT "/dev/ttyUSB0"
#define BRIGHTNESS_FILE "/sys/class/backlight/acpi_video0/brightness"

static struct task_struct *thread;
static int running = 1;

// Function to handle brightness adjustment
static void adjust_brightness(int brightness_value) {
    int brightness_fd;
    char brightness_value_str[4]; // Max 3 digits + null terminator

    // Open the brightness file
    brightness_fd = open(BRIGHTNESS_FILE, O_WRONLY);
    if (brightness_fd < 0) {
        printk(KERN_ERR "Failed to open brightness file\n");
        return;
    }

    // Convert brightness value to string
    snprintf(brightness_value_str, sizeof(brightness_value_str), "%d", brightness_value);

    // Write brightness value to the brightness file
    if (write(brightness_fd, brightness_value_str, strlen(brightness_value_str)) < 0) {
        printk(KERN_ERR "Failed to write to brightness file\n");
    }

    // Close the brightness file descriptor
    close(brightness_fd);
}

// Thread function to read values from Arduino and adjust brightness
static int brightness_thread(void *data) {
    struct serial_struct serinfo;
    int serial_fd;
    char buffer[255];
    int brightness_value;

    // Open serial port
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        printk(KERN_ERR "Failed to open serial port\n");
        return -1;
    }

    // Configure serial port
    if (ioctl(serial_fd, TIOCGSERIAL, &serinfo) < 0) {
        printk(KERN_ERR "Failed to get serial info\n");
        close(serial_fd);
        return -1;
    }
    serinfo.flags |= ASYNC_LOW_LATENCY;
    if (ioctl(serial_fd, TIOCSSERIAL, &serinfo) < 0) {
        printk(KERN_ERR "Failed to set low latency mode\n");
        close(serial_fd);
        return -1;
    }

    // Read from serial port and adjust brightness
    while (!kthread_should_stop()) {
        if (read(serial_fd, buffer, sizeof(buffer)) > 0) {
            brightness_value = atoi(buffer);
            adjust_brightness(brightness_value);
        }
        msleep(100); // Adjust delay as needed
    }

    // Cleanup
    close(serial_fd);
    return 0;
}

// Module initialization
static int __init brightness_control_init(void) {
    // Start kernel thread for brightness control
    thread = kthread_run(brightness_thread, NULL, "brightness_control_thread");
    if (IS_ERR(thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(thread);
    }
    return 0;
}

// Module cleanup
static void __exit brightness_control_exit(void) {
    // Stop kernel thread
    running = 0;
    kthread_stop(thread);
}

module_init(brightness_control_init);
module_exit(brightness_control_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Brightness Control Module");