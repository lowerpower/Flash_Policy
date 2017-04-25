# Flash_Policy
Flash Policy server config files

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

## install

install_redhad.sh - Most redhat/centos systems

install_ubuntu.sh - Most ubuntu/debian systems


