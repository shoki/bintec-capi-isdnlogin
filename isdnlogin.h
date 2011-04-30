#include <signal.h>
#include <sys/ioctl.h>	/* for tcsetattr */
#include <termios.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <libcapi.h>	/* for the fucking types CONST &&& */

#include "xmodem.h"

#define DEBUG 				0

#define STDIN				fileno(stdin)
#define STDOUT				fileno(stdout)

#define CAPI_HOST_DEFAULT		"struppi.dev.bintec.de"
#define	CAPI_USER_DEFAULT		"default"

#define MAX_FD				64
#define CMD_LINE_ARGS			"?hqC:l:c:s:w:W:v:b:p:u:P:"

#define DEFAULT_VERBOSE_LEVEL		2
#define DEFAULT_WAIT4DATACONF		0	/* flag */
#define REGISTER_FAIL_TIMER		30	/* seconds      */

#define	CHECK_CAPI			1	/* keepalive capi and indicate broken connection */
#define MAX_KEEPALIVE_PENDING		0
#define	KEEP_ALIVE_TIMER		5*1000	/* milliseconds	*/

#define LOGIN_SCRIPT_LINE_BY_LINE	0	/* send each login script line
						 * in one data_ind instead of
						 * block based sending 
						 */

#define WITH_UNFINISHED_XMODEM		0	/* enable/disable Xmodem upload feature 
						 *  (*broken*)
						 */
#define WITH_RCAPI_LAYER		0	/* enable/disbale using of rcapi functions
						 * which allow to pass a
						 * hostname and user to the
						 * capi. the problem is that
						 * the code is still buggy and
						 * won't work always */

/* tty stories */
#define	TTYS_ESCAPE_WAITFOR		0
#define	TTYS_ESCAPE_RECV		1
#define	ESCAPE_CHAR			'~'
#define	TERMINATE_CHAR			'.'
#define SENDFILE_CHAR			'>'

#define MAX_CMD_SIZE			256
#define NEWSTATE( ptrCall, newstate) (ptrCall)->state = newstate;

#define REG_CAPI_MSG_SIZE		1024
#define REG_CAPI_LEVEL3CNT		1
#define REG_CAPI_DATA_BLK_SIZE		1400	/* mtu=1500-(tcp+capi)header */
#define REG_WINDOW_SIZE			7	/* TODO with 10 fax aborts */

#define ISDN_HDLC			0	/* ISDN HDLC service mode */
#define V110_ASYNC			1	/* V.110 Async service */
#define TRANS				4	/* transparent */
#define MODEM				5	/* modem service */
#define FAX_G3				6	/* FAX group 3 */
#define ISDN_V42			7	/* HDLC X.75 with V.42bis */

#define CIP_DATA			2
#define CIP_PHONE			1
#define CIP_3KAUDIO			4
#define CIP_FAXG3			17
#define CIP_FAXG4			18
#define CIP_BTX				20	/* not sure about this */

#define TRUE				1
#define FALSE				0

#undef max
#undef min
#define max(a,b)   (((a)>(b))?(a):(b))
#define min(a,b)   (((a)<(b))?(a):(b))

/* exit states */
enum exit_e {
    E_LocalTerminate = 1,
    E_KeepAliveFailed,
    E_ConnectFailed,
    E_CapiError,
    E_PollError,
    E_Disconnected
};
typedef enum exit_e exit_t;

/* call states */
enum state_e {
    Disconnected,
    D_ConnectPending,
    D_Connected,
    B_ConnectPending,
    Connected,
    B_DisconnectPending,
    D_DisconnectPending,
};

/* store services here */
typedef struct service_s service_t ;
struct service_s {
    const char *name;
    unsigned char cip, rate, hw;
    int speed;
    int layer1_mask;
};

/**********************************
 *  call specific data
 **********************************/
typedef struct call_s call_t;
struct call_s {
    call_t		*next;

    /*
     *  name and handle for file to transmit
     */
    CONST char		*pFilename;
    FILE		*txfp;

    /* for Xmodem sending */
    xmodem_t		*xmodem;
    char		error_msg[80];	/* xmodem error message buffer */

    /*
     * static configuration data
     */
    struct userdata 	CallingPartyNumber;
    struct userdata 	CalledPartyNumber;
    struct userdata	CallingPartySubaddress;
    struct userdata 	CalledPartySubaddress;
    struct userdata	Bprotocol;

    /* service specfic configuration */
    service_t 		*service;
    userdata_t		llc;
    userdata_t		AdditionalInfo;

    /*
     * dynamic link data
     */
    int			capi_fd;
    enum state_e	state;
    unsigned short	messid;		/*  message number		*/
    unsigned long 	ident;		/*  contrl, plci, ncci 		*/

    unsigned		active:1;	/*  active, passive side	*/
    unsigned		doSend:1;
    unsigned		wasConnected:1;
    unsigned		NotAcknowledged;/* count datab3_req not ack	*/
    
};

typedef struct cfg_s cfg_t;
struct cfg_s {
    char 		rmttelno[40];
    char 		loctelno[40];
    char 		rmtsubaddr[40];
    char 		locsubaddr[40];
    int			verbose;
    int			speed;
#if WITH_RCAPI_LAYER
    rcapi_host_t	capi_host;
    rcapi_auth_t	capi_auth;
#endif
    int 		controller;
    int			b_channel;
    time_t		starttime;
    time_t		endtime;
    unsigned short	usWindowSize;
    int			wait4dataconf;	
    service_t		*service;
    unsigned char	v110_speed;			
    char		*sendfile;
};

typedef struct pollag_s pollag_t;
struct pollag_s {
    int (*func)(int);
};

typedef struct global_s global_t;
struct global_s {
    exit_t 		endloop;
    call_t	 	*calls;
    struct termios	oldtermopts;
    size_t		alive_pending;
    int			tty_state;
    /* pollshiting */
    int 		npollfds;
    struct pollfd 	pollfds[MAX_FD];
    pollag_t 		pollags[MAX_FD];
};

/*******************************************************************
 * other call struct specific includes
 *******************************************************************/
#include "xmodem.h"	/* for xmodem functions */

/*******************************************************************
 * local proto's
 *******************************************************************/
void catch_signal 	(int signo);
int start_external	(void);
int pollset		(int fd, int events, int (*func)(int));
int pollloopt		( long t );
int mypolldel		( int fd );
call_t *alloc_call	(void);
void free_call		( call_t *ptrCall );
/* TODO: replace with userdata_t */
struct userdata *setCalledPartyNumber  ( call_t *ptrCall, char *szCalledPartyNumber);
struct userdata *setCallingPartyNumber ( call_t *ptrCall, char *szCallingPartyNumber, int lPresentation);
struct userdata *setCalledPartySubaddress ( call_t *ptrCall, char *szCalledPartySubaddress);
struct userdata *setCallingPartySubaddress ( call_t *ptrCall, char *szCallingPartySubaddress);
userdata_t *setBprotocol( call_t *ptrCall );
userdata_t *setLLC	( call_t *ptrCall ); 
userdata_t *setAdditionalInfo( call_t *ptrCall );
int setListen( call_t *ptrCall, int mask );
int getCapiInfo		( union CAPI_primitives *capi );
void doDisconnect	( call_t *ptrCall );
void doConnect		( call_t *ptrCall );
void SendData		( call_t *ptrCall );
void handleConnectB3ActiveInd	( call_t *ptrCall );
int handleDataB3Ind	( call_t *ptrCall, unsigned short messid, unsigned char *dataptr, int datalen, unsigned short handle, unsigned short flags );
void handleDataB3Conf	( call_t *ptrCall, unsigned short messid, unsigned short handle);
void handleInfoInd	( call_t *ptrCall, unsigned short infonumber, struct userdata *data);
void handleDisconnectB3Ind	( call_t *ptrCall, unsigned short nReason, struct userdata *ncpi);
void handleDisconnectInd	( call_t *ptrCall, unsigned short nReason);
int capi_event		( int fd );
int usage		( void );
int init_capi		( void);
int init_program        (int argc, char **argv);
void mymakeraw		(struct termios *t);
void reset_terminal	(void);
void set_raw_terminal	(void);
int get_capi_info	(void);
int tty_event 		(int fd);
void prompt		(char *promptmsg, char *inputbuf, size_t buf_len);
void check_capi		(void);
int my_kill		(void);
void get_service	(char *serv);
void set_speeed		(void);
int exit_handler 	(exit_t type);
int get_options 	(int argc, char **argv);
int main		( int argc, char **argv );

#if WITH_UNFINISHED_XMODEM
/* XModem stuff */
extern xmodem_t *xmodem_open(call_t *ptrCall, int baud, int one_kb, char *emsg, char *filename);
extern int xmodem_getpkt(xmodem_t *x, char *rbuf, int *lenp, char *emsg);
extern int xmodem_putpkt(xmodem_t *x, char *sbuf, int len, char *emsg);
extern void xmodem_cancel(xmodem_t *x);
extern void xmodem_close(xmodem_t *x);
extern int xmodem_onekb(xmodem_t *x);
#endif

#ifdef __STDC__
void  my_exit	  ( void );
#else
void my_exit	  ( int *func, caddr_t arg);
#endif

