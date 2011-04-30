#include "isdnlogin.h"

extern global_t global;

int tty_event(int fd)
{
    size_t	len;
    call_t	*ptrCall = global.calls;
    static unsigned short usDataHandle;
    char	out[REG_CAPI_DATA_BLK_SIZE+1];
    char	*c = &out[1];

    len = read(fd, c, sizeof(out));
    /* check for read errors */
    if (len <=0) {
	switch(errno) {
	    case EBADF:
		printf("read: bad filedescriptor!\n");
		return 0;
	    case EFAULT:
		printf("read: Buf points outside the allocated address space.\n");
		return 0;
	    case EIO:
		printf("read: An I/O error occurred while reading from the file system.\n");
		return 0;
	    case EINTR:
		printf("read: interrupted!\n");
		return 0;
	    case EINVAL:
		printf("read: The pointer associated with d was negative.\n");
		return 0;
	    case EAGAIN:
		printf("read: no data to read.\n");
		return 0;
	    default:
		printf("got read error = %d\n", len);
		return 0;
		break;
	}
    }

    if (TTYS_ESCAPE_RECV == global.tty_state) {
	switch (*c) {
	    case TERMINATE_CHAR:
		global.endloop = E_LocalTerminate;
		break;
#if WITH_UNFINISHED_XMODEM
	    case SENDFILE_CHAR:
		/* this feature is not really working now, the first shot
		 * showed that it's much too slow for ISDN connections. So
		 * maybe i will never implement this. Also the prompt()
		 * feature breaks keepalive feature. readbyte() in xmodem.c
		 * won't work on other messages than DATAB3_IND, so a
		 * disconnect while in transfer will be ignored. Feel free to 
		 * integrate this feature */
		if (ptrCall) {
		    /* send first data now */
		    /* TODO: to be implemented here using prompt() function */

		    char buf[256];
		    prompt("Local file name? ", buf, sizeof(buf));
		    printf("buf=%s\n", buf);

		    ptrCall->xmodem = xmodem_open( ptrCall, 115200, 1, ptrCall->error_msg,
			    buf);
		    while ( SendXmodemData(ptrCall) );

		    if (ptrCall->error_msg) printf("XMODEM error: %s\n", ptrCall->error_msg);
		}
		break;
#endif
	    default:
		/* nothing */
		len++;
		out[0] = ESCAPE_CHAR;
		c = &out[0];
		break;

	}
	global.tty_state = TTYS_ESCAPE_WAITFOR;
    } else if (ESCAPE_CHAR == *c) {
	/* eat the tilde */
	global.tty_state = TTYS_ESCAPE_RECV;
	return 0;
    }


    /* send data if in right state */
    if ((ptrCall) && ptrCall->state == Connected) {
	ptrCall->NotAcknowledged++;		/* count pending confirms */
	
	usDataHandle++;
	capi2_datab3_req( ptrCall->capi_fd,
		ptrCall->ident,
		c,
		len,
		0,
		usDataHandle);
    } 

    return 0;
}

/* copied from /usr/src/lib/libc/gen/termios.c of FreeBSD because this
 * is no POSIX compatible function and not available on any OS such as
 * HP-UX or Solaris */

/*
 * Make a pre-existing termios structure into "raw" mode: character-at-a-time
 * mode with no characters interpreted, 8-bit data path.
 */
void mymakeraw(struct termios *t)
{
    t->c_iflag &= ~(IMAXBEL|IXOFF|INPCK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IGNPAR);
    t->c_iflag |= IGNBRK;
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON|ISIG|IEXTEN|NOFLSH|TOSTOP|PENDIN);
    t->c_cflag &= ~(CSIZE|PARENB);
    t->c_cflag |= CS8|CREAD;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}



void reset_terminal(void) {
    /* restore old terminal opts */
    tcsetattr(0,TCSANOW, &(global.oldtermopts));
}

void set_raw_terminal(void) {
    struct termios scheisse;

    /* set terminal to raw mode */
    memcpy(&scheisse, &(global.oldtermopts), sizeof(scheisse));
    mymakeraw(&scheisse);
    tcsetattr(0,TCSANOW, &scheisse);
}

void prompt(char *promptmsg, char *inputbuf, size_t buf_len)
{
    char *bufptr = inputbuf;

    reset_terminal();
    mypolldel(STDIN);

    printf("%s", promptmsg);
    while ((*bufptr = getchar())) {
	if (*bufptr == '\n') break;
	if (buf_len-- <= 0) break;

	//printf("bufptr=%c buf_len=%d\n", *bufptr, buf_len);
	bufptr++;
    }
    *bufptr = '\0';
   
    //printf("end of conversation\n");
    pollset( STDIN, POLLIN, tty_event);
    set_raw_terminal();
}
