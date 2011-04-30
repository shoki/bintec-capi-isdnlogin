#include "isdnlogin.h"

extern cfg_t cfg;
extern global_t global;

int get_capi_info (void)
{
    int i;
    int		max_controller;
    unsigned long controller_mask = 0;
    capi_getprofile_t profile;

    memset( &profile, 0, sizeof(profile));
#if WITH_RCAPI_LAYER
    rcapi_get_profile( 0, &profile, &(cfg.capi_host));
#else
    capi2_get_profile( 0, &profile );
#endif

    max_controller = profile.ncontrl;

    printf("Available CAPI controllers: %d\n", max_controller);

    if (max_controller) {
	for( i=1; i<33; i++) {
	    memset( &profile, 0, sizeof(profile));
#if WITH_RCAPI_LAYER
	    if (!rcapi_get_profile( i, &profile, &(cfg.capi_host))) {
#else
	    if (!capi2_get_profile( i, &profile)) {
#endif
		controller_mask |= (1 << (i-1));
		if (!--max_controller) break;
	    }
	}
    }

    // XXX: doesnt seem to work!
    if (!(((1 << cfg.controller)-1) & controller_mask)) {
	fprintf( stderr, "No such Controller %d!\n", cfg.controller);
	return 1;
    }
    memset( &profile, 0, sizeof(profile));
#if WITH_RACPI_LAYER
    capi2_get_profile( cfg.controller, &profile, &(cfg.capi_host) );
#else
    capi2_get_profile( cfg.controller, &profile );
#endif

#if DEBUG	/* debug printf */
    fprintf( stderr, "ncontrl    = 0x%x\n", profile.ncontrl); 
    fprintf( stderr, "nchannel   = 0x%x\n", profile.nchannel); 
    fprintf( stderr, "options    = 0x%x\n", profile.options); 
    fprintf( stderr, "b1protocol = 0x%x\n", profile.b1protocol);
    fprintf( stderr, "b2protocol = 0x%x\n", profile.b2protocol);
    fprintf( stderr, "b3protocol = 0x%x\n", profile.b3protocol);
    fprintf( stderr, "\n");
#endif

    if ( !(profile.b1protocol & (SI_PHONE)) ||
	 !(profile.b2protocol & (SI_PHONE)) ||
	 !(profile.b3protocol & (SI_PHONE))) {
	fprintf( stderr, "Controller %d: does not support telephony service\n",
						cfg.controller);
	return 1;
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int init_capi()
{
    int		fd;
    
    for(;;) { 
	if(cfg.verbose > 2){
	    putchar('R');
	}
#if WITH_RCAPI_LAYER
	fd = capi2_register( REG_CAPI_MSG_SIZE,
		REG_CAPI_LEVEL3CNT,
		cfg.usWindowSize,
		REG_CAPI_DATA_BLK_SIZE,
		NULL,
		&(cfg.capi_host),
		&(cfg.capi_auth));
#else
	fd = capi2_register( REG_CAPI_MSG_SIZE,
		REG_CAPI_LEVEL3CNT,
		cfg.usWindowSize,
		REG_CAPI_DATA_BLK_SIZE,
		NULL);
#endif

	if(fd >0 ) break;
	fprintf( stderr, "capi2_register failed! retry in %d seconds\n",
		REGISTER_FAIL_TIMER     );
	sleep(REGISTER_FAIL_TIMER);
    }
    return fd;
}   

int setListen( call_t *ptrCall, int mask )
{
    int ret;
    ret = capi2_listen_req( ptrCall->capi_fd,
		      ptrCall->ident, 	/* Controller */
		      0x000001ff,	/* Info Mask */
		      mask,	 	/* CIP Mask */
		      0,		/* CIP Mask2 */
		      NULL, 		/* Calling party number */
		      NULL);		/* Calling party subaddress */

    return ret;
}

void check_capi (void) 
{
    if (global.alive_pending > MAX_KEEPALIVE_PENDING) {
	global.endloop = E_KeepAliveFailed;	/* indicate keepalive overflow */
	return;
    }

    capi_control_req( global.calls->capi_fd, global.calls->ident, 
	    CTRL_CAPISTATE , NULL);

    global.alive_pending++;
}

/*******************************************************************
 *
 *******************************************************************/
int getCapiInfo( union CAPI_primitives *capi )
{
    int info;

    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_LISTEN_CONF:
	    info = GET_WORD(capi->c2listen_conf.info);
	    break;
	case CAPI2_ALERT_CONF:
	    info = GET_WORD(capi->c2alert_conf.info);
	    break;
	case CAPI2_CONNECT_CONF:		
	    info = GET_WORD(capi->c2connect_conf.info);
	    break;
	case CAPI2_INFO_CONF:
	    info = GET_WORD(capi->c2info_conf.info);
	    break;
	case CAPI2_CONNECTB3_CONF:
	    info = GET_WORD(capi->c2connectb3_conf.info);
	    break;
	case CAPI2_DATAB3_CONF:
	    info = GET_WORD(capi->c2datab3_conf.info);
	    break;
	case CAPI2_RESETB3_CONF:
	    info = GET_WORD(capi->c2resetb3_conf.info);
	    break;
	case CAPI2_DISCONNECTB3_CONF:
	    info = GET_WORD(capi->c2disconnectb3_conf.info);
	    break;
	case CAPI2_DISCONNECT_CONF:
	    info = GET_WORD(capi->c2disconnect_conf.info);
	    break;
	case CAPI2_FACILITY_CONF:
	    info = GET_WORD(capi->c2facility_conf.info);
	    break;
	case CAPI2_SELECTB_CONF:
	    info = GET_WORD(capi->c2selectb_conf.info);
	    break;
	case CAPI2_DISCONNECT_IND:
	    info = GET_WORD(capi->c2disconnect_ind.reason);
	    break;
	case CAPI2_DISCONNECTB3_IND:
	    info = GET_WORD(capi->c2disconnectb3_ind.reason_b3);
	    break;
	default:
	    info = 0;
	    break;
    }
    return (info);
}

void doConnect( call_t *ptrCall )
{
    ptrCall->messid = capi2_connect_req( ptrCall->capi_fd,
		                         ptrCall->ident & 0x7f,
		                         ptrCall->service->cip,
		                         &ptrCall->CalledPartyNumber,
		                         &ptrCall->CallingPartyNumber,
		                         &ptrCall->CalledPartySubaddress,
		                         &ptrCall->CallingPartySubaddress,
		                         &ptrCall->Bprotocol,
		                         NULL, &ptrCall->llc, NULL,
		                         &ptrCall->AdditionalInfo);
    if(cfg.verbose>8) {
	printf("messid from capi2_connect_req() == 0x%x\n", ptrCall->messid);
    }
    NEWSTATE( ptrCall, D_ConnectPending);
}

/*******************************************************************
 *
 *******************************************************************/
void doDisconnect( call_t *ptrCall )
{
    unsigned long plci, ncci;

    ncci = ptrCall->ident;
    plci = ptrCall->ident & 0xffff;

    switch (ptrCall->state) {
	case B_ConnectPending:
	case Connected:
	    if(cfg.wait4dataconf && ptrCall->NotAcknowledged) {
		if(cfg.verbose>1) {
		    printf("doDisconnect() wait for %d CAPI2_DATAB3_CONF\n", 
			    ptrCall->NotAcknowledged);
		}
		/* don't send a capi2_disconnectb3_req() we have to wait */
		break;
	    }
	    if(ptrCall->NotAcknowledged && cfg.verbose>0) {
		printf("doDisconnect() missing %d CAPI2_DATAB3_CONF\n", 
			ptrCall->NotAcknowledged);
	    }
	    capi2_disconnectb3_req( ptrCall->capi_fd,
				    ncci,
				    NULL);
	    break;

	case D_ConnectPending:
	case B_DisconnectPending:
	    NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( ptrCall->capi_fd,
				  plci,
				  NULL);

	    break;

	case D_DisconnectPending:
	case Disconnected:
	default:
	    break;
    }
}

#if WITH_UNFINISHED_XMODEM
int SendXmodemData( call_t *ptrCall )
{
    char	buffer[1024];
    size_t	len;
    xmodem_t	*xm = ptrCall->xmodem;
    
    len = fread( buffer, 1, sizeof(buffer), xm->sendfp);
    if (len <= 0) {
	/* XXX: check for errors */
	fclose(xm->sendfp);
    }

    printf("\nSend XMODEM sector: %d errors: %d ", xm->sentsectcnt, xm->errors);  

    if (!xmodem_putpkt(xm, buffer, len, ptrCall->error_msg)) {
	xmodem_close(xm);
	return 0;
    }
    return 1;
} 
#endif

/*******************************************************************
 * 
 *******************************************************************/
void SendData( call_t *ptrCall )
{
    static unsigned short usDataHandle;
    char		buffer[REG_CAPI_DATA_BLK_SIZE];
    int			len;

    if (0== ptrCall->doSend) { return; }

    /*
     * Read next blk from file into buffer
     */
#if LOGIN_SCRIPT_LINE_BY_LINE
    {
	char		*dataptr;
	dataptr = fgets( buffer, sizeof(buffer), ptrCall->txfp);
	if (dataptr == NULL) {
	    /* fgets returned noting, we are out */
	    ptrCall->doSend = 0;
	    return;
	}
	len = strlen(dataptr);	/* get line length */
    }

#else
    /*
     * NOTE: Don't know why but if we send data with a length
     *       equal to REG_CAPI_DATA_BLK_SIZE, the brick gona
     *       complain about to long packet, so we just send
     *       half of the max buffer size.
     */

    len = fread( buffer, 1, (sizeof(buffer) / 2), ptrCall->txfp);
    if (len <= 0) {
	if (len < 0) {
	    perror("\nERR:SendData\n");
	}
	ptrCall->doSend = 0;
	return;
    }
#endif

    ptrCall->NotAcknowledged++;		/* count pending confirms */
    usDataHandle++;

    capi2_datab3_req( ptrCall->capi_fd,
	    ptrCall->ident,
	    buffer,
	    len,
	    0,
	    usDataHandle);
}

/*******************************************************************
 *
 *******************************************************************/
void handleConnectB3ActiveInd( call_t *ptrCall )
{
    if(cfg.verbose > 0) {
	printf("Terminate sequence is '%c%c'\n", ESCAPE_CHAR, TERMINATE_CHAR);
    }

    /* put terminal into raw mode after connect was okay */
    set_raw_terminal();

    cfg.starttime		= time(0);
    ptrCall->wasConnected	= 1;
    ptrCall->doSend		= 1;

    if (ptrCall->txfp) {
	SendData( ptrCall );
    }
}


/*******************************************************************
 *
 *******************************************************************/
int handleDataB3Ind( call_t		*ptrCall,
	unsigned short	messid,
	unsigned char	*dataptr,
	int		datalen,
	unsigned short	handle,
	unsigned short	flags )
{

#if 0
    printf("komisch hab ne data indicationb\n");
    capi_hexdump( dataptr, datalen, 16, 2);
#endif

    printf("%*.*s", datalen, datalen, dataptr);

    return 1;	/*  response */
}

/*******************************************************************
 *
 *******************************************************************/
void handleDataB3Conf( call_t		*ptrCall,
	unsigned short	messid,
	unsigned short	handle )
{
    ptrCall->NotAcknowledged--;		/* We got a acknowledge */
#ifdef FAX
    if(cfg.verbose > 2) {
	putchar('c');
    }
#endif
    if (ptrCall->txfp) {
	SendData( ptrCall );		/* Send the next one	*/
    } 
}

/*******************************************************************
 *
 *******************************************************************/
void handleInfoInd( call_t	*ptrCall,
	unsigned short	infonumber,
	struct userdata	*data )
{

    unsigned long	mask;
    unsigned char      *datap = data->data;

    switch(infonumber){
	default:
	    if(cfg.verbose>5){
	        printf("\tINFOIND: 0x%04x\n", infonumber);
		capi_hexdump( data->data, data->length, 16, 2);
	    }
	    break;
	case 0x18:	/* IBCHI	*/
	    if (*datap & (1<<5)) {
		/* primary rate interface */
		if (*datap++ & (1<<6)) {
		    /* interface identifier present */
		    while((*datap++ & 0x80) == 0);
		}
		if (*datap++ & (1<<4)) {
		    cfg.b_channel = 1;
		    mask = 1;
		    while(!(mask & *datap)){
			mask<<=1;
			cfg.b_channel++;
		    }
		}
		else {
		    cfg.b_channel = *datap & 0x7f;
		}
	    }
	    else {
		/* basic rate interface */
		cfg.b_channel = *datap & 0x03;
	    }
	    if(cfg.verbose>2){
		printf("\tINFOIND: ISDN-BCHANNEL %d\n", cfg.b_channel);
	    }
	    break;
    }

}

/*******************************************************************
 *
 *******************************************************************/
void handleDisconnectB3Ind( call_t		*ptrCall,
	unsigned short	nReason,
	struct userdata	*ncpi )
{
    int			delta_time;

    cfg.endtime = time(0);
    delta_time  = cfg.endtime - cfg.starttime;

    reset_terminal();

    /* No need for this here 
     * if (nReason) printf("\tDISCONNECT B3_Reason (0x%04x)\n", nReason);
     */

    if(ptrCall->NotAcknowledged && cfg.verbose>0) {
	printf("handleDisconnectB3Ind() missing %d CAPI2_DATAB3_CONF\n", 
		ptrCall->NotAcknowledged);
    }
}

/*******************************************************************
 *
 *******************************************************************/
void handleDisconnectInd( call_t *ptrCall, 
	unsigned short nReason)
{
    printf("ISDN D-channel disconnect, Reason (%04x)\n", nReason);
}
