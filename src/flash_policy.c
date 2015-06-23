/*!									
 *---------------------------------------------------------------------------
 *! \file Flash_Policy.c
 *  \brief Flash Policy Server
 *
 *  Simple Flash Policy server, simple, reliable and suitable for most 
 *  applications.  Policy is specify in a file, auto reload of Policy file 
 *  while running without reload.
 *
 * The operation is very simple, accepts TCP connections on a socket,
 * default 843, and responds with the policy file and immeadatly closes
 * socket.  No data is read from client.
 *
 * This is a single thread, select based, non blocking server. Should
 * easly handle hunderes of connections per second.
 *
 * Statistics file can be optionally written at an interval.
 *
 * This server can be run as a daemon or an application.
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version Oct 4 2014									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2014, mycal.net							-
 *																			-
 *---------------------------------------------------------------------------
 *
 * Notes:
 * User definable policy should be here or specified on command line:
 *
 * Unix:             /usr/local/etc/flashpolicy.xml
 * Windows           c:/flash/flashpolicy.xml
 *
 *
 *
*/

/*					*/ 
/* include files 	*/
/*					*/
#include "flash_policy.h"
#include "arch.h"
#include "yselect.h"
#include "daemonize.h"
#include "file_config.h"
#include "debug.h"


#if defined(WIN32)
#include "wingetopt.h"
#endif

int global_flag=0;
int go=0;


const char default_policy[] = "<?xml version=\"1.0\"?>\n"
"<!DOCTYPE cross-domain-policy SYSTEM \"/xml/dtds/cross-domain-policy.dtd\">\n"
"<!-- Policy file for xmlsocket://socks.example.com -->\n"
"<cross-domain-policy>\n"
"   <site-control permitted-cross-domain-policies=\"master-only\"/>\n"
"   <allow-access-from domain=\"*.yoics.net\" to-ports=\"30000-40000\" />\n"
"</cross-domain-policy>";



//
// network_init()-
// If anything needs to be initialized before using the network, put it here, mostly this
//	is for windows.
//
int network_init(void)
{
#if defined(WIN32) || defined(WINCE)

	WSADATA w;								/* Used to open Windows connection */
	/* Open windows connection */
	if (WSAStartup(0x0101, &w) != 0)
	{
		fprintf(stderr, "Could not open Windows connection.\n");
	    printf("**** Could not initialize Winsock.\n");
		exit(0);
	}	

#endif
return(0);
}
int
set_sock_nonblock(SOCKET lsock)
{
	int ret;
#if defined(WIN32) || defined(WINCE)
	u_long	flags;

	flags=0x1;
	ret=ioctlsocket ( lsock, FIONBIO, (u_long FAR *) &flags);
#endif

#if defined (__ECOS)
    int tr = 1;
    ret=ioctl(lsock, FIONBIO, &tr); 
#endif

#if defined(LINUX) || defined(MACOSX) || defined(IOS)

	int flags;

	flags = fcntl(lsock, F_GETFL, 0);
	ret=fcntl(lsock, F_SETFL, O_NONBLOCK | flags);

#endif

	return(ret);
}

int
set_sock_block(SOCKET lsock)
{
	int ret;
#if defined(WIN32) || defined(WINCE)
	u_long	flags;

	flags=0x0;
	ret=ioctlsocket ( lsock, FIONBIO, (u_long FAR *) &flags);
#endif

#if defined (__ECOS)
    int tr = 0;
    ret=ioctl(lsock, FIONBIO, &tr); 
#endif

#if defined(LINUX) || defined(MACOSX) || defined(IOS)

	int flags;

	flags = fcntl(lsock, F_GETFL, 0);
	ret=fcntl(lsock, F_SETFL, ~O_NONBLOCK & flags);

#endif

	return(ret);
}

int
set_sock_send_timeout(SOCKET lsock, int secs)
{
	int ret=-1;
	struct timeval tv;

#if defined(WINDOWS)
	int timeout=secs*100;
	ret = setsockopt(lsock,SOL_SOCKET ,SO_SNDTIMEO,(char *)&timeout,sizeof(timeout));
#else
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) ) < 0)
	{
		
	}
#endif
	return(ret);
}


//setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv);
int
set_sock_recv_timeout(SOCKET lsock, int secs)
{
	int ret=-1;
	struct timeval tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) ) < 0)
	{
		
	}
    DEBUG1("set recv timeout ret %d\n",ret);
	return(ret);
}


//
// Update the statistics file, should only be called periodically at most every 15 seconds, more likely every 1-5 min for MRTG or similar.
//
int
write_statistics(POLICY *policy)
{
	FILE *fp;
	
	if(!policy->stats_file)
		return(-1);
	if(0==strlen((char*)policy->stats_file))
		return(-1);

	DEBUG2(" writing stats at %s\n",policy->stats_file);

	if(NULL == (fp = fopen((char*)policy->stats_file, "w")) )				// fopen_s for windows?
		return -1;

   	fprintf(fp,"policy_requests = %ld;\n",policy->requests);

	// Last message
	fclose(fp);
	return(0);
}

int
init_server(POLICY *policy)
{
    int ret=-1;
	struct sockaddr_in	client;				/* Information about the client */

	/* gety a tcp socket */
	policy->listen_soc = socket(AF_INET, SOCK_STREAM, 0);
	if (policy->listen_soc == INVALID_SOCKET)
	{
		DEBUG1("Could not create socket.\n");
        policy->listen_soc=0;
		return(-1);
	}
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));   
	client.sin_family	    = AF_INET;					                // host byte order
	client.sin_port		    = htons(policy->listen_port);				// listen port
    client.sin_addr.s_addr  = policy->Bind_IP.ip32;                     // client.sin_addr.s_addr
	ret=bind(policy->listen_soc, (struct sockaddr *)&client, sizeof(struct sockaddr_in));
	if(ret==-1)
	{
		DEBUG1("fail to bind\n");
		closesocket(policy->listen_soc);
        policy->listen_soc=0;
		return(-1);
	}
    //
    // If your needing very high load, increase the proxy_listen_backlog value, this will 
    // let more connections stack up and may help with a loaded server.  The default
    // value of 10 should be more than enough for most servers.
    //
	ret=listen(policy->listen_soc,POLICY_LISTEN_BACKLOG);
	if(-1==ret)
	{
		closesocket(policy->listen_soc);
        policy->listen_soc=0;
		return(-1);
	}
	// Proxy Listener started, lets create the client info

	DEBUG1("listener started\n");

    // Add the listen socket to the select map
    Y_Set_Select_rx(policy->listen_soc);

    // Non Block
    set_sock_nonblock(policy->listen_soc);

    return(1);

}

int
policy_load_policy_file(POLICY *policy,int flen)
{
    int     tret,ret=-1;
    char    *tbuffer;
    FILE    *fp=0;


    while(flen>0)
    {
	    if(NULL == (fp = fopen( (char *) policy->policy_file, "r")) )
	    {
		    yprintf("cannot open %s config file.\n",policy->config_file);
	        break;
        }
	    else
	    {
            // Alloc buffer and read file into buffer
            tbuffer=(char*)malloc(flen+1);
            if(0==tbuffer)
            {
                DEBUG1("failed to malloc %d bytes\n",flen+1);
                ret=-2;
            }
            //
            // Read file to buffer.
            //
            tret=fread(tbuffer,1,flen,fp);
            if((tret)<1)
            {
                free(tbuffer);
                break;
            }
            tbuffer[tret]=0;

            // OK we got a good
            if(policy->policy)
                free(policy->policy);
            policy->policy=tbuffer;
            ret=0;
        }
        // ALways break.
        break;
    }
    // close the file if open
    if(fp)
        fclose(fp);

    return(ret);
}

//
// This checks if the policy file needs to be reloaded, and if so reloads it.
// This function just checks the modify time attribute on the file against what
// is stored, and if there is a change reloads it.
//
int
policy_reload_policy_file(POLICY *policy)
{
	time_t old_time=policy->policy_file_info.st_mtime;


	if(-1!=stat(policy->policy_file,&policy->policy_file_info))
	{		
		// Must use difftime to be completly portable
		if( (difftime(old_time, policy->policy_file_info.st_mtime)) || (0==old_time) )
		{
			// File has changed, reload
            DEBUG1("Reload Policy File\n");
			if(policy->verbose) printf("Reload Policy File\n");
			return(policy_load_policy_file(policy,policy->policy_file_info.st_size));
		}
	}
	else
	{
		if(policy->verbose) printf("Failed stat %s\n",policy->policy_file);
	}
	return(0);
}


int
policy_rx(POLICY *policy)
{
    SOCKET				ts;
    int                 count=0;
    int                 ret;
    int                 len;
	struct sockaddr_in	client;				/* Information about the client */
#if defined(WEB_TEST)
    char                tbuf[10001];
#endif

    // Check for anything ready, wait for 1 second
    ret=Y_Select(1000);
    if(ret>0)
    {
        //if(policy->verbose) printf("selected\n");
        // Check to see if we can accept on the socket
        if(Y_Is_Select(policy->listen_soc))
        {   
            do
            {
                // Accept the socket
                memset((void *)&client, '\0', sizeof(struct sockaddr_in));
                len=sizeof(struct sockaddr);
 			    ts=accept(policy->listen_soc,(struct sockaddr *)&client, (socklen_t *) &len);


			    if(-1!=ts)
			    {
                    policy->requests++;
                    set_sock_block(ts);
                    set_sock_send_timeout(ts, 1);
#if defined(WEB_TEST)                    
                    set_sock_recv_timeout(ts, 1000);
                    //
                    // Dummy Read?  Cannot block
                    ret=recv(ts,tbuf,1000,0);
                    // Accepted, just dump the policy file and close the socket
                    sprintf(tbuf,"HTTP/1.0 200 OK\r\nConnection: Close\r\n\r\n%s",policy->policy);
				    ret=send(ts,tbuf,strlen(tbuf),0);				  
#endif

                    ret=send(ts,policy->policy,strlen(policy->policy),0);
                    if(-1==ret)
                        policy->tx_err++;
				    // Close the socket
				    closesocket(ts);
                }
                else
                {
                    policy->accept_err++;
                }
                count++;
            }while((-1!=ts) && (count<100));            /*we only handle 100 accepts before checking our other stuff, this can be optimized */
        }
        else
        {
            DEBUG1("error on select not found\n");
        }
    }
    return(-1);
}


#if defined(WIN32)
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch(CEvent)
    {
    case CTRL_C_EVENT:
		yprintf("CTRL+C received!\n");
        break;
    case CTRL_BREAK_EVENT:
		yprintf("CTRL+BREAK received!\n");
        break;
    case CTRL_CLOSE_EVENT:
		yprintf("program is being closed received!\n");
        break;
    case CTRL_SHUTDOWN_EVENT:
		yprintf("machine is being shutdown!\n");
		break;
    case CTRL_LOGOFF_EVENT:
		return FALSE;
    }
	go=0;

    return TRUE;
}

#else

void
termination_handler (int signum)
{

	//write_statistics(global_chat_ptr);
	go=0;	

    if((SIGFPE==signum) || (SIGSEGV==signum) || (11==signum))
    {
        yprintf("Flash Policy Server Terminated from Signal %d\n",signum);
		if(global_flag&GF_DAEMON) syslog(LOG_ERR,"Flash Policy Server Terminated from Signal 11\n");

        exit(11);
    }
}
#endif


void
startup_banner()
{
	//------------------------------------------------------------------
	// Print Banner
	//------------------------------------------------------------------
	printf("Flash Policy Server built " __DATE__ " at " __TIME__ "\n");
	printf("   Version " VERSION " - (c)2015 Mycal.net\n");
	fflush(stdout);	
}


void usage(int argc, char **argv)
{
  startup_banner();

  printf("usage: %s [-h] [-v(erbose)] [-d][pid file] [-f config_file] [-p policy_file] [-a(uto reload)] [-l listen_tcp_port] \n",argv[0]);
  printf("\t -h this output.\n");
  printf("\t -v console debug output.\n");
  printf("\t -d runs the program as a daemon with optional pid file.\n");
  printf("\t -f specify a config file.\n");
  printf("\t -p specify a policy file.\n");
  printf("\t -a interval_in_seconds (0=off)\n");
  printf("\t -l Listen port (defaults to 843)\n");
  exit(2);
}

int main(int argc, char *argv[])
{
POLICY  		policy;
int				c;


#if defined(LINUX) || defined(MACOSX)
	signal(SIGPIPE, SIG_IGN);
#endif


	go=1;

	//
	// Clean the whole policy structure
	memset(&policy,'\0',sizeof(POLICY));
	// Set default config file
	strcpy(policy.config_file,POLICY_DEFAULT_CONFIG_FILE);

	//
	// Set Defaults, can be overwritten later by config file, but will operate on these if it does not exist
	//
	policy.listen_port=POLICY_DEFALT_LISTEN_PORT;
    policy.verbose=0;
    policy.Bind_IP.ip32=0;
    //
    // Set default policy
    //
    strcpy(policy.policy_file,POLICY_DEFAULT_FILE);

	// Set update server and filter files
	//
	//default stats updated once every 60 seconds
    //
	strcpy(policy.stats_file,POLICY_DEFAULT_STATISTICS_FILE);
    policy.stats_interval=POLICY_DEFAULT_STATISTICS_INTERVAL;
	//
	// Banner
	startup_banner();
	
	// Startup Network
	network_init();
    Y_Init_Select();


	//------------------------------------------------------------------
	// Initialize error handling and signals
	//------------------------------------------------------------------
#if defined(WIN32) 
if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
{
    // unable to install handler... 
    // display message to the user
    yprintf("Error - Unable to install control handler!\n");
    exit(0);
}
#else 
#if !defined(WINCE)
	//	SetConsoleCtrlHandle(termination_handler,TRUE);

	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);
	if (signal (SIGILL , termination_handler) == SIG_IGN)
		signal (SIGILL , SIG_IGN);
	if (signal (SIGFPE , termination_handler) == SIG_IGN)
		signal (SIGFPE , SIG_IGN);
	//if (signal (SIGSEGV , termination_handler) == SIG_IGN)
	//	signal (SIGSEGV , SIG_IGN);
#if defined(LINUX) || defined(MACOSX) || defined(IOS)
	if (signal (SIGXCPU , termination_handler) == SIG_IGN)
		signal (SIGXCPU , SIG_IGN);
	if (signal (SIGXFSZ , termination_handler) == SIG_IGN)
		signal (SIGXFSZ , SIG_IGN);
#endif
#endif
#endif
    //
    // Look just for a config file first
    //
	while ((c = getopt(argc, argv, "p:c:u:t:l:a:d:vh")) != EOF)
	{
    	switch (c) 
		{
    	case 0:
    		break;
    	case 'f':
    		//Policy_file
			strncpy(policy.config_file,optarg,MAX_PATH-1);
    		break;
    	default:
			break;
    	}
    }
    optind = 1;
    //
    // Load Config File if set
    //
	if(strlen(policy.config_file))
	{
		if(read_config(&policy))
		{
			if(policy.verbose) printf("Config File Loaded\n");
		}
		else
		{
			if(policy.verbose) printf("Config File Failed to Load, using defaults.\n");
		}
   	}

	//
	// Override with command line, after config file is loaded
	//
	while ((c = getopt(argc, argv, "p:c:u:t:l:a:d:vh")) != EOF)
	{
    		switch (c) 
			{
    		case 0:
    			break;
    		case 'l':
    		    //Policy_file
                policy.listen_port=atoi(optarg);
    		    break;
    		case 'p':
    		    //Policy_file
				strncpy(policy.policy_file,optarg,MAX_PATH-1);
    		    break;
    		case 'd':
				// Startup as daemon with pid file
				strncpy(policy.pidfile,optarg,MAX_PATH-1);
			    printf("Starting up as daemon with pidfile %s\n",policy.pidfile);
                global_flag|=GF_DAEMON;
    			break;
    		case 'a':
    			policy.auto_reload=atoi(optarg);;
    			break;
    		case 'v':
    			policy.verbose=1;
    			break;
    		case 'h':
    			usage (argc,argv);
    			break;
    		default:
    			usage (argc,argv);
				break;
    	}
    }
	argc -= optind;
	argv += optind;
	
	//if (argc != 1)
	//	usage (argc,argv);


	// Read Policy File
    if(strlen(policy.policy_file))
        policy_reload_policy_file(&policy);
    else
    {
        // use default policy

    }

    // Bind TCP
    if(-1==init_server(&policy))
    {
        printf("could not bind server port %d.%d.%d.%d:%d, exiting.\n",policy.Bind_IP.ipb1,policy.Bind_IP.ipb2,policy.Bind_IP.ipb3,policy.Bind_IP.ipb4,
                policy.listen_port);
        exit(-1);
    }

#if !defined(WIN32)
    //
    // Should Daemonize here
    //
	if(global_flag&GF_DAEMON)
	{
        // Daemonize this
        daemonize(policy.pidfile,0,0,0,0,0,0);
        // Setup logging
		openlog("chat_server",LOG_PID|LOG_CONS,LOG_USER);
		syslog(LOG_INFO,"Flash Policy Server built "__DATE__ " at " __TIME__ "\n");
		syslog(LOG_INFO,"   Version " VERSION " - (c)2015 Mycal.net\n");
		syslog(LOG_INFO,"Starting up as daemon\n");	       
    }
#endif

	//
	// Main Loop Forever, we should exit on HUP or program exit, timeout every 1s if no packet to handle housekeeping
    //
	if(policy.verbose) printf("Starting Flash Policy Server\n");	
	
    go=10;
    while(go)
	{
		// Everything fun happens in rx_packet, this is the server
		policy_rx(&policy);
        //
        // Check if we need to check policy file for reload
        //
        if(policy.auto_reload)
        {
            if((U32)(second_count()-policy.policy_file_timestamp) > policy.auto_reload)
            {
                policy.policy_file_timestamp=second_count();	
                policy_reload_policy_file(&policy);
            }
        }
        //
        if(policy.stats_file[0])
        {
		    if((U32)(second_count()-policy.stats_file_timestamp) > policy.stats_interval)
		    {
			    policy.stats_file_timestamp=second_count();	
			    //
			    // Write out statistics
			    //
			    write_statistics(&policy);
			    fflush(stdout);	
		    }
        }
	}



	// Should never exit, but if we do cleanup and print statistics
	if(policy.verbose) printf("Exiting On Go = 0\n");	
	fflush(stdout);	
	
	return(0);
}


