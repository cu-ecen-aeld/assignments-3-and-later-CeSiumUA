/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    int composed_length = 0;
    int i;

    if(buffer == NULL || entry_offset_byte_rtn == NULL)
    {
        goto find_entry_offset_for_fpos_exit;
    }

    PDEBUG("initial pos: out: %d in: %d, searched len: %ld\n", buffer->out_offs, buffer->in_offs, char_offset);

    for(i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        int buff_idx = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        PDEBUG("i: %d\n", i);
        composed_length += buffer->entry[buff_idx].size;
        PDEBUG("composed_length: %d\n", composed_length);
        if(composed_length > char_offset)
        {
            *entry_offset_byte_rtn = char_offset - (composed_length - buffer->entry[buff_idx].size);
            PDEBUG("entry_offset_byte_rtn: %ld\n", *entry_offset_byte_rtn);
            PDEBUG("entry str: %s\n", buffer->entry[buff_idx].buffptr);
            return &(buffer->entry[buff_idx]);
        }
    }

    PDEBUG("didn't find\n");

find_entry_offset_for_fpos_exit:
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char *old_ptr = NULL;

    if(buffer == NULL || add_entry == NULL)
    {
        goto add_entry_exit;
    }

    if(buffer->full)
    {
        old_ptr = buffer->entry[buffer->in_offs].buffptr;
    }

    buffer->entry[buffer->in_offs] = *add_entry;
    
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    if(buffer->full)
    {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    if(buffer->in_offs == 0)
    {
        buffer->full = true;
    }

add_entry_exit:
    return old_ptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
