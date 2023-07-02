#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/delay.h>

/* Meta Information */
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("PURV Grupa");
MODULE_DESCRIPTION("A simple driver for buzzer");

/* Variables for device and device class */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "buzz_driver"
#define DRIVER_CLASS "BuzzerClass"

/* Variables for PWM  */
struct pwm_device *pwm0 = NULL;

/* duty cycle of PWM
    ** will be passed as an argument to function pwm_config  
*/

/**
 * @brief Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;
    char value;

    /* Get amount of data to copy */
    to_copy = min(count, sizeof(value));

    /* Copy data to user */
    not_copied = copy_from_user(&value, user_buffer, to_copy);

    printk("%s\n", user_buffer);

    /* Set PWM on time: a - stop buzzing ; b - start buzzing */
    if(value != 'a' && value != 'b')
        printk("Invalid Value\n");
    else
        pwm_config(pwm0, 1000000 * (value - 'a'), 2600000);

    if(value == 'b'){
        msleep(1000);
    }
    pwm_config(pwm0, 0, 2600000);

    /* Calculate data */
    delta = to_copy - not_copied;

    return delta;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_open(struct inode *device_file, struct file *instance) {
    printk("dev_nr - open was called!\n");
    return 0;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_close(struct inode *device_file, struct file *instance) {
    printk("dev_nr - close was called!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .write = driver_write
};

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init ModuleInit(void) {
    printk("buzz_driver!\n");

    /* Allocate a device nr */
    if( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
        printk("buzz_driver Nr. could not be allocated!\n");
        return -1;
    }
    printk("read_write - Device Nr. Major: %d, Minor: %d was registered!\n", my_device_nr >> 20, my_device_nr && 0xfffff);

    /* Create device class */
    if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
        printk("Device class can not be created!\n");
        goto ClassError;
    }

    /* create device file */
    if(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
        printk("Can not create device file!\n");
        goto FileError;
    }

    /* Initialize device file */
    cdev_init(&my_device, &fops);

    /* Regisering device to kernel */
    if(cdev_add(&my_device, my_device_nr, 1) == -1) {
        printk("Registering of device to kernel failed!\n");
        goto AddError;
    }

    pwm0 = pwm_request(1, "buzz-pwm");
    if(pwm0 == NULL) {
        printk("Could not get PWM0!\n");
        goto AddError;
    }

    pwm_config(pwm0, 0, 20000000);
    pwm_enable(pwm0);

    return 0;
AddError:
    device_destroy(my_class, my_device_nr);
FileError:
    class_destroy(my_class);
ClassError:
    unregister_chrdev_region(my_device_nr, 1);
    return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
    pwm_disable(pwm0);
    pwm_free(pwm0);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    printk("buzz_driver exit\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);