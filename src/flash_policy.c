/*!									
 *---------------------------------------------------------------------------
 *! \file Flash_Policy.c
 *  \brief Flash Policy Server
 *
 *  Simple Flash Policy server, specify policy on file, auto reload of
 *  Policy file while running.
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
 * 
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
"<!-- This is an empty file, please specify a policy file -->\n"
"</cross-domain-policy>\n";

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
	ret=listen(policy->listen_soc,POLICY_LISTEN_BACKLOG);
	if(-1==ret)
	{
		closesocket(policy->listen_soc);
        policy->listen_soc=0;
		return(-1);
	}
	// Proxy Listener started, lets create the client info

	DEBUG1("listener started\n");

    Y_Set_Select_rx(policy->listen_soc);

    return(1);

}



int
policy_rx(POLICY *policy)
{
    SOCKET				ts;
    int                 ret;
    int                 len;
	struct sockaddr_in	client;				/* Information about the client */

    // Check for anything ready, wait for 1 second
    ret=Y_Select(1000);
    if(ret>0)
    {
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
                    // Accepted, just dump the policy file and close the socket
				    ret=send(ts,default_policy,strlen(default_policy),0);
				    // Close the socket
				    closesocket(ts);
                }
            }while(-1!=ts);
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
	printf("   Version " VERSION " - (c)2014 Mycal.net\n");
	fflush(stdout);	
}


void usage(int argc, char **argv)
{
  startup_banner();

  printf("usage: %s [-h] [-v(erbose)] [-d][pid file] [-f config_file] [-c control_port] [-u dns_udp_port] [-t dns_tcp_port] \n",argv[0]);
  printf("\t -h this output.\n");
  printf("\t -v console debug output.\n");
  printf("\t -d runs the program as a daemon with optional pid file.\n");
  printf("\t -f specify a config file.\n");
  printf("\t -l Listen port (defaults to 843)\n");
  exit(2);
}

int main(int argc, char *argv[])
{
POLICY  		policy;
int				range_len=0;
int				c;
U32				timestamp=second_count();

#if defined(LINUX) || defined(MACOSX)
/* Our process ID and Session ID */
pid_t			pid, sid;

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
    // Set default data files
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
	//
	//
	while ((c = getopt(argc, argv, "f:c:u:t:l:dvh")) != EOF)
	{
    		switch (c) 
			{
    		case 0:
    			break;
    		case 'f':
    		    //Policy_file
				strncpy(policy.policy_file,optarg,MAX_PATH-1);
    		    break;
    		case 'd':
				// Startup as daemon with pid file
				printf("Starting up as daemon\n");
				strncpy(policy.pidfile,optarg,MAX_PATH-1);
                global_flag|=GF_DAEMON;
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

	// Read File Config
	if(strlen(policy.config_file))
	{
		/*if(read_file_config(policy.config_file, &policy))
		{
			if(policy.verbose) printf("Config File Loaded\n");
		}
		else
		{
			if(policy.verbose) printf("Config File Failed to Load, using defaults.\n");
		}
        */
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
            daemonize(0,0,0,0);

            // Setup logging
			openlog("chat_server",LOG_PID|LOG_CONS,LOG_USER);
			syslog(LOG_INFO,"Flash Policy Server built "__DATE__ " at " __TIME__ "\n");
			syslog(LOG_INFO,"   Version " VERSION " - (c)2014 Mycal.net\n");
			syslog(LOG_INFO,"Starting up as daemon\n");
	       

            // create pid file
			if(pidfile)
			{
				FILE *fd;
				// pidfile creation specified
				fd=fopen(argv[pidfile],"w");
				if(fd)
				{
					fprintf(fd,"%d",getpid());
					fclose(fd);
					syslog(LOG_INFO,"Creating pidfile %s with PID %d\n",argv[pidfile],getpid());
				}
				else
				{
					syslog(LOG_ERR,"Failed creating pidfile %s with PID %d -errno %d\n",argv[pidfile],getpid(),errno);	
					exit(0);
				}
			}
    }
#endif

	//------------------------------------------------------------------
	// Initialize error handling and signals
	//------------------------------------------------------------------
#if defined(WIN32) 
if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
{
    // unable to install handler... 
    // display message to the user
    printf("!!Error - Unable to install control handler!\n");
    return -1;
}
#else 
	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);
	if (signal (SIGILL , termination_handler) == SIG_IGN)
		signal (SIGILL , SIG_IGN);
	if (signal (SIGFPE , termination_handler) == SIG_IGN)
		signal (SIGFPE , SIG_IGN);
	if (signal (SIGSEGV , termination_handler) == SIG_IGN)
		signal (SIGSEGV , SIG_IGN);
	if (signal (SIGXCPU , termination_handler) == SIG_IGN)
		signal (SIGXCPU , SIG_IGN);
	if (signal (SIGXFSZ , termination_handler) == SIG_IGN)
		signal (SIGXFSZ , SIG_IGN);
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
		// Do Checks and write statistics every 60 seconds, we also check for reload of policy file
		//
		if((second_count()-timestamp)> policy.stats_interval)
		{
			//if(dns.verbose) printf("Try Reload\n");
			timestamp=second_count();	
			//
			// Write out statistics
			//
			write_statistics(&policy);
            //
            // Check Policy File
            //
			fflush(stdout);	
		}
	}



	// Should never exit, but if we do cleanup and print statistics
	if(policy.verbose) printf("Exiting On Go = 0\n");	
	fflush(stdout);	
	
	return(0);
}


