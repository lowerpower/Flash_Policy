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

// Uncomment this line so you can use a web performance test like ab to test server performance (makes this server act like a webserver)
//#define WEB_TEST 1

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
    struct stat config_file_info;
    //
	int			verbose;
	int			log_level;
    //
    // Policy
    char        *policy;
    //
    // Policy File Info
	U32 		auto_reload;
	char        policy_file[MAX_PATH];
    U32         policy_file_timestamp;
    struct stat policy_file_info;
    
    // Stats
	char        stats_file[MAX_PATH];
    U32         stats_interval;
    U32         stats_file_timestamp;
    // Stat Values
    long		requests;
    long        accept_err;
    long		tx_err;

	char		pidfile[MAX_PATH];
}POLICY;

#endif

