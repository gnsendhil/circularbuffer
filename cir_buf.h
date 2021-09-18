#ifndef SFM_CIRCULARBUFFER_UTIL_H
#define SFM_CIRCULARBUFFER_UTIL_H

#include <pthread.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef int int32_t;


/** @brief Structure used to process the data in circular buffer fashion */
typedef struct SFM_CircularBufferTag
{
    uint8_t      *circular_buffer_start;         /*!< @brief Pointer to the circular buffer start */
    uint32_t     cb_size;                        /*!< @brief circular buffer total size */
    uint8_t      *cb_read_ptr;                   /*!< @brief circular buffer read pointer */
    uint8_t      *cb_write_ptr;                  /*!< @brief circular buffer read pointer */
    uint8_t      *circular_buff_end;             /*!< @brief circular buffer end pointer */
    pthread_mutex_t    sfm_circular_buffer_mutex; /*!< @brief mutex to protect circular buffer read and write */
} SFM_CircularBuffer;

/** @brief  writes data into circular buffer
 *
 * @param[in] circular buffer Pointer
 * @param[in] input buffer pointer from where the data has to be copied to CB
 * @param[in] Size of the data to be copied
 *
 * @return size if successfull
 * @return -1 if there is no space in circular buffer
 *
 */
int SFM_CircBuff_WritetoCB(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size);

/** @brief  reads data from circular buffer
 *
 * @param[in] circular buffer Pointer
 * @param[in] ouput buffer pointer to where the data has to be copied from CB
 * @param[in] Size of the data to be copied
 *
 * @return size if successfull
 * @return -1 if there is no data available in circular buffer
 */
int SFM_CircBuff_ReadFromCB(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size);

/** @brief  reads data from circular buffer without updating read pointer
 *
 * @param[in] circular buffer Pointer
 * @param[in] ouput buffer pointer to where the data has to be copied from CB
 * @param[in] Size of the data to be copied
 *
 * @return size if successfull
 * @return -1 if there is no data available in circular buffer
 */
int32_t SFM_CircBuff_Peek(SFM_CircularBuffer *cbuffer, uint8_t *buffer, size_t size);

/** @brief  resets read/write pointers of circular buffer to circular buffer start.
 *
 * @param[in] circular buffer Pointer
 *
 * @return void
 */
void SFM_CircBuff_Flush(SFM_CircularBuffer *cbuffer );

/*
** @}
** @}
*/
#endif /* ifndef SFM_INTERNAL_COMMON_H */


