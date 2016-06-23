#include "boxgen.h"
#include "boximpl.h"
#include <stdio.h>
#include <zlog.h>

void* box_impl_a( void* handler )
{
    dzlog_info("start thread a");
    a( handler );
    dzlog_info("end thread a");
    return NULL;
}

void* box_impl_b( void* handler )
{
    dzlog_info("start thread b");
    b( handler );
    dzlog_info("end thread b");
    return NULL;
}
