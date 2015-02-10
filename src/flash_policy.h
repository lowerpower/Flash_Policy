#ifndef __Y_FLASH_POLICY_H__
#define __Y_FLASH_POLICY_H__
//---------------------------------------------------------------------------
// flash_policy.h - Flash Policy Server         						-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version Oct 5, 2014     							-
//																			-
// (c)2014 Mycal.net									-
//---------------------------------------------------------------------------
#include "config.h"
#include "mytypes.h"


#if defined(WIN32)
#define POLICY_DEFAULT_FILE			"c:/flash/policy.txt"
#define POLICY_DEFAULT_CONFIG_FILE	"c:/flash/pconfig.txt"
#else
#define POLICY_DEFAULT_FILE			"/etc/flash/policy.txt"
#define POLICY_DEFAULT_CONFIG_FILE	"/etc/flash/pconfig.txt"
#endif

#if defined(WIN32)
#define POLICY_DEFAULT_STATISTICS_FILE  "c:/flash/policy_stats.txt"
#else
#define POLICY_DEFAULT_STATISTICS_FILE  "/tmp/policy_stats.txt"
#endif
#define POLICY_DEFAULT_STATISTICS_INTERVAL 30


#define POLICY_LISTEN_BACKLOG       10
#define POLICY_DEFALT_LISTEN_PORT   843


//
// GF flags, global flags
//
#define	GF_GO			0x01				/* go */
#define GF_DAEMON		0x02				/* we are a daemon */
#define GF_QUIET		0x04				/* no output */
#define GF_WRITESTATS	0x08				



// Custom File config for each product here
typedef struct policy_config_
{                      
	IPADDR		Bind_IP;
    U16         listen_port;
	SOCKET		listen_soc;
    char        config_file[MAX_PATH];
    //
	int			verbose;
	int			log_level;
	int			auto_reload;
	char        policy_file[MAX_PATH];
    
    // Stats
	char        stats_file[MAX_PATH];
    unsigned int stats_interval;
    long		requests;


	char		pidfile[MAX_PATH];
}POLICY;

#endif

