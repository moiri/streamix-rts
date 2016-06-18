#include "boxgen.h"
#include "boximpl.h"
#include <stdio.h>
#include <pthread.h>

enum COM_STATE { SYN, SYN_ACK, ACK, DONE };

void* box_impl_a( void* handler )
{
    a( handler );
    pthread_exit( NULL );
}

void* box_impl_b( void* handler )
{
    b( handler );
    pthread_exit( NULL );
}
