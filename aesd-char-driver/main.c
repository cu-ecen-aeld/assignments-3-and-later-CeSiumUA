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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("CeSiumUA");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;

    PDEBUG("open");
    
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
    retval = bytes_read;

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
    size_t new_entry_size = 0;

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

    new_entry_size = (dev->buf_entry.size + count);

    dev->buf_entry.buffptr = krealloc(dev->buf_entry.buffptr, new_entry_size, GFP_KERNEL);
    if(dev->buf_entry.buffptr == NULL)
    {
        PERROR("unable to allocate %lu bytes of memory", new_entry_size);
        retval = -ENOMEM;
        goto aesd_write_exit;
    }

    retval = copy_from_user((void *)(dev->buf_entry.buffptr + dev->buf_entry.size), buf, count);
    if(retval != 0)
    {
        // FIXME - not all bytes could be transfered, so it could be possible to continue execution even if retval != 0
        PERROR("copy_from_user failed");
        retval = -EFAULT;
        goto aesd_write_exit;
    }

    dev->buf_entry.size += count;
    retval = count;

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

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos = 0;
    loff_t size = 0;
    struct aesd_buffer_entry *entry = NULL;
    struct aesd_circular_buffer *circular_buf = NULL;
    struct aesd_dev *dev = NULL;

    PDEBUG("llseek %lld %d",off,whence);

    dev = filp->private_data;

    if(dev == NULL)
    {
        PERROR("device not found");
        return -ENODEV;
    }

    circular_buf = &dev->circular_buf;

    mutex_lock(&dev->mutex_lock);

    AESD_CIRCULAR_BUFFER_FOREACH(entry,circular_buf,size) {
        size += dev->buf_entry.size;
    }

    mutex_unlock(&dev->mutex_lock);

    newpos = fixed_size_llseek(filp, off, whence, size);
    
    return newpos;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    long retval = 0;
    struct aesd_seekto aesd_seek;
    struct aesd_dev *dev = NULL;
    struct aesd_circular_buffer *circular_buffer = NULL;
    int available_entries = 0;
    int command_idx = 0;
    size_t entry_offset = 0;
    int i = 0;

    dev = filp->private_data;
    if(dev == NULL)
    {
        PERROR("device not found");
        return -ENODEV;
    }

    circular_buffer = &dev->circular_buf;
    if(circular_buffer == NULL)
    {
        PERROR("circular buffer not found");
        return -ENODEV;
    }

    PDEBUG("ioctl request %u",cmd);
    PDEBUG("ioctl arg %lu",arg);
    
    if(_IOC_TYPE(cmd) != AESD_IOC_MAGIC)
    {
        PERROR("invalid ioctl magic, expected: %x, got: %x", AESD_IOC_MAGIC, _IOC_TYPE(cmd));
        return -ENOTTY;
    }
    
    if(_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR)
    {
        PERROR("invalid ioctl command");
        return -ENOTTY;
    }

    if(cmd != AESDCHAR_IOCSEEKTO)
    {
        PERROR("invalid ioctl command");
        return -ENOTTY;
    }

    if(copy_from_user(&aesd_seek, (const void __user *)arg, sizeof(struct aesd_seekto)))
    {
        PERROR("copy_from_user failed");
        return -EFAULT;
    }

    if(mutex_lock_interruptible(&dev->mutex_lock))
    {
        PERROR("unable to acquire mutex");
        return -ERESTARTSYS;
    }

    available_entries = circular_buffer->in_offs - circular_buffer->out_offs;
    if(circular_buffer->full){
        available_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    
    PDEBUG("available entries: %d", available_entries);

    if(aesd_seek.write_cmd >= available_entries)
    {
        PERROR("invalid write command");
        retval = -EINVAL;
        goto aesd_ioctl_exit;
    }

    PDEBUG("write command: %d", aesd_seek.write_cmd);
    PDEBUG("write command offset: %d", aesd_seek.write_cmd_offset);

    command_idx = (circular_buffer->out_offs + aesd_seek.write_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    i = circular_buffer->out_offs;

    while(i != command_idx)
    {
        entry_offset += circular_buffer->entry[i].size;
        i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    if(circular_buffer->entry[i].size < aesd_seek.write_cmd_offset)
    {
        PERROR("invalid write command offset");
        retval = -EINVAL;
        goto aesd_ioctl_exit;
    }

    entry_offset += aesd_seek.write_cmd_offset;

    PDEBUG("entry offset: %zu", entry_offset);

    filp->f_pos = entry_offset;

    PDEBUG("new file position: %lld", filp->f_pos);

aesd_ioctl_exit:
    mutex_unlock(&dev->mutex_lock);

    return retval;
}

struct file_operations aesd_fops = {
    .owner =            THIS_MODULE,
    .read =             aesd_read,
    .write =            aesd_write,
    .open =             aesd_open,
    .release =          aesd_release,
    .llseek =           aesd_llseek,
    .unlocked_ioctl =   aesd_ioctl
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
    uint8_t index = 0;
    struct aesd_buffer_entry *entry = NULL;

    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.circular_buf,index) {
        if(entry->buffptr != NULL)
        {
            kfree(entry->buffptr);
            entry->buffptr = NULL;
        }
    }

    mutex_destroy(&aesd_device.mutex_lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
