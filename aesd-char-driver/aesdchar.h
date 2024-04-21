/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#else
#include <stddef.h> // size_t
#include <stdint.h> // uintx_t
#include <stdbool.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#undef PINFO
#undef PERROR

#ifdef AESD_DEBUG
#ifdef __KERNEL__
    /* This one if debugging is on, and kernel space */
#	define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#	define PINFO(fmt, args...) printk( KERN_INFO "aesdchar: " fmt, ## args)
#	define PERROR(fmt, args...) printk( KERN_ERR "aesdchar: " fmt, ## args)
#else
    /* This one for user space */
#	define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#	define PINFO(fmt, args...) fprintf(stderr, fmt, ## args)
#	define PERROR(fmt, args...) fprintf(stderr, fmt, ## args)
#endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct aesd_dev
{
	struct aesd_circular_buffer circular_buf;
    struct mutex mutex_lock;
    struct cdev cdev;
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
