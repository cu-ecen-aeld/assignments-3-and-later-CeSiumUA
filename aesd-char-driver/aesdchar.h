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

struct aesd_dev
{
	struct aesd_buffer_entry buf_entry;
	struct aesd_circular_buffer circular_buf;
    struct mutex mutex_lock;
    struct cdev cdev;
};

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
