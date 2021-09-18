#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<time.h>
#include"cir_buf.h"

#define CIRC_BUFF_SIZE    32

uint8_t    peek_buffer[CIRC_BUFF_SIZE];
uint8_t * prod_buffer;
uint8_t * cons_buffer;
unsigned int consumer_buffer_ptr = 0;
unsigned int producer_buffer_ptr = 0;
unsigned int inputbuffersize;

static SFM_CircularBuffer cb;


static void *producer(void *arg)
{
    int ret_bytes,req_bytes;

    SFM_CircularBuffer *cb = arg;
    while(1)
    {
        req_bytes = rand() % CIRC_BUFF_SIZE;
        
        if(req_bytes <= (inputbuffersize - producer_buffer_ptr ))
        {
            if((ret_bytes = SFM_CircBuff_WritetoCB(cb, (prod_buffer + producer_buffer_ptr), req_bytes)) != req_bytes)
            {
                printf("No space to write %d bytes line: %d\n",req_bytes, __LINE__);
            } 
            else
            {
                producer_buffer_ptr += req_bytes;
                printf("%d bytes written into cb line: %d producer_buffer_ptr %d\n", ret_bytes, __LINE__, producer_buffer_ptr);
            }
        }
        else
        {
            if((ret_bytes = SFM_CircBuff_WritetoCB(cb, (prod_buffer+ producer_buffer_ptr), (inputbuffersize - producer_buffer_ptr))) != (inputbuffersize - producer_buffer_ptr))
            {
                printf("No space to write %d bytes line: %d\n",(inputbuffersize - producer_buffer_ptr), __LINE__);
            } 
            else
            {
                producer_buffer_ptr += (inputbuffersize - producer_buffer_ptr);
                printf("%d bytes written into cb line: %d producer_buffer_ptr %d\n", ret_bytes, __LINE__, producer_buffer_ptr);
                break;
            }
        }
        
        sleep(1);

        SFM_CircBuff_Stat(cb);
    }

}

static void * consumer(void *arg)
{
    int ret_bytes, req_bytes;

    SFM_CircularBuffer *cb = arg;
    while(1)
    {

        req_bytes = rand() % CIRC_BUFF_SIZE;

        if(req_bytes <= (inputbuffersize - consumer_buffer_ptr))
        {
            if((ret_bytes = SFM_CircBuff_ReadFromCB(cb, (cons_buffer + consumer_buffer_ptr), req_bytes)) != req_bytes)
            {
                printf("No enough data  to read %d bytes line: %d\n", req_bytes, __LINE__);
            }
            else
            {
                consumer_buffer_ptr += req_bytes;
                printf("%d bytes read from cb line:%d consumer_buffer_ptr %d\n",ret_bytes, __LINE__, consumer_buffer_ptr);
            }
        }
        else
        {
            if((ret_bytes = SFM_CircBuff_ReadFromCB(cb, (cons_buffer + consumer_buffer_ptr), 
                        (inputbuffersize - consumer_buffer_ptr))) != (inputbuffersize - consumer_buffer_ptr))
            {
                printf("No enough data  to read %d bytes line: %d\n", (inputbuffersize - consumer_buffer_ptr), __LINE__);
            }
            else
            {
                consumer_buffer_ptr += (inputbuffersize - consumer_buffer_ptr);
                printf("%d bytes read from cb line:%d consumer_buffer_ptr %d\n",ret_bytes, __LINE__, consumer_buffer_ptr);
            }
        }

        if(memcmp(prod_buffer,cons_buffer, consumer_buffer_ptr) == 0)
        {
            printf("%d bytes of DATA ARE SAME\n", consumer_buffer_ptr );
        }
        else
        {
            printf("producer and consumer buffer are different. data read %d\n", consumer_buffer_ptr);
#ifdef    _DEBUG_DATA_
            printf("Producer buffer\n");
            for(int i=0; i<consumer_buffer_ptr;i++)
                printf("0x%02x ", prod_buffer[i]);
            
            printf("\nConsumer buffer\n");
            for(int i=0; i<consumer_buffer_ptr;i++)
                printf("0x%02x ", cons_buffer[i]);
            printf("\n");
#endif /* _DEBUG_DATA_ */
        }

        sleep(1);

        req_bytes = rand() % CIRC_BUFF_SIZE;
        
        /* PEEK TEST */
        if((ret_bytes = SFM_CircBuff_Peek(cb, peek_buffer, req_bytes)) != req_bytes)
        {    
            printf("PEEK : No enough data to read %d bytes line: %d\n", req_bytes, __LINE__);
        }
        else
        {
            printf("%d bytes peek from cb line:%d\n", ret_bytes, __LINE__);
            /* We are sure that we can read req_bytes since peek is successful */
            SFM_CircBuff_ReadFromCB(cb, (cons_buffer + consumer_buffer_ptr), req_bytes );
            
            if(memcmp(peek_buffer, (cons_buffer + consumer_buffer_ptr), req_bytes) == 0)
            {
                printf("PEEK BUFFER has valid data\n");    
            }    
            else
            {
                printf("PEEK BUFFER has invalid buffer\n");
            }
    
            consumer_buffer_ptr += req_bytes;
                
        }




        if(consumer_buffer_ptr == inputbuffersize)
        {
            FILE *fp = fopen("out", "wb");
            fwrite(cons_buffer,inputbuffersize,1,fp);
            fclose(fp);
            
            fp = fopen("in_cir", "wb");
            fwrite(prod_buffer,inputbuffersize,1,fp);
            fclose(fp);

            break;
        }

    }
}


int load_file_into_memory(void)
{

    FILE *input_fd = fopen("test", "rb");

    if(input_fd == NULL)
    {
        printf("Unable to open file\n");
        return -1;
    }
    fseek(input_fd, 0, SEEK_END);
    long fsize = ftell(input_fd);
    fseek(input_fd, 0, SEEK_SET); 

    prod_buffer = malloc(fsize );
    if(prod_buffer == NULL)
    {
        printf("Unable to allocate buffer for file\n");
        return -1;
    }

    fread(prod_buffer, fsize, 1, input_fd);

    cons_buffer = malloc(fsize);
    if(cons_buffer == NULL)
    {
        printf("Unable to allocate buffer for file\n");
        return -1;
    }

    inputbuffersize = fsize;
    

    fclose(input_fd);

    return 0;

}

int main()
{

    pthread_t    producer_thread;
    pthread_t    consumer_thread;        

    /*initialize circular buffer*/
        cb.circular_buffer_start = malloc(CIRC_BUFF_SIZE);
        cb.cb_size = CIRC_BUFF_SIZE;
        cb.cb_read_ptr = cb.circular_buffer_start;
        cb.cb_write_ptr = cb.circular_buffer_start;
        cb.circular_buff_end = cb.circular_buffer_start + CIRC_BUFF_SIZE -1;

    if(load_file_into_memory() != 0)
    {
        exit(0);
    }

    srand(time(NULL));

    pthread_mutex_init(&cb.sfm_circular_buffer_mutex, NULL);


    if(pthread_create(&producer_thread, NULL, producer, &cb))
    {
        printf("Error creating thread\n");
        return 1;
    }

    if(pthread_create(&consumer_thread, NULL, consumer, &cb))
    {
        printf("Error creating thread\n");
        return 1;
    }

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

}


