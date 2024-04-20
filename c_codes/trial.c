#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

/* ============== krn space '/dev/ttyN' access ============= */

static struct file *ktty_open(const char *filename, int flags, umode_t mode)
{
    return filp_open(filename, O_RDWR | O_NOCTTY, 0);
}

static ssize_t ktty_write(struct file *f, const char *buf, int count)
{

    int result;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    f->f_pos = 0;
    result = f->f_op->write(f, buf, count, &f->f_pos);
    set_fs(oldfs);

    return result;
}

static ssize_t ktty_read(struct file * f, char * buf, int count)
{
	int result = 0;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	result = f->f_op->read(f, buf, count, &f->f_pos);
	set_fs(oldfs);
	return result;
}

static void ktty_close(struct file *xktty, fl_owner_t id)
{
        filp_close(xktty, id);
}

/* =============== module file operations ===================== */
DEFINE_MUTEX(xmutex);
static struct file *xktty = NULL;
static int xktty_open(struct inode *inode, struct file *filp)
{
    #define XKTTY_MAX_PATH 20
    #define XKTTY_NUM 0
    char filename[XKTTY_MAX_PATH];

    /* only one process at a time */
    if(!(mutex_trylock(&xmutex)))
        return -EBUSY;

    snprintf(filename, XKTTY_MAX_PATH, "/dev/ttyACM%d", XKTTY_NUM);
    xktty = ktty_open(filename, 0, O_RDWR);
    if (PTR_RET(xktty)) {
        mutex_unlock(&xmutex);
        return PTR_RET(xktty);
    }

    return 0;
}

static int xktty_release(struct inode *inode, struct file *file)
{
    if(!IS_ERR_OR_NULL(xktty))
        ktty_close(xktty, 0);
    mutex_unlock(&xmutex);
    return 0;
}

static ssize_t xktty_write(struct file *filp,
                 const char __user * buf, size_t count,
                 loff_t * f_pos)
{

    #define XKTTY_MAX_BUF_LEN 200
    const char kbuf[XKTTY_MAX_BUF_LEN];

    count = count < XKTTY_MAX_BUF_LEN ? count : XKTTY_MAX_BUF_LEN;
    if (copy_from_user((char *)kbuf, (const char __user *)buf, count))
        return -EFAULT;

    if (!IS_ERR_OR_NULL(xktty))
        return ktty_write(xktty, kbuf, count);
    else
        return -EFAULT;
}

static ssize_t xktty_read(struct file * flip, char __user * buf, size_t count,
		loff_t * f_pos)
{
	char kbuf[XKTTY_MAX_BUF_LEN] = { '\0' };
	int result = 0;
	count = count < XKTTY_MAX_BUF_LEN ? count : XKTTY_MAX_BUF_LEN;

	if (!IS_ERR_OR_NULL(xktty))
	{
		printk(KERN_INFO "Tying to read...Count: %d\n", count);
		result = ktty_read(xktty, kbuf, count);
		printk("Buf: %s\nResult: %d\n", kbuf, result);

		if (copy_to_user((char *)buf, (char __user *)kbuf, result))
		{
			printk(KERN_ERR "Cannot copy\n");
			return -EFAULT;
		}
	}
	else
	{
		return -EFAULT;
	}

	return result;
}

static struct file_operations xktty_ops = {
    .owner = THIS_MODULE,
    .open = xktty_open,
    .release = xktty_release,
    .write = xktty_write,
	.read = xktty_read
};

/* =================== init/exit ======================== */

static struct cdev cdev;
static struct class *class;
static int xktty_mjr;

static int xktty_init(void)
{
    #define XKTTY_NAME "xktty"

    dev_t devt = MKDEV(0, 0);
    if (alloc_chrdev_region(&devt, 0, 1, XKTTY_NAME) < 0)
        return -1;
    xktty_mjr = MAJOR(devt);

    cdev_init(&cdev, &xktty_ops);
    cdev.owner = THIS_MODULE;
    devt = MKDEV(xktty_mjr, 0);
    if (cdev_add(&cdev, devt, 1))
        goto exit0;

    class = class_create(THIS_MODULE, XKTTY_NAME);
    if (!class)
        goto exit1;

    devt = MKDEV(xktty_mjr, 0);
    if (!(device_create(class, NULL, devt, NULL, XKTTY_NAME)))
        goto exit2;

    return 0;

exit2:
    class_destroy(class);
exit1:
    cdev_del(&cdev);
exit0:
    unregister_chrdev_region(MKDEV(xktty_mjr, 0), 1);

    return -1;
}

static void xktty_fini(void)
{
    device_destroy(class, MKDEV(xktty_mjr, 0));
    class_destroy(class);
    cdev_del(&cdev);
    unregister_chrdev_region(MKDEV(xktty_mjr, 0), 1);
}

module_init(xktty_init);
module_exit(xktty_fini);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("_ParaPik_");