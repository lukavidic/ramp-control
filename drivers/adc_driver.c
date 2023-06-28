#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#define I2C_BUS_AVAILABLE   (1)              // I2C Bus available in our Raspberry Pi
#define SLAVE_DEVICE_NAME   ("ETX_ADC")              // Device and Driver Name
#define ADC_SLAVE_ADDR  (0x48)              // Slave Address

static struct i2c_adapter *etx_i2c_adapter     = NULL;  // I2C Adapter Structure
static struct i2c_client  *etx_i2c_client_adc = NULL;  // I2C Cient Structure 
const char INIT_MSG = 0x8c;
const char EXIT_MSG = 0x80;
int adc_driver_major;
char data[2];


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("PURV Grupa");
MODULE_DESCRIPTION("ADC Driver");

/*
** This function writes the data into the I2C client
**
**  Arguments:
**      buff -> buffer to be sent
**      len  -> Length of the data
**   
*/
static int I2C_Write()
{
    /*
    ** Sending Start condition, Slave address with R/W bit, 
    ** ACK/NACK and Stop condtions will be handled internally.
    */ 
    int ret = i2c_master_send(etx_i2c_client_adc, INIT_MSG, 2);
    
    return ret;
}

/*
** This function reads one byte of the data from the I2C client
**
**  Arguments:
**      out_buff -> buffer wherer the data to be copied
**      len      -> Length of the data to be read
** 
*/
static int I2C_Read()
{
    /*
    ** Sending Start condition, Slave address with R/W bit, 
    ** ACK/NACK and Stop condtions will be handled internally.
    */ 
    int ret = i2c_master_recv(etx_i2c_client_adc, data, 2);
    
    return ret;
}

/*
** This function getting called when the slave has been found
** Note : This will be called only once when we load the driver.
*/
static int etx_adc_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    pr_info("ADC Probed!!!\n");
    return 0;
}

/*
** This function getting called when the slave has been removed
** Note : This will be called only once when we unload the driver.
*/
static int etx_adc_remove(struct i2c_client *client)
{   
    int ret = i2c_master_send(etx_i2c_client_adc, &EXIT_MSG, 1);

    pr_info("ADC Removed!!!\n");
    return 0;
}

/* File open function. */
static int adc_driver_open(struct inode *inode, struct file *filp)
{
    return 0;
}

/* File close function. */
static int adc_driver_release(struct inode *inode, struct file *filp)
{
    return 0;
}


static ssize_t adc_driver_read(struct file *filp, char *buf, size_t len, loff_t *f_pos)
{
    int data_size = 0;

    if (*f_pos == 0)
    {
        /* Get size of valid data. */
        I2C_Write();
        I2C_Read();
        /* Send data to user space. */
        if (copy_to_user(buf, data, 2) != 0)
        {
            return -EFAULT;
        }
        else
        {
            (*f_pos) += data_size;

            return data_size;
        }
    }
    else
    {
        return 0;
    }
}

static ssize_t adc_driver_write(struct file *filp, const char *buf, size_t len, loff_t *f_pos)
{

    /* Get data from user space.*/
    if (copy_from_user(led_buff, buf, len) != 0)
    {
        return -EFAULT;
    }
    else
    {
        return 0;
    }
}

/*
** Structure that has slave device id
*/
static const struct i2c_device_id etx_adc_id[] = {
        { SLAVE_DEVICE_NAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, etx_adc_id);


/*
** I2C driver Structure that has to be added to linux
*/
static struct i2c_driver etx_adc_driver = {
        .driver = {
            .name   = SLAVE_DEVICE_NAME,
            .owner  = THIS_MODULE,
        },
        .probe          = etx_adc_probe,
        .remove         = etx_adc_remove,
        .id_table       = etx_adc_id,
};

/* File operations struct */

static struct file_operations adc_fops = {
	.open = adc_driver_open,
	.release = adc_driver_close,
	.write = adc_driver_write,
    .read = adc_driver_read
};

/*
** I2C Board Info strucutre
*/
static struct i2c_board_info adc_i2c_board_info = {
        I2C_BOARD_INFO(SLAVE_DEVICE_NAME, ADC_SLAVE_ADDR)
    };

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
    int ret = -1;
    int result = -1;
    etx_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    
    if( etx_i2c_adapter != NULL )
    {
        etx_i2c_client_adc = i2c_new_device(etx_i2c_adapter, &adc_i2c_board_info);
        
        if( etx_i2c_client_adc != NULL )
        {
            i2c_add_driver(&etx_adc_driver);
            ret = 0;
        }
        
        i2c_put_adapter(etx_i2c_adapter);
    }
    
    pr_info("I2C driver added!!!\n");

    printk(KERN_INFO "Inserting adc_driver module\n");

    /* Registering device. */
    result = register_chrdev(0, "adc_driver", &adc_fops);
    if (result < 0)
    {
        printk(KERN_INFO "adc_driver: cannot obtain major number %d\n", adc_driver_major);
        return result;
    }

    adc_driver_major = result;
    printk(KERN_INFO "adc_driver major number is %d\n", adc_driver_major);

    return ret;
}

/*
** Module Exit function
*/
static void __exit etx_driver_exit(void)
{
    i2c_unregister_device(etx_i2c_client_adc);
    i2c_del_driver(&etx_adc_driver);
    unregister_chrdev(adc_driver_major, "adc_driver");
    pr_info("I2C driver Removed!!!\n");
    pr_info("adc_driver removed!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);