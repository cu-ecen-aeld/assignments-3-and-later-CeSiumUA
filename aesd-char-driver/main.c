/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("CeSiumUA");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");

    struct aesd_dev *dev = NULL;
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");

    filp->private_data = NULL;

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    ssize_t bytes_read = 0;
    size_t entry_offset = 0;
    struct aesd_buffer_entry *entry = NULL;
    struct aesd_dev *dev = NULL;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
    if(filp == NULL || buf == NULL)
    {
        PERROR("invalid arguments");
        return -EINVAL;
    }

    dev = filp->private_data;

    if(dev == NULL)
    {
        PERROR("device not found");
        return -ENODEV;
    }

    if(mutex_lock_interruptible(&dev->mutex_lock))
    {
        PERROR("unable to acquire mutex");
        return -ERESTARTSYS;
    }

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buf, *f_pos, &entry_offset);
    if(entry == NULL)
    {
        retval = bytes_read;
        goto aesd_read_exit;
    }

    bytes_read = entry->size - entry_offset;

    if(bytes_read > count)
    {
        bytes_read = count;
    }

    retval = copy_to_user(buf, entry->buffptr + entry_offset, bytes_read);

    if(retval != 0)
    {
        // FIXME - not all bytes could be transfered, so it could be possible to continue execution even if retval != 0
        PERROR("copy_to_user failed");
        retval = -EFAULT;
        goto aesd_read_exit;
    }

    *f_pos += bytes_read;

aesd_read_exit:
    mutex_unlock(&dev->mutex_lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = NULL;
    const char *free_buffer_ptr = NULL;

    if(filp == NULL || buf == NULL)
    {
        PERROR("invalid arguments");
        return -EINVAL;
    }
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    dev = filp->private_data;

    if(dev == NULL)
    {
        PERROR("device not found");
        return -ENODEV;
    }

    if(mutex_lock_interruptible(&dev->mutex_lock))
    {
        PERROR("unable to acquire mutex");
        return -ERESTARTSYS;
    }

    size_t new_entry_size = (dev->buf_entry.size + count);

    dev->buf_entry.buffptr = krealloc(dev->buf_entry.buffptr, new_entry_size, GFP_KERNEL);
    if(dev->buf_entry.buffptr == NULL)
    {
        PERROR("unable to allocate %lu bytes of memory", new_entry_size);
        retval = -ENOMEM;
        goto aesd_write_exit;
    }

    retval = copy_from_user((dev->buf_entry.buffptr + dev->buf_entry.size), buf, count);
    if(retval != 0)
    {
        // FIXME - not all bytes could be transfered, so it could be possible to continue execution even if retval != 0
        PERROR("copy_from_user failed");
        retval = -EFAULT;
        goto aesd_write_exit;
    }

    dev->buf_entry.size += count;

    if(dev->buf_entry.buffptr[dev->buf_entry.size - 1] == '\n')
    {
        free_buffer_ptr = aesd_circular_buffer_add_entry(&dev->circular_buf, &dev->buf_entry);

        if(free_buffer_ptr != NULL)
        {
            kfree(free_buffer_ptr);
            free_buffer_ptr = NULL;
        }

        dev->buf_entry.buffptr = NULL;
        dev->buf_entry.size = 0;
    }

aesd_write_exit:
    mutex_unlock(&dev->mutex_lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.mutex_lock);
    aesd_circular_buffer_init(&aesd_device.circular_buf);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
