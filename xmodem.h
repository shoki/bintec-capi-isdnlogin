/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: xmodem.h,v $
 *   $Revision: 74 $
 *       $Date: 2005-12-22 01:06:50 +0100 (Thu, 22 Dec 2005) $
 *      $State: Exp $
 *     
 *        Type: include file for ..
 *    Products: ALL | XS,XM,XL,XP,BGO,BGP,XCM
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 ***********************************************************************/

#ifndef _xmodem_h
#define _xmodem_h

#ifdef USE_RCSID_H
    static const char _rcsid_xmodem_h[] __UNUSED = "$Id: xmodem.h,v 1.1 2002/12/19 12:53:33 shoki Exp $";
#endif

/*  ASCII Constants  */
#define      SOH  	001 
#define	     STX	002
#define	     ETX	003
#define      EOT	004
#define	     ENQ	005
#define      ACK  	006
#define	     LF		012   /* Unix LF/NL */
#define	     CR		015  
#define      NAK  	025
#define	     SYN	026
#define	     CAN	030
#define	     ESC	033

/*  XMODEM Constants  */
#define      TIMEOUT  	-1
#define      ERRORMAX  	10    /* maximum errors tolerated */
#define      CRCSWMAX	4     /* maximum time to try CRC mode before switching */
#define      KSWMAX	5     /* maximum errors before switching to 128 byte packets */
#define      NAKMAX	2     /* maximum times to wait for initial NAK when sending */
#define      RETRYMAX  	5     /* maximum retries to be made */
#define      CRCCHR	'C'   /* CRC request character */
#define	     CTRLZ	032   /* CP/M EOF for text (usually!) */

#undef DEBUG

/* the CRC polynomial. */
#define	P	0x1021

/* number of bits in CRC */
#define W	16

/* this the number of bits per char */
#define B	8

#define XMODEM_EOT	0	/* End of transmission	*/
#define XMODEM_ERROR	1	/* Too many errors	*/
#define XMODEM_RCVPKT	2	/* Received Packet	*/

#define	     BBUFSIZ	1024    /* buffer size          */

typedef struct {
    int rd;		/* read path                                          */
    int wr;		/* write path                                         */
    unsigned long ident; /* for capi ident */
    unsigned short handle; /* for capi data handle */
} rwfd_t;

typedef struct {
    CONST char	*filename;	/* filename of file to send */
    FILE *sendfp;	/* filepointer to opened file to be sent */
    time_t starttime;   /* when did the transfer start */
    int sectnum;	/* number of last packet (modulo 128)                 */
    int errors;		/* running cnt of errors(reset when 1st packet starts */
    int recvsectcnt;	/* running sector count (128 byte sectors)            */
    int sentsectcnt;
    rwfd_t fd;		/* i/o pathes                                         */
    int ldsav;		/* saved ld mode                                      */
    char rcvstartch;	/* char to request next packet                        */
    unsigned char buf[BBUFSIZ + 6];
    unsigned one_kb:1;	/* 1024 byte mode configured (only sender)            */
    unsigned got_ack:1;
    unsigned crcmode:1;
    unsigned failed:1;
    unsigned got_eot:1;	/* end-of-transmission was received */
    unsigned conmode:1;	/* i/o via console */ 
} xmodem_t;

#endif
