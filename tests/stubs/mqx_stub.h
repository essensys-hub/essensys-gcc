#ifndef MQX_STUB_H
#define MQX_STUB_H
typedef struct { int dummy; } MUTEX_STRUCT;
typedef unsigned int _mqx_uint;
#define MQX_OK 0
#define _mutex_init(m, a)    (MQX_OK)
#define _mutex_lock(m)       (MQX_OK)
#define _mutex_unlock(m)     (MQX_OK)
#define _mutex_destroy(m)    (MQX_OK)
#endif
