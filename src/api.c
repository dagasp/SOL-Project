#include "api.h"
#include "util.h"
#include "hash.h"

int openConnection(const char *sockname, int msec, const struct timespec abstime) {
    errno = 0;
    int fd_skt;
    int r = 0; 
    struct sockaddr_un sa;
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, sockname,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;
    SYSCALL_RETURN("socket", fd_skt,socket(AF_UNIX,SOCK_STREAM,0), "Errore nella creazione del socket\n");
    const struct timespec current_time;
    SYSCALL_RETURN("clock_getime, "r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n");
    while (r = connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa)) != 0 && abstime.tv_sec > current_time.tv_sec) {
        usleep(msec * 1000);
        r = 0;
        SYSCALL_RETURN("clock_getime, "r, clock_gettime(CLOCK_REALTIME, &current_time), "Errore in clock_gettime\n");
    }
    if (r != -1)
        return 0;
    //settare errno;
}
