#ifndef HANDLER_H
#define HANDLER_H

#define STRMX_IN( h, idx ) h->value[idx]
#define STRMX_OUT( h, idx, val ) h->value[idx] = val

typedef struct strmx strmx;

struct strmx {
    int box;
    void** value;
};


#endif // HANDLER_H
