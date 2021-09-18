
/**************************************************************************************************
 *  INCLUDES                                                                                      *
 **************************************************************************************************/
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "cir_buf.h"


/**************************************************************************************************
 *  TYPEDEFS AND MACROS                                                                           *
 **************************************************************************************************/

 /******************************************************************************
  *  LOCAL VARIABLES                                                           *
  ******************************************************************************/

static size_t SFM_CircBuff_GetFreeSize(SFM_CircularBuffer *cbuffer);
static size_t SFM_CircBuff_GetDataSize(SFM_CircularBuffer *cbuffer);

/* ============================================================================================== *
 *                                     *** LOCAL FUNCTIONS ***                                    */

/** @brief Returns free size of the circular buffer
 *
 * @param[in] circular buffer Pointer
 *
 * @return available free size of circular buffer
 */
static size_t SFM_CircBuff_GetFreeSize(SFM_CircularBuffer *cbuffer)
{
    unsigned int    free_size;

    if (cbuffer->cb_write_ptr > cbuffer->cb_read_ptr)
    {
        /* size from write pointer to end of the buffer. */
        free_size = ((cbuffer->circular_buff_end ) - cbuffer->cb_write_ptr);
        /* size from start of cb to read pointer */
        free_size += (cbuffer->cb_read_ptr - cbuffer->circular_buffer_start);
    }
    else if (cbuffer->cb_write_ptr < cbuffer->cb_read_ptr)
    {
        /* -1 is used to keep write pointer away from read ptr by atleast 1 byte while writing */
        free_size = (cbuffer->cb_read_ptr - cbuffer->cb_write_ptr - 1);
    }
    else
    {
        /*if both read and write pointers are same, entire buffer is free */
        free_size = cbuffer->cb_size -1;
    }

    return free_size;
}

/** @brief Returns data occupied size of the circular buffer
 * @param[in] circular buffer Pointer
 * @return available data size of circular buffer
 */
static size_t SFM_CircBuff_GetDataSize(SFM_CircularBuffer *cbuffer)
{
    unsigned int    data_size;

    if(cbuffer->cb_read_ptr == cbuffer->cb_write_ptr)
    {
        /*no data is written yet */
        data_size = 0;
    }
    else if(cbuffer->cb_write_ptr > cbuffer->cb_read_ptr)
    {
        data_size = cbuffer->cb_write_ptr - cbuffer->cb_read_ptr;
    }
    else
    {
        data_size = (cbuffer->circular_buff_end - cbuffer->cb_read_ptr + 1);
        data_size += cbuffer->cb_write_ptr - cbuffer->circular_buffer_start;
    }

    return data_size;
}

/* ============================================================================================== *
 *                                     *** GLOBAL FUNCTIONS ***                                   */

/** @brief  writes data into circular buffer
 *
 * @param[in] circular buffer Pointer
 * @param[in] input buffer pointer from where the data has to be copied to CB
 * @param[in] Size of the data to be copied
 *
 * @return size - if successful
 * @return -1 - if there is no space in circular buffer
 */
int32_t SFM_CircBuff_WritetoCB(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size)
{
    int bytes_written = -1;
    int wrap_size;

    pthread_mutex_lock(&cbuffer->sfm_circular_buffer_mutex);

    if(SFM_CircBuff_GetFreeSize(cbuffer) >= size )
    {
        /* Enough space is there to copy */

        /* check for linear space available */
        if((cbuffer->cb_write_ptr + size - 1) <= cbuffer->circular_buff_end )
        {
            memcpy(cbuffer->cb_write_ptr, buffer, size );
            cbuffer->cb_write_ptr += size;

        /* To handle linear space ends at the buffer end */
        if((cbuffer->cb_write_ptr - 1) == cbuffer->circular_buff_end)
            cbuffer->cb_write_ptr = cbuffer->circular_buffer_start;
        }
        /*circular buffer wrap around case */
        else
        {
            wrap_size = (cbuffer->circular_buff_end - cbuffer->cb_write_ptr +1);
            memcpy(cbuffer->cb_write_ptr,  buffer, wrap_size);
            memcpy(cbuffer->circular_buffer_start, (buffer + wrap_size), (size - wrap_size));
            cbuffer->cb_write_ptr = cbuffer->circular_buffer_start + (size - wrap_size);
        }

        bytes_written = size;
    }

    pthread_mutex_unlock(&cbuffer->sfm_circular_buffer_mutex);

    return bytes_written;

}

/** @brief  reads data from circular buffer
 *
 * @param[in] circular buffer Pointer
 * @param[in] ouput buffer pointer to where the data has to be copied from CB
 * @param[in] Size of the data to be copied
 *
 * @return size if successful
 * @return -1   if there is no data available in circular buffer
 *
 */
int32_t SFM_CircBuff_ReadFromCB(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size)
{
    int bytes_read = -1;
    int wrap_size;

    pthread_mutex_lock(&cbuffer->sfm_circular_buffer_mutex);

    if(SFM_CircBuff_GetDataSize(cbuffer) >= size )
    {

        /* check for linear space available */
        if((cbuffer->cb_read_ptr + size -1 ) <= cbuffer->circular_buff_end )
        {
            memcpy(buffer, cbuffer->cb_read_ptr, size);
            cbuffer->cb_read_ptr += size;

            if((cbuffer->cb_read_ptr -1 ) == cbuffer->circular_buff_end)
            {
                cbuffer->cb_read_ptr = cbuffer->circular_buffer_start;
            }

        }
        /*circular buffer wrap around case */
        else
        {
            wrap_size = (cbuffer->circular_buff_end - cbuffer->cb_read_ptr +1);
            memcpy(buffer,  cbuffer->cb_read_ptr, wrap_size);
            memcpy((buffer + wrap_size),  cbuffer->circular_buffer_start, (size - wrap_size));

            cbuffer->cb_read_ptr = cbuffer->circular_buffer_start + (size - wrap_size);
        }

        bytes_read  =   size;

    }

    pthread_mutex_unlock(&cbuffer->sfm_circular_buffer_mutex);

    return bytes_read;
}

/** @brief  reads data from circular buffer without updating read pointer
 *
 * @param[in] circular buffer Pointer
 * @param[in] ouput buffer pointer to where the data has to be copied from CB
 * @param[in] Size of the data to be copied
 *
 * @return size - if successful
 * @return -1     if there is no data available in circular buffer
 */
int32_t SFM_CircBuff_Peek(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size)
{
    int bytes_read = -1;
    int wrap_size;


    pthread_mutex_lock(&cbuffer->sfm_circular_buffer_mutex);

    if(SFM_CircBuff_GetDataSize(cbuffer) >= size )
    {
        /* check for linear space available */
        if ((cbuffer->cb_read_ptr + size -1 ) <= cbuffer->circular_buff_end)
        {
            memcpy(buffer, cbuffer->cb_read_ptr, size);
        }
        /* circular buffer wrap around case */
        else
        {
            wrap_size = (cbuffer->circular_buff_end - cbuffer->cb_read_ptr +1);
            memcpy(buffer,  cbuffer->cb_read_ptr, wrap_size);
            memcpy((buffer + wrap_size),  cbuffer->circular_buffer_start, size - wrap_size);
        }

        bytes_read  =   size;
    }

    pthread_mutex_unlock(&cbuffer->sfm_circular_buffer_mutex);

    return bytes_read;
}


/** @brief  resets read/write pointers of circular buffer to circular buffer start.
 *
 * @param[in] circular buffer Pointer
 *
 * @return void
 */
void SFM_CircBuff_Flush(SFM_CircularBuffer *cbuffer)
{

    pthread_mutex_lock(&cbuffer->sfm_circular_buffer_mutex);

    /* Reset read and write pointers */
    cbuffer->cb_read_ptr    =   cbuffer->circular_buffer_start;
    cbuffer->cb_write_ptr   =   cbuffer->circular_buffer_start;

    pthread_mutex_unlock(&cbuffer->sfm_circular_buffer_mutex);

}


/** @brief  prints circular buffer stats.
 *
 * @param[in] circular buffer Pointer
 *
 * @return void
 */
void SFM_CircBuff_Stat(SFM_CircularBuffer *cbuffer)
{

    pthread_mutex_lock(&cbuffer->sfm_circular_buffer_mutex);

    printf("============= CB STATS =============\n");
    printf("start address 0x%08x\n", cbuffer->circular_buffer_start);
    printf("end address   0x%08x\n", cbuffer->circular_buff_end);
    printf("read pointer  0x%08x\n", cbuffer->cb_read_ptr);
    printf("write pointer 0x%08x\n", cbuffer->cb_write_ptr);
    printf("cb size          %d\n",  cbuffer->cb_size);
    printf("free size        %d\n",  SFM_CircBuff_GetFreeSize(cbuffer));
    printf("data size        %d\n",  SFM_CircBuff_GetDataSize(cbuffer));
    printf("====================================\n");

    pthread_mutex_unlock(&cbuffer->sfm_circular_buffer_mutex);

}

