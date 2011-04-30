#include "isdnlogin.h"

extern global_t global;

/*******************************************************************
 *
 *******************************************************************/
int pollset( fd, events, func)
int fd; 
int events;
int (*func)PROTO((int));
{
    int i;
    struct pollfd *pfp;
    pollag_t *pap;
 
    if (global.npollfds >= MAX_FD) return -1;
 
    for (i=0, pfp=global.pollfds, pap=global.pollags; 
	    i < global.npollfds; 
	    ++i, ++pfp, ++pap) 
    {
        if (pfp->fd == fd && pfp->events == events) break;
    }
    if ( i < global.npollfds) {
        printf("poll already set !!\n");
        return 0;
    }
 
    pfp = global.pollfds + global.npollfds;
    pap = global.pollags + global.npollfds;
 
    pfp->fd      = fd;
    pfp->events  = events;
    pfp->revents = 0;
    if (poll(pfp, 1, 0) == -1) return -1;
 
    pap->func    = func;
 
    global.npollfds ++;
    return 0;
}

/*******************************************************************
 * 
 *******************************************************************/
int pollloopt(t)
long t;
{
    struct pollfd *pfp;
    pollag_t *pap;
    int i;
    int fds;
    int donefds;
 
    while (global.npollfds > 0) {
	fds = poll(global.pollfds, global.npollfds, t);
        switch (fds) {
            case -1:
                if (errno == EINTR) continue;
                if (errno == EAGAIN) continue;
                return -1;
            case 0:
                return 0;
            default:
                pfp = global.pollfds;
                pap = global.pollags;
		donefds = 0;
                for (i=0; i < global.npollfds; ++i, ++pfp, ++pap) {
                    if (pfp->revents) {
                        (*pap->func)(pfp->fd);
                        pfp->revents = 0;
			donefds++;
                    }
                }
                return donefds;
                break;
        }
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int mypolldel(fd)
int fd;
{
    struct pollfd *pfp;
    pollag_t *pap;
    int i;
 
    for (i=0, pfp=global.pollfds, pap=global.pollags; 
	    i < global.npollfds; 
	    ++i, ++pfp, ++pap) {
        if (pfp->fd == fd) break;
    }
 
    if (i >= global.npollfds) {
        errno = ENOENT;
        return -1;
    }
 
    for (++i, ++pfp, ++pap; i < global.npollfds; ++i, ++pfp, ++pap) {
        pfp[-1] = pfp[0];
        pap[-1] = pap[0];
    }
    global.npollfds --;
    return 0;
}
 

