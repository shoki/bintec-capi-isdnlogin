#include "isdnlogin.h"

extern cfg_t cfg;
extern char * prog_logo;

/*******************************************************************
 *
 *******************************************************************/
call_t *alloc_call(void)
{
    call_t *ptrCall;

    ptrCall = (call_t *)malloc( sizeof(*ptrCall));

    if (!ptrCall) return NULL;
    memset( ptrCall, 0, sizeof(*ptrCall));

    /*
     * open file to transmit
     */
    if (cfg.sendfile) {
	ptrCall->pFilename = cfg.sendfile;
	if (!(ptrCall->txfp=fopen( ptrCall->pFilename, "r"))) {
	    printf ("Could read from file: <%s>\n", ptrCall->pFilename);
	    exit ( 1 );
	}
	if ( cfg.verbose > 2 ) {
	    printf("Reading from file: <%s>\n", ptrCall->pFilename);
	}
    }

    /* default call settings */

    /* add service and controller to call */
    ptrCall->active = 1;
    ptrCall->ident  = cfg.controller;	/*  controller  */
    ptrCall->service= cfg.service;

    /* set numbers */
    setCalledPartyNumber(      ptrCall, cfg.rmttelno);
    setCallingPartyNumber(     ptrCall, cfg.loctelno, 1);
    setCalledPartySubaddress(  ptrCall, cfg.rmtsubaddr);
    setCallingPartySubaddress( ptrCall, cfg.locsubaddr);

    /* adjust protocols */
    setBprotocol( ptrCall); 
    setLLC( ptrCall );
    setAdditionalInfo( ptrCall );

    return ptrCall;
}

/*******************************************************************
 *
 *******************************************************************/
void free_call( call_t *ptrCall )
{
    if (ptrCall) {
	if (ptrCall->txfp) {
	    fclose( ptrCall->txfp);
	}
	free(ptrCall);
    }
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCalledPartyNumber( call_t *ptrCall, 
	char *szCalledPartyNumber)
{
    size_t len = 0;

    ptrCall->CalledPartyNumber.length = 0;
    if (szCalledPartyNumber) {
	len = strlen( szCalledPartyNumber);
	len = min(len, CAPI1_MAXMSGLEN-1);
	if (len) {
	    ptrCall->CalledPartyNumber.data[0] = 0x81;

	    memcpy( &ptrCall->CalledPartyNumber.data[1],
		    szCalledPartyNumber, len);
	    ptrCall->CalledPartyNumber.length = len+1;
	}
    }

    return &ptrCall->CalledPartyNumber;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCallingPartyNumber( call_t *ptrCall,
	char *szCallingPartyNumber,
	int lPresentation)
{
    size_t len = 0;

    ptrCall->CallingPartyNumber.length = 0;
    if (szCallingPartyNumber) {
	len = strlen( szCallingPartyNumber);
	len = min(len, CAPI1_MAXMSGLEN-1);
	if (len) {
	    ptrCall->CallingPartyNumber.data[0] = 0x01;
	    ptrCall->CallingPartyNumber.data[1] = lPresentation ? 0x80 : 0xa0;

	    memcpy( &ptrCall->CallingPartyNumber.data[2], 
		    szCallingPartyNumber, len);
	    ptrCall->CallingPartyNumber.length = len+2;
	}
    } 

    return &ptrCall->CallingPartyNumber;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCalledPartySubaddress( ptrCall, szCalledPartySubaddress)
call_t *ptrCall;
char *szCalledPartySubaddress;
{
    size_t len;

    ptrCall->CalledPartySubaddress.data[0] = 0x80;
    if ((len = strlen(szCalledPartySubaddress))) {
	memcpy( &ptrCall->CalledPartySubaddress.data[1],
		szCalledPartySubaddress, len);
    } else {
	ptrCall->CalledPartySubaddress.data[1] = 0;
	len = 1;
    }

    ptrCall->CalledPartySubaddress.length  = len + 1;
    return &ptrCall->CalledPartySubaddress;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCallingPartySubaddress( call_t *ptrCall,
	char *szCallingPartySubaddress)
{
    size_t len;

    ptrCall->CallingPartySubaddress.data[0] = 0x80;	/* always there */

    if ((len = strlen(szCallingPartySubaddress))) {
	/* if subaddress suplied use it */
	memcpy( &ptrCall->CallingPartySubaddress.data[1],
		szCallingPartySubaddress, len);
    } else {
	ptrCall->CallingPartySubaddress.data[1] = 0;
	len = 1;
    }

    ptrCall->CallingPartySubaddress.length  = len + 1; /* one for the type */
    return &ptrCall->CallingPartySubaddress;
}

userdata_t	 *setBprotocol( call_t *ptrCall) 
{
    userdata_t	 	*data;
    struct bprotocol 	*bprot;
    struct b1config 	*b1cfg;
    struct b1config_modem *b1cfg_mdm;
    struct b2config 	*b2cfg;
    struct b3config	*b3cfg;

    bprot = (struct bprotocol *)&ptrCall->Bprotocol;

    /* set layer 1 protocol */
    PUT_WORD( bprot->b1proto, ptrCall->service->layer1_mask);
     
    /* depending on service set layer 2 protocol */
    switch(ptrCall->service->hw) {
	case ISDN_HDLC:
	    PUT_WORD( bprot->b2proto, B2X75);
	    break;
	case FAX_G3:
	    PUT_WORD( bprot->b2proto, B2T30);
	    break;
	case ISDN_V42:
	    PUT_WORD( bprot->b2proto, B2X75V42BIS);
	    break;
	case V110_ASYNC:
	case TRANS:
	case MODEM:
	default:
	    PUT_WORD( bprot->b2proto, B2TRANS);
 	    break;
    }

    /* check for layer 2 specific settings */
    switch (ptrCall->service->hw) {
	case FAX_G3:
	    PUT_WORD( bprot->b3proto, B3T30);
	    break;
	default:
	    /* layer 3 is always trans for now */
	    PUT_WORD( bprot->b3proto, B3TRANS);
	    break;
    }
    data = (userdata_t	 *)&bprot->structlen;

    /* check if the service has special layer 2 config */
    switch(ptrCall->service->hw) {
	case V110_ASYNC:
	    b1cfg = (struct b1config *) data;
	    /* v110 specific layer 2 params */
	    PUT_WORD( b1cfg->rate, cfg.speed);
	    PUT_WORD( b1cfg->bpc, 8);
	    PUT_WORD( b1cfg->parity, 0);
	    PUT_WORD( b1cfg->stopbits, 0);
	    b1cfg->length = sizeof( struct b1config) - 1;
	    break;
        case MODEM:
	    /* modem specific params */
	    b1cfg_mdm = (b1config_modem_t *) data;
	    PUT_WORD( b1cfg_mdm->rate, cfg.speed);	/* 0 = max speed */
	    PUT_WORD( b1cfg_mdm->bpc, 8);
	    PUT_WORD( b1cfg_mdm->parity, 0);
	    PUT_WORD( b1cfg_mdm->stopbits, 0);
	    PUT_WORD( b1cfg_mdm->options, 0);
	    PUT_WORD( b1cfg_mdm->negotiation, 1);	/* allow autoneg */
	    b1cfg_mdm->length = sizeof( struct b1config_modem) -1;
	    break;
	case FAX_G3:
	    /* FAX group 3 params */
	    b1cfg = (struct b1config *) data;
	    PUT_WORD( b1cfg->rate, cfg.speed);
	    PUT_WORD( b1cfg->bpc, 0);
	    PUT_WORD( b1cfg->parity, 0);
	    PUT_WORD( b1cfg->stopbits, 0);
	    b1cfg->length = sizeof( struct b1config) - 1;
	    break;
	default:
	    data->length = 0;
	    break;
    }
    data = (userdata_t	 *)&data->data[data->length];

    b2cfg = (struct b2config *) data;
    switch(ptrCall->service->hw) {
	case ISDN_HDLC:
	    b2cfg->addressA = 3; 
	    b2cfg->addressB = 1;
	    b2cfg->moduloMode = 8;
	    b2cfg->windowSize = 7;
	    b2cfg->xidlen = 0;
	    b2cfg->length = sizeof( struct b2config) - 1;	
	    break;
	default:
	    b2cfg->length = 0;
	    break;
    }
    data = (userdata_t	 *)&data->data[data->length];

    /* Layer 3 always transparent */
    b3cfg = (struct b3config *) data;
    b3cfg->length = 0;

    bprot->length = (char *)data - (char *)&bprot->length -1;
#if 0
    capi_hexdump((char*) bprot, bprot->length, 16, 2);
    printf("b1cfg->length=%d\n", b1cfg->length);
    printf("b1cfg_mdm->length=%d\n", b1cfg_mdm->length);
    printf("b2cfg->length=%d\n", b2cfg->length);
    printf("bprot->length=%d\n", bprot->length);
    printf("data->length=%d\n", data->length);
    //printf("data->structlen=%d\n", data->structlen);
#endif

    return (userdata_t	 *)bprot;
}

userdata_t *setLLC( call_t *ptrCall ) {
    userdata_t *llc;
    /* Lower Layer Compatibility */
    static char llcs[] = { 0x90, 0x88, 0x90, 0x21, 0x42, 0x00, 0xbb };

    llc = &ptrCall->llc;

    if (ptrCall->service->rate) {
	llc = (userdata_t *)&ptrCall->llc;

	llcs[4] = cfg.v110_speed;	/* set user rate */
	memcpy(&ptrCall->llc, &llcs, sizeof(llcs));
	llc->length = sizeof(llcs) - 1;
    }
    return llc;
}

userdata_t *setAdditionalInfo( call_t *ptrCall )
{
    struct userdata *add;
    struct userdata *data;

    add = (struct userdata *)&ptrCall->AdditionalInfo;
    data = (struct userdata *)add->data;

    data->length = 0;	/* b-channel */

    data = (struct userdata *)&data->data[data->length];
    data->length = 0;   /*  keypad  */

    data = (struct userdata *)&data->data[data->length];
    memcpy(&data->data[0], prog_logo, strlen(prog_logo));

    data->length = strlen(data->data);   /*  user user data  */

    data = (struct userdata *)&data->data[data->length];
    data->length = 0;   /*  facility  */

    data = (struct userdata *)&data->data[data->length];
    add->length = (char *)data - (char *)&add->length - 1;

    return (struct userdata *)add;
}
