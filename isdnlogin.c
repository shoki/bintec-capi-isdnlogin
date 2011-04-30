/************************************************************************
 *  (C)opyright 1991-1999 Andre Ilie, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: isdnlogin.c,v $
 *   $Revision: 74 $
 *       $Date: 2005-12-22 01:06:50 +0100 (Thu, 22 Dec 2005) $
 *      $State: Exp $
 *     
 *        Type: streams driver/module | application | library
 *    Products: ALL | XS,XM,XL,XP,BGO,BGP,XCM
 *  Interfaces: IPI, DLPI
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/

#include "isdnlogin.h"

/**********************************
 * global variables
 **********************************/
const char * name		= NULL;
const char * prog_logo		= "BIANCA/CAPI ISDN Login Client 1.51 by Andrei Ilie <bender@duese.org>";
const char * rcs_id_isdnlogin_c	= "$Id: isdnlogin.c,v 1.52 2003/07/30 12:14:36 shoki Exp $";

global_t global;				/* global program data */

cfg_t cfg = {
	"",					/*  remote telno	*/
	"+49 911 12345678",			/*  local telno		*/
	"",					/*  remote subaddr	*/
	"",					/*  local subaddr	*/
	DEFAULT_VERBOSE_LEVEL,			/*  verbose mode 	*/
	38400,					/*  maximum speed 	*/
#if WITH_RCAPI_LAYER
	{ CAPI_HOST_DEFAULT, NULL, 0 },
						/*  capi host info	*/
	{ CAPI_USER_DEFAULT, NULL },
						/* capi auth info 	*/
#endif
	1,					/*  controller		*/
	-1,					/*  b-channel number    */
	0,					/*  store start time    */
	0,					/*  store end time    	*/
	REG_WINDOW_SIZE,			/*  Window Size		*/
	DEFAULT_WAIT4DATACONF,			/*  flag 		*/
	NULL,					/* service */
	0,					/* v110_rate */
	NULL					/* file to send */
};


/* for service specific parameters */
static service_t service[] = {
    /* name         cip  	rate 	hw 		speed   layer1_mask */
    { "data",       CIP_DATA,  	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    { "telephony",  CIP_PHONE, 	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    /* TODO: check this out... don't know if it works 
    { "faxg3",      CIP_FAXG3, 	0, 	FAX_G3,     	14400, 	B1FAXG3 },	
    { "faxg4",      CIP_FAXG4, 	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    { "t_online",   CIP_BTX,   	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    { "datex_j",    CIP_BTX,   	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    { "btx",        CIP_BTX,   	0, 	ISDN_HDLC,     	0, 	B1HDLC },
    { "v120",       CIP_DATA, 	0xb9,	ISDN_HDLC, 	56000, 	B1HDLC56 }, 
    { "56k",        CIP_DATA, 	0, 	ISDN_HDLC,	56000, 	B1HDLC56 },
    */
    { "modem",      CIP_3KAUDIO,0, 	MODEM,     	0, 	B1MODEMASYNC },
    { "trans",      CIP_DATA,  	0, 	TRANS,     	0, 	B1TRANS },
    { "compr",      CIP_DATA,  	0, 	ISDN_V42,     	0, 	B1HDLC },
    { "v110",  	    CIP_DATA, 	0x4d,	V110_ASYNC,  	38400, 	B1V110TRANS }, 
    { "",           0,   	0, 	0,     		0, 	0 },
};

#ifdef __STDC__
void  my_exit( void  )
#else
void my_exit( func, arg)
    int	       *func;
    caddr_t	arg;
#endif
{
    my_kill();
}


/*******************************************************************
 *
 *******************************************************************/
int usage( void )
{
    service_t   *sv;
    int		i = 1;

    fprintf( stderr, "\nUsage: %s [-%s] <#phone> [service]\n", name, 
	CMD_LINE_ARGS);
    fprintf( stderr, "\tOptions:\n");
#if WITH_RCAPI_LAYER
    fprintf( stderr, "\t-b: specify remote CAPI host\n");
    fprintf( stderr, "\t-P: specify remote CAPI port\n");
    fprintf( stderr, "\t-u: specify remote CAPI user\n");
    fprintf( stderr, "\t-p: specify remote CAPI password\n");
#endif
    fprintf( stderr, "\t-l: specify local phone number\n");
    fprintf( stderr, "\t-s: specify maximum transfer speed in bps\n");
    fprintf( stderr, "\t-c: specify ISDN controller to use [1..n]\n");
    fprintf( stderr, "\t-q: use quiet mode\n");
    fprintf( stderr, "\t-C: specify file to send after dialin (autologin)\n");
    fprintf( stderr, "\t-v: specify verbose level (default=%d)\n",
	    DEFAULT_VERBOSE_LEVEL);
    fprintf( stderr, "\t-w: specify data window size (default=%d)\n",
	    REG_WINDOW_SIZE);
    fprintf( stderr, "\t-W: wait for DATAB3_CONF before DISCONNECTB3_REQ (default=%d)\n",
	    DEFAULT_WAIT4DATACONF);

    fprintf( stderr, "\nServices:\n\t" );
    for (sv = service; *sv->name; sv++) {
	fprintf(stderr, "%s ", sv->name);
	if (!(i++%6)) fprintf(stderr, "\n\t");
    }
    fprintf( stderr, "\n");
    exit(1);
}

void get_service(char *serv) 
{
    service_t   *sv;

    cfg.service = service;	/* default service is the frist one */

    /* get or guess service type */
    if (serv != NULL) {
	for (sv = service; *sv->name; sv++) {
	    if (!strncmp( serv, sv->name, strlen(serv))) {
		/* got service in table, save pointer to it */
		cfg.service = sv;
		break;
	    }
	}
	if (!*sv->name) {
		fprintf( stderr, "Service '%s' not found, using '%s'.\n", 
			serv, cfg.service->name);
	}
    }
}

void set_speed(void)
{
    /* adjust speed setting */
    switch (cfg.speed) {
	/* speed is in range */
	case 1200:	cfg.v110_speed = 0x42;		break;
	case 2400:	cfg.v110_speed = 0x43;		break;
	case 4800:	cfg.v110_speed = 0x45;		break;
	case 9600:	cfg.v110_speed = 0x48;		break;
	case 14400:	cfg.v110_speed = 0x49;		break;	/* XXX: broken */
	case 19200:	cfg.v110_speed = 0x4b;		break;
	case 38400:	cfg.v110_speed = 0x4d;		break;
	default:
	    /* if service specific max speed given use this */
	    printf("Invalid speed setting %d bps! ", cfg.speed);
	    if (cfg.service->speed) {
		cfg.speed = cfg.service->speed;
		cfg.v110_speed = cfg.service->rate;
	    } else {
		cfg.speed = 1200;	/* min rate */
		cfg.v110_speed = 0x42;
	    }
	    printf("Speed set to %d bps\n", cfg.speed);
	    break;
    }
}


int exit_handler (exit_t type)
{
    int exitval;

    switch (type) {
	case E_LocalTerminate:
	    printf("Local terminate requested.\n");
	    exitval = 0;
	    break;
	case E_KeepAliveFailed:
	    printf( "CAPI keepalive failed (%d pending)!\n" ,
		global.alive_pending);
	    exitval = 1;
	    break;
	case E_ConnectFailed:
	    printf("Could not connect to remote system!\n");
	    exitval = 1;
	    break;
	case E_CapiError:
	    printf("Got fatal CAPI error!\n");
	    exitval = 1;
	    break;
	case E_PollError:
	    printf("Got fatal poll error!\n");
	    exitval = 1;
	    break;
	case E_Disconnected:
	    exitval = 0;
	    break;
	default:
	    exitval = 1;
	    break;
    }
    return (exitval);
}

int get_options (int argc, char **argv)
{
    int 		i;
    extern int		optind;
    extern char 	*optarg;

    /*
     * get options
     */
    while ((i = getopt( argc, argv, CMD_LINE_ARGS )) != EOF) switch (i) {
	case 'l': strncpy( cfg.loctelno,  optarg, sizeof(cfg.loctelno));  break;
#if WITH_RCAPI_LAYER
	case 'b': cfg.capi_host.hostname = strdup(optarg); 		  break;
	case 'P': cfg.capi_host.port     = atoi(optarg);		  break;
	case 'u': cfg.capi_auth.user	 = strdup(optarg);		  break;
	case 'p': cfg.capi_auth.passwd	 = strdup(optarg);		  break;
#endif
	case 'c': cfg.controller    = atoi(optarg);			  break;
	case 's': cfg.speed         = atoi(optarg);		    	  break;
	case 'q': cfg.verbose       = 0;			  	  break;
	case 'v': cfg.verbose       = atoi(optarg);		    	  break;
	case 'C': cfg.sendfile      = strdup(optarg);			  break;
	case 'w': cfg.usWindowSize  = atoi(optarg);			  break;
	case 'W': cfg.wait4dataconf = atoi(optarg);			  break;
	case '?':
	case 'h':
	default:  usage();
    }

    if (optind < argc) {
	strncpy( cfg.rmttelno, argv[optind++], sizeof(cfg.rmttelno));
    } else {
	usage();
	return 1;
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int main( int argc, char **argv )
{
    call_t	*ptrCall;
    int 	ret;
#if CHECK_CAPI
    int		timeout = KEEP_ALIVE_TIMER;
#else
    int		timeout = -1; 	/* inifinite poll */
#endif

    init_program(argc, argv);
    if (get_options(argc, argv)) exit(1);

    /* service specific settings */
    get_service(argv[optind]);
    set_speed();

    /* CAPI stuff */
    if (get_capi_info()) exit( 1 );

    /* allocate first call instance */
    ptrCall = alloc_call();
    global.calls = ptrCall;
    if (!ptrCall) {
	fprintf( stderr, "failed to allocate call!\n");
	exit ( 1 );
    }

    /* do register */
    ptrCall->capi_fd = init_capi();
    if (ptrCall->capi_fd <= 0) {
	exit ( 1 );
    }
    
    // For server mode
    //setListen(ptrCall, ptrCall->service->cip);

    printf("Trying...\n");
    doConnect( ptrCall );

    /* poll on STDIN for incoming packets */
    pollset( STDIN, POLLIN, tty_event);

    /* poll on capi for incoming packets */
    pollset( ptrCall->capi_fd, POLLIN, capi_event);

    /* poll fds for incoming packets and handel them */
    while (!global.endloop) {
	ret = pollloopt(timeout);
	switch(ret) {
	    case -1:
		/* poll error */
		global.endloop = E_PollError;
		break;
	    case 0:
		/* got timeout */
#if CHECK_CAPI
		/* check capi when we have time for it */
		check_capi();
#endif
		break;
	    default:
		/* did some work on fds */
		break;
	}
    }

    /* reset terminal to unraw mode and output failure messages */
    reset_terminal();

    exit(exit_handler(global.endloop));
}

