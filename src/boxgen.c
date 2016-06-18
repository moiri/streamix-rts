#include "boxgen.h"
#include "boximpl.h"
#include <stdio.h>
#include <pthread.h>
#include <zlog.h>

enum COM_STATE { SYN, SYN_ACK, ACK, DONE };

void* box_impl_a( void* handler )
{
    dzlog_info("start thread a");
    a( handler );
    dzlog_info("end thread a");
    pthread_exit( NULL );
}

void* box_impl_b( void* handler )
{
    dzlog_info("start thread b");
    b( handler );
    dzlog_info("end thread b");
    pthread_exit( NULL );
}
