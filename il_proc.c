#include "isdnlogin.h"

extern global_t global;
extern cfg_t cfg;
extern char *name;
extern char *prog_logo;

/*******************************************************************
 *
 *******************************************************************/
int my_kill(void)
{
    call_t	*ptrCall = NULL;

    /* restore terminal settings */
    reset_terminal();

    mypolldel( STDIN );

    ptrCall = global.calls;
    if ( ptrCall ) {
	free_call( ptrCall );
	mypolldel( ptrCall->capi_fd );
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
void catch_signal( int signo )
{
    switch (signo) {
	case SIGHUP:
	    check_capi();
	    break;
	default:
	    if (signo) fprintf( stderr, "\ngot signal (%d)\n", signo);
	    exit ( 1 );
    }
}
/*******************************************************************
 *
 *******************************************************************/

/*******************************************************************
 *
 *******************************************************************/
int init_program( int argc, char **argv )
{
    name = argv[0];
    printf("%s\n", prog_logo);

    global.endloop = 0;
    global.calls   = NULL;

    /* store the actual terminal settings */
    tcgetattr(0, &(global.oldtermopts));

    /* output completely  unbuffered */
    setbuf(stdout, NULL);	
    setbuf(stderr, NULL);
    setbuf(stdin, NULL);

#ifdef __STDC__
    atexit( my_exit );
#else
    on_exit( my_exit, 0);
#endif
    signal(SIGHUP,	catch_signal);
    signal(SIGINT,	catch_signal);
    signal(SIGQUIT,	catch_signal);
    signal(SIGKILL,	catch_signal);
    signal(SIGTERM,	catch_signal);
    signal(SIGUSR1,	catch_signal);
    signal(SIGUSR2,	catch_signal);

    return 0;
}

