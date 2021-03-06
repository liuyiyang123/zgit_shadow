#define ZNETUTILS_H

#ifndef _Z_BSD
    #ifndef _XOPEN_SOURCE
        #define _XOPEN_SOURCE 700
        #define _DEFAULT_SOURCE
        #define _BSD_SOURCE
    #endif
#endif


#include <netdb.h>

#ifndef ZCOMMON_H
#include "zCommon.h"
#endif

struct zNetUtils__ {
    _i (* gen_serv_sd) (char *, char *, _i);

    _i (* tcp_conn) (char *, char *, _i);

    _i (* sendto) (_i, void *, size_t, _i, struct sockaddr *);
    _i (* sendmsg) (_i, struct iovec *, size_t, _i, struct sockaddr *);
    _i (* recv_all) (_i, void *, size_t, _i, struct sockaddr *);

    _ui (* to_bin)(const char *);
    void (* to_str)(_ui, char *);
};


// extern struct zNetUtils__ zNetUtils_;
