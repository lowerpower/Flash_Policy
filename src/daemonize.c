/*!																www.mycal.net			
 *---------------------------------------------------------------------------
 *! \file daemonize.c
 *  \brief Function to daemonize a process
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version June 3, 2006									-        
 *
 *---------------------------------------------------------------------------    
 * Version                                                                  -
 * 0.1 Original Version August 31, 2006     							    -
 *																			-
 * (c)2006 mycal.net								-
 *---------------------------------------------------------------------------
 *
 */
#if !defined(WIN32)             // For unix only

#include <stdio.h>    //printf(3)
#include <stdlib.h>   //exit(3)
#include <unistd.h>   //fork(3), chdir(3), sysconf(3)
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <sys/types.h>
#include <pwd.h>        // getepwname(3)
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)
#include <string.h>
#include <errno.h>
#include "daemonize.h"



/*! \fn int daemonize(user,path,outfile,errorfile,infile)
    \brief Daemonize a process

    \param 

	\return 
*/

int
daemonize(char* user, char *dir, char* path, char* outfile, char* errfile, char* infile )
{
    int ret;
    pid_t child;
    int fd;
    struct passwd *pw;


    if(user)
    {
        if ((pw = getpwnam(user)) == NULL) {
                fprintf(stderr, "getpwnam(%s) failed: %s\n",
                                user, strerror(errno));
                exit(EXIT_FAILURE);
        }
    }

    // Fill in defaults if not specified
    if(!path) { path="/"; }
    if(!infile) { infile="/dev/null"; }
    if(!outfile) { outfile="/dev/null"; }
    if(!errfile) { errfile="/dev/null"; }

    //fork, detach from process group leader
    if( (child=fork())<0 ) { //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child>0) { //parent
        exit(EXIT_SUCCESS);
    }
    if( setsid()<0 ) { //failed to become session leader
        fprintf(stderr,"error: failed setsid\n");
        exit(EXIT_FAILURE);
    }

    //catch/ignore signals
    signal(SIGCHLD,SIG_IGN);
    signal(SIGHUP,SIG_IGN);

    //fork second time
    if ( (child=fork())<0) { //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if( child>0 ) { //parent
        exit(EXIT_SUCCESS);
    }

    //new file permissions
    umask(0);
    //change to path directory
    ret=chdir(path);

    //Close all open file descriptors
    for( fd=sysconf(_SC_OPEN_MAX); fd>0; --fd )
    {
        close(fd);
    }

    //reopen stdin, stdout, stderr
    stdin=fopen(infile,"r");   //fd=0
    stdout=fopen(outfile,"w+");  //fd=1
    stderr=fopen(errfile,"w+");  //fd=2

    ret=1;
    return(ret);
}

#endif
