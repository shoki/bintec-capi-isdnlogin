/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: XMODEM transfer functions
 *      Author: bernd
 *    $RCSfile: xmodem.c,v $
 *   $Revision: 74 $
 *       $Date: 2005-12-22 01:06:50 +0100 (Thu, 22 Dec 2005) $
 *      $State: Exp $
 *     
 *        Type: library
 *    Products: ALL
 *  Interfaces: -
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 ***********************************************************************/

#ifdef USE_RCSID_C
    static const char _rcsid_xmodem_c[] __UNUSED = "$Id: xmodem.c,v 1.2 2003/07/30 11:26:56 shoki Exp $";
#endif

#if WITH_UNFINISHED_XMODEM

#include "isdnlogin.h"

#define TRACE		printf
static void trace(const char *fmt, ...)
{ }



/*
 * CRC-16 constant array... from Usenet contribution by Mark G. Mendel,
 * Network Systems Corp. (ihnp4!umn-cs!hyper!mark)
 */
static unsigned short crctab[1 << B] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/*******************************************************************************
 * function: readbyte
 *	Read one byte from data stream (with timeout)
 * parameters:
 *	fd	 i/o pathes of data stream
 *	seconds  timeout
 * return: byte or TIMEOUT
 ******************************************************************************/
static int readbyte(rwfd_t fd, int seconds)
{
    struct pollfd pfd;
    unsigned char c;
    unsigned char	proto[1024];
    unsigned char	data[4096];
    union CAPI_primitives *capi;
    int lcause;

    do {
	if (capi2_wait_for_signal(fd.rd, seconds * 1000 ) == CAPI2_E_MSG_QUEUE_EMPTY) {
	    printf("readbyte timeout %d\n", seconds);
	    return TIMEOUT;
	} else {
	    lcause = capi2_get_message( fd.rd, &capi,
		    (union CAPI_primitives *)proto, &c, 1);
	    //printf("capi=%x\n", GET_PRIMTYPE(capi));
	}
    } while (GET_PRIMTYPE(capi) != CAPI2_DATAB3_IND);

    capi2_datab3_resp( fd.rd,
	    GET_MESSID(capi),
	    fd.ident,
	    GET_WORD(capi->c2datab3_ind.handle));
    return c;
}

/*******************************************************************************
 * function: readdata
 *	Read block of data from given i/o pathes (with timeout)
 * parameters:
 *	x	  xmodem handle
 *	bufsize	  receiver buffer size
 *	checksum  accumulated crc
 * return: data byte or TIMEOUT
 ******************************************************************************/
static int readdata(xmodem_t *x, int bufsize, int *checksum)
{
    int         c;		/* character being processed */
    unsigned short chksm;	/* working copy of checksum */
    int         j;		/* loop index */

    chksm = 0;
    for (j = 0; j < bufsize; j++) {
	if ((c = readbyte(x->fd, 1)) == TIMEOUT) {	/* 1s timeout */
	    return TIMEOUT;
	}
	x->buf[j] = c & 0xff;
	if (x->crcmode)		/* CRC */
	    chksm = (chksm << B) ^ crctab[(chksm >> (W - B)) ^ c];
	else			/* checksum */
	    chksm = ((chksm + c) & 0xff);
    }
    *checksum = chksm;

    TRACE("XMODEM read (%d bytes)", j);    
    return (0);
}

/*******************************************************************************
 * function: sendbyte
 *	Write one byte to data stream
 * parameters:
 *	fd	 i/o pathes of data stream
 *	data	 data byte
 * return: -
 ******************************************************************************/
static void sendbyte(rwfd_t fd, char data)
{
    TRACE("XMODEM write (%d bytes)", 1);    
    //write(fd.wr, &data, 1);
    capi2_datab3_req( fd.wr, fd.ident, &data, 1, 0, fd.handle);
}

/*******************************************************************************
 * function: purge
 *	Wait for the line to clear (read rest)
 * parameters:
 *	fd	 i/o pathes of data stream
 * return: -
 ******************************************************************************/
static void purge(rwfd_t fd)
{
    int c;
    while ((c = readbyte(fd, 1)) != TIMEOUT) ;
}

/*******************************************************************************
 * function: purge
 *	Send CAN twice
 * parameters:
 *	fd	 i/o pathes of data stream
 * return: -
 ******************************************************************************/
static void cancel(rwfd_t fd)
{
    sendbyte(fd, CAN);
    sendbyte(fd, CAN);
    purge(fd);
    sendbyte(fd, CAN);
    sleep(1);
}

/*******************************************************************************
 * function: xmodem_open
 *	Open console and set baud rate
 * parameters:
 *	dev     device name (e.g. "consexcl")
 *              or NULL for stdio (baud rate ignored!)
 *	baud    baud rate [baud]
 *      one_kb  xmodem-1k mode enable
 *      emsg	buffer for error message (80 bytes)
 * return: xmodem ptr or error (NULL)
 ******************************************************************************/
xmodem_t *xmodem_open(call_t *ptrCall, int baud, int one_kb, char *emsg, char *filename)
{
    int fd;
    xmodem_t *x = malloc(sizeof *x);

    if (!x) {
	if (emsg) sprintf(emsg, "XMODEM: out of memory");
	return 0;
    }
    memset(x, 0, sizeof *x);

    reset_terminal();					/* unraw terminal */

    if (ptrCall) {
	/*------------------------------+
	|  i/o via stdio pathes         |
	+------------------------------*/
	TRACE("XMODEM via CAPI");
	x->conmode = 0;
	x->fd.rd = ptrCall->capi_fd;			/* use stdio pathes */
	x->fd.wr = ptrCall->capi_fd;
	x->fd.ident = ptrCall->ident;
    } else {
	TRACE("XMODEM via stdio");
	x->conmode = 0;
	x->fd.rd = STDIN;
	x->fd.wr = STDOUT;
    }

    if (filename) {
	x->sendfp = fopen(filename, "r");
	/* XXX: check for fopen errors here */
	if (!x->sendfp) {
	    /* failure */
	    return 0;
	}
    } else {
	TRACE("XMODEM no filename to send specified");
    	return 0;
    }

    x->failed 	  = FALSE;
    x->crcmode    = TRUE;
    x->rcvstartch = x->crcmode ? CRCCHR : NAK;
    x->one_kb	  = one_kb & 1;	/* only relevant for sending packets */
    x->starttime  = time(NULL);

    return x;
}

/*******************************************************************************
 * function: xmodem_onekb
 *	Return if modem-1k mode enabled
 * parameters:
 *	x	xmodem ptr
 * return: modem-1k mode enabled (1) or not (0)
 ******************************************************************************/
int xmodem_onekb(xmodem_t *x)
{
    return x->one_kb;
}

/*******************************************************************************
 * function: xmodem_cancel
 *	Send CAN to stop an unterminated getpkt transfer
 * parameters:
 *	x	xmodem ptr
 * return: -
 ******************************************************************************/
void xmodem_cancel(xmodem_t *x)
{
    cancel(x->fd);
}

/*******************************************************************************
 * function: xmodem_close
 *	Close device and free memory
 * parameters:
 *	x	xmodem ptr
 * return: success (0) or error code
 ******************************************************************************/
void xmodem_close(xmodem_t *x)
{
    time_t	tdiff;

    if (x->fd.rd != -1) {
	if (x->conmode) {
	    close(x->fd.rd);
	}
    }
    if (x->sendfp) {
	fclose (x->sendfp);	/* close opened file to send */
	x->sendfp = NULL;
    }
    x->fd.rd = -1;

    if (x->starttime) {
	tdiff = time(NULL) - x->starttime;
	printf("duration: %d\n", tdiff );
    }

    free(x);
    set_raw_terminal();		/* enable raw terminal mode */
}
#ifdef USE_UNPORTED
/*******************************************************************************
 * function: xmodem_getpkt
 *	Receive one XMODEM-Packet and write it into buf
 *
 *	The length of the packet is written into (*lenp).
 *      An error message is written into emsg. 
 *  	The length of data packets (128/1024) is determined by the remote site
 * parameters:
 *	x	xmodem ptr
 *	buf	data packet buffer (1024 bytes)
 *	lenp	length of the data packet
 *	emsg	buffer for error message (80 bytes)
 * return: status code
 * 	XMODEM_RCVPKT:	successfully received one packet
 * 	XMODEM_EOT:	received End-of-Transmission character
 * 	XMODEM_ERROR:	receive failed
 ******************************************************************************/
int xmodem_getpkt(xmodem_t *x, char *buf, int *lenp, char *emsg)
{
    int         checksum,	/* packet checksum                            */
                firstchar,	/* first character of a packet                */
                sectcurr,	/* second byte of packet--should be packet    */
				/* number (mod 128)                           */
                sectcomp,	/* third byte of packet--should be complement */
	                        /* of sectcurr                                */
                errorflag,	/* set true when packet (or first char of     */
	                        /* putative packet) is invalid                */
                inchecksum,	/* incoming checksum or CRC                   */
                msb, lsb,	/* incoming checksum (upper/lower byte)       */
	        duplicate,	/* got duplicate packet                       */
	        timeout,	/* read timeout		                      */
                bufsize;	/* packet size (128 or 1024)                  */
                

    sendbyte(x->fd, x->rcvstartch);

  restart:
    bufsize = 128;
    errorflag = FALSE;
    duplicate = FALSE;
    timeout = FALSE;
    *lenp = 0;

    do {			/* start by reading first byte in packet */
	if ((firstchar = readbyte(x->fd, 4)) == TIMEOUT) {
	    errorflag = timeout = TRUE;
	    goto abort;
	}
    } while ((firstchar != SOH)
	     && (firstchar != STX)
	     && (firstchar != EOT)
	     && (firstchar != ACK || x->recvsectcnt > 0)
	     && (firstchar != CAN));

    if (firstchar == EOT) {	/* check for REAL EOT */
	TRACE("EOT");
	if (readbyte(x->fd, 1) != TIMEOUT) {
	    errorflag = timeout = TRUE;
	    TRACE("ignoring extra characters");
	    goto abort;
	}	
    }
    if (firstchar == CAN) {	/* bailing out? */
	TRACE("CAN");
	if ((readbyte(x->fd, 3) & 0x7f) == CAN) {
	    sleep(1);
	    if (emsg) strcpy(emsg, "XMODEM: canceled by user");
	    return XMODEM_ERROR;
	} else {
	    errorflag = TRUE;
	    goto abort;
	}
    }
    if (firstchar == SOH || firstchar == STX) {	/* start reading packet */
	bufsize = (firstchar == SOH) ? 128 : 1024;
	
	if (x->recvsectcnt == 0) {	/* 1st data packet, initialize */
	    if (bufsize == 1024) {
		TRACE("1K packet mode chosen");
	    }
	    x->errors = 0;
	    x->got_eot = 0;
	}
	if ((sectcurr = readbyte(x->fd, 3)) == TIMEOUT  ||
	    (sectcomp = readbyte(x->fd, 3)) == TIMEOUT) {
	    errorflag = timeout = TRUE;
	    goto abort;
	}
	TRACE("%s: sectcurr=0x%02x sectcomp=0x%02x",
	      firstchar==SOH ? "SOH":"STX", sectcurr, sectcomp);
	if ((sectcurr + sectcomp) == 0xff) {	/* is packet number
						 * checksum correct? */
	    if (sectcurr == ((x->sectnum + 1) & 0xff)) {	
		/*
		 * Read, process and calculate checksum for a buffer of
		 * data
		 */
		if ((readdata(x, bufsize, &checksum)) == TIMEOUT) {
		    errorflag = timeout = TRUE;
		    goto abort;
		}
		/* verify checksum or CRC */
		if (x->crcmode) {
		    checksum &= 0xffff; 
		    /* get 16-bit CRC */
		    if ((msb = readbyte(x->fd, 3)) == TIMEOUT  ||
			(lsb = readbyte(x->fd, 3)) == TIMEOUT) {
			errorflag = timeout = TRUE;
			goto abort;
		    }
		    inchecksum = (msb << 8) | lsb;
		} else {
		    /* get simple 8-bit checksum */
		    if ((inchecksum = readbyte(x->fd, 3)) == TIMEOUT) {
			errorflag = timeout = TRUE;
			goto abort;
		    }
		}
		    
		if (inchecksum == checksum) {	/* good checksum, hence
						 * good packet */
		    x->errors = 0;
		    x->recvsectcnt += (bufsize == 128) ? 1 : 8;
		    x->sectnum = sectcurr;
		    memcpy(buf, x->buf, bufsize);
		    *lenp = bufsize;
		    x->rcvstartch = ACK;
		}
		/*
		 * Start handling various errors and special
		 * conditions
		 */
		else {	/* bad checksum */
		    TRACE("checksum error  crcmode=%d: inchecksum=%x crc=%x", 
			  x->crcmode, inchecksum, checksum);
		    errorflag = TRUE;
		}
	    } else {	/* sector number is wrong */
		if (sectcurr == x->sectnum) {	/* duplicate sector? */
		    errorflag = TRUE;
		    duplicate = TRUE;
		    TRACE("duplicate sector");
		    while (readbyte(x->fd, 3) != TIMEOUT);
		} else {	/* no, real phase error */
		    errorflag = TRUE;
		    x->failed = TRUE;
		    cancel(x->fd);
		    TRACE("phase error, expected sector=%d", x->recvsectcnt);
		}
	    }
	} else {		/* bad packet number checksum */
	    TRACE("wrong header sector number");
	    errorflag = TRUE;
	}
    }			/* END reading packet loop */

 abort:
    if (timeout) TRACE("timeout sector=%d", x->recvsectcnt);
    if ((errorflag && !x->failed) || x->recvsectcnt == 0) {	
	if (errorflag) x->errors++;
	if (x->recvsectcnt != 0) {
	    while (readbyte(x->fd, 3) != TIMEOUT) ;/* wait for line to settle if
						    * not beginning */
	}
	if (duplicate) {
	    sendbyte(x->fd, ACK);
	} else if (x->crcmode && x->recvsectcnt == 0 && x->errors == CRCSWMAX) {
	    x->crcmode = FALSE;
	    TRACE("CRC mode not accepted");
	    purge(x->fd);
	    sendbyte(x->fd, NAK);
	} else if (!x->crcmode && x->recvsectcnt == 0 && x->errors == CRCSWMAX){
	    x->crcmode = TRUE;
	    TRACE("checksum mode not accepted");
	    purge(x->fd);
	    sendbyte(x->fd, CRCCHR);
	} else if (x->crcmode && x->recvsectcnt == 0) {
	    purge(x->fd);
	    sendbyte(x->fd, CRCCHR);
	} else {
	    purge(x->fd);
	    sendbyte(x->fd, NAK);
	}
    }

    if (firstchar == EOT) {
	x->got_eot = 1;
	sendbyte(x->fd, ACK);
	sleep(1);
	return XMODEM_EOT;
    } else if (x->failed || x->errors >= ERRORMAX) {
	cancel(x->fd);
	x->failed = 1;
	if (emsg) strcpy(emsg, "XMODEM: too many errors");
	return XMODEM_ERROR;
    } else if (errorflag) {
	goto restart;
    } else {
	return XMODEM_RCVPKT;
    }
}
#endif

/*******************************************************************************
 * function: xmodem_putpkt
 *	Send one XMODEM packet
 *
 *	The protocol packet length is 128 or 1024 bytes padded with CTRL-Z.
 *      If len argument is not 128/1024, this is the last packet and an EOT is
 *  	sent to the remote site after sending the data packet.
 *  	If the last packet sent had 128/1024 bytes, the application must invoke
 *  	this funktion again with len=0 to indicate EOT.
 * parameters:
 *	x	xmodem ptr
 *	buf	data packet buffer (1024 bytes)
 *	len	length of the data packet
 *	emsg	buffer for error message (80 bytes)
 * return: 0 if error
 ******************************************************************************/
int xmodem_putpkt(xmodem_t *x, char *data, int len, char *emsg)
{
    int bufctr; 		/* array index for data buffer              */
    unsigned short checksum; 	/* checksum/crc                             */
    int	attempts,		/* number of attempts made to xmit a packet */
	sendfin, 		/* flag that we are sending the last packet */
	bbufcnt,		/* array index for packet                   */
	firstchar,		/* first character in protocol transaction  */
	bufsize,		/* packet size (128 or 1024)                */
	sendresp;  		/* response char received from remote       */
    char *cp = data;

    bufsize = x->one_kb ? 1024 : 128;  
    sendfin = FALSE;
    attempts = 0;

    if (!x->got_ack) {	      /* sending first packet */
	x->crcmode = FALSE;   /* Receiver determines use of crc or checksum */
	x->sectnum = 1;
	do {
	    /* wait for and read startup character */
	    while (
		(firstchar = readbyte(x->fd, 30)) != NAK
		&& firstchar != CRCCHR
		&& firstchar != CAN
	    ) {
		if (++attempts > NAKMAX) {
		    if (emsg) strcpy(emsg, 
				     "XMODEM: remote system not responding");
		    x->failed = 1;
		    return 0;
		}
	    }
	    if ((firstchar & 0x7f) == CAN) {
		TRACE("CAN");
		if (readbyte(x->fd, 3) == CAN) {
		    if (emsg) sprintf(emsg, "XMODEM: canceled by user");
		    x->failed = 1;
		    return 0;
		}
	    }
	    if (firstchar == CRCCHR) {
		x->crcmode = TRUE;
		TRACE("CRC mode requested");
	    }
	} while (firstchar != NAK && firstchar != CRCCHR);
    }

    if (len == 0) goto sendeot;

    bbufcnt = 0;		/* start building buf to be sent */
    x->buf[bbufcnt++] = (bufsize == 1024) ? STX : SOH;
    x->buf[bbufcnt++] = x->sectnum;	    /* current sector # */
    x->buf[bbufcnt++] = ~x->sectnum;   /* and its complement */

    checksum = 0;
    for (bufctr=0; bufctr < bufsize; bufctr++) {
	if (bufctr >= len) {
	    sendfin = TRUE;  /* this is the last sector */
	    x->buf[bbufcnt + bufctr] = CTRLZ;  /* Control-Z for CP/M EOF */
	} else {
	    x->buf[bbufcnt + bufctr] = *cp++;
	}
	if (x->crcmode) {
	    checksum = (checksum<<B) ^ crctab[(checksum>>(W-B)) ^ x->buf[bbufcnt + bufctr]];
	} else {
	    checksum = ((checksum + x->buf[bbufcnt + bufctr]) & 0xff);
	}
    }
    bbufcnt += bufsize;

    if (x->crcmode) {		/* put in CRC */
	checksum &= 0xffff;
	x->buf[bbufcnt++] = ((checksum >> 8) & 0xff);
	x->buf[bbufcnt++] = (checksum & 0xff);
    } else {			/* put in checksum */
	x->buf[bbufcnt++] = checksum;
    }

    attempts = 0;
    do {				/* inner packet loop */
	TRACE("XMODEM write (%d bytes) ", bbufcnt);    
	//write(x->fd.wr, x->buf, bbufcnt);	/* write the buf */
	x->fd.handle++;			/* increase data handle var for capi */
	capi2_datab3_req( x->fd.wr, x->fd.ident, x->buf, bbufcnt, 0, x->fd.handle);

	TRACE("%s: sectcurr=0x%02x", x->buf[0]==SOH ? "SOH":"STX", x->sectnum);
#if 0
	TRACE("%d byte %02xh (%02xh) sent, checksum %02xh %02xh secnum %d\n", 
	      bbufcnt, x->buf[1]&0xff, x->buf[2]&0xff, x->buf[bufsize+3]&0xff,
	      x->buf[bufsize+4]&0xff, x->sectnum
	);
#endif
	attempts++;
	sendresp = readbyte(x->fd, 10);  /* get response from remote */

	if (sendresp != ACK) {
	    x->errors++;
		    
	    if ((sendresp & 0x7f) == CAN) {
		TRACE("CAN");
		if ((readbyte(x->fd, 3) & 0x7f) == CAN) {
		    if (emsg) strcpy(emsg, "XMODEM: canceled by user");
		    x->failed = 1;
		    return 0;
		}
	    }
	    if (sendresp == TIMEOUT) {
		TRACE("timeout on sector %d", x->sentsectcnt);
	    } else {
		TRACE("non-ACK on sector %d", x->sentsectcnt);
	    }
	} else {
	    x->got_ack = 1;
	}
    } while (
	sendresp != ACK && attempts < RETRYMAX && x->errors < ERRORMAX
    );

    x->sectnum++;
    x->sentsectcnt += (bufsize == 128) ? 1 : 8;

    if (attempts >= RETRYMAX) {
	sendbyte(x->fd, CAN); sendbyte(x->fd, CAN); sendbyte(x->fd, CAN);
	if (emsg) strcpy(emsg, "XMODEM: remote system not responding");
	x->failed = 1;
	return 0;
    }

    if (attempts > ERRORMAX) {
	sendbyte(x->fd, CAN); sendbyte(x->fd, CAN); sendbyte(x->fd, CAN);
	if (emsg) strcpy(emsg, "XMODEM: too many errors");
	x->failed = 1;
	return 0;
    }

    if (sendfin) {
      sendeot:
	attempts = 0;
	sendbyte(x->fd, EOT);  /* send 1st EOT to close down transfer */

	/* wait for ACK of EOT */
	while (readbyte(x->fd, 15) != ACK && attempts++ < RETRYMAX) {
	    TRACE("EOT not acked");
	    sendbyte(x->fd, EOT);
	}

	if (attempts >= RETRYMAX) {
	    if (emsg) strcpy(emsg, "XMODEM: last ACK missing");
	    x->failed = 1;
	    return 0;
	}
    }
    return 1;
}

#endif



