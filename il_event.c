#include "isdnlogin.h"

extern global_t global;
extern cfg_t cfg;

/*******************************************************************
 *
 *******************************************************************/
int capi_event( int fd )
{
    int			info;
    int			lcause;
    unsigned long	ident;
    unsigned char	proto[1024];
    unsigned char	data[4096];
    union CAPI_primitives *capi;
    call_t *ptrCall;

    lcause = capi2_get_message( fd, &capi,
	    (union CAPI_primitives *)proto, data, sizeof(data));

    /* 
     *  check for errors
     */
    switch (lcause) {
	case CAPI2_E_MSG_QUEUE_EMPTY:	 	return  0;		
	case CAPI2_E_INTERNAL_BUSY_CONDITION:	
	case CAPI2_E_ILLEGAL_APPLICATION:
	case CAPI2_E_OS_RESOURCE_ERROR:
	    /* error occured, stop working */
            global.endloop = E_CapiError;
	    return -1;
	default:				break;
    }

    info    = getCapiInfo(capi);
    if (info) {
	/* TODO: some special output function for local output */
	capi2_perror("\nCAPI Info", info);
    }

    /*
     *  get the call structure
     */
    ident   = GET_c2IDENT(capi);
#if 0
    ptrCall = getCallbyIdent( ident);
    printf("CAPI: 0x%08x 0x%08x\n", ident, GET_PRIMTYPE(capi));
#endif
    ptrCall = global.calls;


    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_LISTEN_CONF:
	    break;
	
	case CAPI2_CONNECT_CONF:
#if 0
	    ptrCall = getCallbyMessid(GET_MESSID(capi));
#endif
	    if (ptrCall) {
		ptrCall->ident = ident;
		if (info) {	/* CONNECT_REQ failed -> end program */ 
		    global.endloop = E_ConnectFailed;
		}
	    }	
	    break;
	
	case CAPI2_INFO_IND:
	    capi2_info_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		handleInfoInd( ptrCall,
			GET_WORD(capi->c2info_ind.info),
			(struct userdata *)&capi->c2info_ind.structlen);
	    }
	    break;
#if 0
	case CAPI2_CONNECT_IND:
	    if (ptrCall) {
		capi2_connect_resp( fd, 
			(unsigned short)GET_MESSID(capi),
			ident,        
			0, 			/* accept */
			&ptrCall->Bprotocol,
			NULL,  			/* cad */
			NULL,  			/* csa */
			NULL,  			/* llc */
			NULL); 			/* add */
	    }
	    break;
#endif
	
	case CAPI2_CONNECTACTIVE_IND:
	    if (cfg.verbose>8) {
		printf("CAPI2_CONNECTACTIVE_IND %lx\n", ident);
	    }
	    capi2_connectactive_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		NEWSTATE( ptrCall, D_Connected);
		if (ptrCall->active) {
		    NEWSTATE( ptrCall, B_ConnectPending);
		    capi2_connectb3_req( fd, ident, NULL);
		}
	    }
	    break;
	    
	case CAPI2_CONNECTB3_CONF:
#if 0
	    ptrCall = getCallbyPlci( ident & 0xffff );
#endif
	    if (ptrCall) {
		/*
		 * NCCI allocation -> store NCCI for later use
		 */
		ptrCall->ident = ident;
	    }
	    break;
	    
	case CAPI2_CONNECTB3ACTIVE_IND:
	    capi2_connectb3active_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		NEWSTATE( ptrCall, Connected);
		handleConnectB3ActiveInd( ptrCall );
	    }
	    break;

	case CAPI2_DATAB3_CONF:
	    if (ptrCall) {
		handleDataB3Conf( ptrCall, 
				  GET_MESSID(capi),
				  GET_WORD(capi->c2datab3_conf.handle));
	    }
	    break;

	case CAPI2_DATAB3_IND:
	    if (ptrCall) {
		if (handleDataB3Ind( ptrCall,
				     GET_MESSID(capi),
				 (char *)GET_DWORD(capi->c2datab3_ind.dataptr),
				     GET_WORD( capi->c2datab3_ind.datalen),
				     GET_WORD( capi->c2datab3_ind.handle),
				     GET_WORD( capi->c2datab3_ind.flags))) {
		    capi2_datab3_resp( fd,
				       GET_MESSID(capi),
				       ident,
				       GET_WORD(capi->c2datab3_ind.handle));
		}
	    }
	    break;

	case CAPI2_DISCONNECTB3_IND:
	    if (ptrCall) {
		NEWSTATE( ptrCall, B_DisconnectPending);
		handleDisconnectB3Ind( ptrCall, 
				GET_WORD(capi->c2disconnectb3_ind.reason_b3),
		    (struct userdata *)&capi->c2disconnectb3_ind.structlen);
	    }	
	    capi2_disconnectb3_resp( fd,
				     GET_MESSID(capi),
				     ident);
	    if (ptrCall) NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( fd,
				  ident & 0xffff,
				  NULL);
	    break;
		
	case CAPI2_DISCONNECT_IND:
	    capi2_disconnect_resp( fd,
				   GET_MESSID(capi),
				   ident);

	    if (ptrCall) {
		NEWSTATE( ptrCall, Disconnected);
		handleDisconnectInd( ptrCall, 
				     GET_WORD(capi->c2disconnect_ind.reason));

	    }
	    global.endloop = E_Disconnected;
	    break;
	case CAPI_CONTROL_CONF:
	    /* decrement alive pendings */
	    global.alive_pending--;
	    break;
	case CAPI2_ALERT_CONF:
	case CAPI2_INFO_CONF:
	case CAPI2_DISCONNECTB3_CONF:
	case CAPI2_DISCONNECT_CONF:
	case CAPI2_FACILITY_CONF:
	    break;
    }
    return 0;
}

