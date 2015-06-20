/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file file_config.c
 *  \brief Configuration file reader - this could be replaced by EEPROM 
 *		reader for embeded designes.
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version June 3, 2006									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Yoics Inc, www.yoics.com								-
 *                                                                         	-
 * $Date: 2006/08/29 20:35:55 $
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 *
 *  This module requires the following globals to be defined in the application:
 *
 *	char uid[8]				- uid of item.
 *  char remote_address[8]	- uid of remote client to connect to.
 *  char password[32]		- password.
 *	char YoicsServer1[64]	- YoicsServer1
 * 
 *
*/


#include "config.h"
#include "arch.h"
#include "flash_policy.h"
#include "debug.h"

#define MAX_LINE_SIZE	1024

// config file strings
#define		FLASH_LISTEN_PORT  			"listen_port"
#define		FLASH_VERBOSE 				"verbose"
#define		FLASH_POLICY_FILE			"policy_file"
#define		FLASH_POLICY_RELOAD 		"policy_autoreload"
#define		FLASH_STATS_FILE			"stats_file"
#define		FLASH_STATS_INTERVAL		"stats_interval"
#define		FLASH_BIND_IP       		"bind_ip"



int
read_config(POLICY *policy)
{
	char	*subst;
	char	*strt_p;
    int     ret=1;
	char	line[MAX_LINE_SIZE];
    FILE    *fp;

    // Read from file
	if(NULL == (fp = fopen( (char *) policy->config_file, "r")) )
	{
		yprintf("cannot open %s config file.\n",policy->config_file);
		ret=-1;
	}
	else
	{
        // File is open, read the config
		while(readln_from_a_file(fp, (char *) line, MAX_LINE_SIZE-4))
		{
			if(strlen((char *) line)==0)
				continue;

			subst=strtok_r((char *) line," \n",&strt_p);

			// Get Rid of whitespace
			while(*subst==' ')
				subst++;

			DEBUG1("readcmd->%s\n",subst);

			if(strlen( (char *) subst)==0)
			{
				// do nothing
            }
			else if(0==strcmp((char *) subst,FLASH_LISTEN_PORT))
			{
				subst=strtok_r(NULL," \n",&strt_p);
				policy->listen_port=(U16) atoi((char *) subst);		
			}
			else if(0==strcmp((char *) subst,FLASH_VERBOSE))
			{
				subst=strtok_r(NULL," \n",&strt_p);
				policy->verbose=(U16) atoi((char *) subst);		
			}
			else if(0==strcmp((char *) subst,FLASH_POLICY_RELOAD ))
			{
				subst=strtok_r(NULL," \n",&strt_p);
				policy->auto_reload=(U16) atoi((char *) subst);		
			}
			else if(0==strcmp(subst,FLASH_POLICY_FILE	))
			{
		        subst=strtok_r(NULL," \n",&strt_p);
        		strncpy(policy->policy_file,subst,MAX_PATH);
		        policy->policy_file[MAX_PATH-1]=0;
			}
			else if(0==strcmp(subst,FLASH_STATS_FILE	))
			{
		        subst=strtok_r(NULL," \n",&strt_p);
        		strncpy(policy->stats_file,subst,MAX_PATH);
		        policy->stats_file[MAX_PATH-1]=0;
			}
			else if(0==strcmp((char *) subst,FLASH_STATS_INTERVAL ))
			{
				subst=strtok_r(NULL," \n",&strt_p);
				policy->stats_interval=(U16) atoi((char *) subst);		
			}
			else if(0==strcmp((char *) subst,FLASH_BIND_IP))
			{
				subst=strtok_r(NULL,".\n",&strt_p);
				if(strlen((char *) subst))
					policy->Bind_IP.ipb1=atoi(subst);		

				subst=strtok_r(NULL,".\n",&strt_p);
				if(strlen((char *) subst))
					policy->Bind_IP.ipb2=atoi(subst);

				subst=strtok_r(NULL,".\n",&strt_p);
				if(subst)
					policy->Bind_IP.ipb3=atoi(subst);

				subst=strtok_r(NULL,".\n",&strt_p);
				if(subst)
					policy->Bind_IP.ipb4=atoi(subst);
			}
        }
    }
    if(fp)
        fclose(fp);
    return(1);
}