#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>


#define CONFIG_DIR		"/etc/nicepowerd"
#define LOG_PATH		"/var/log/nicepowerd"
#define AC_PATH		"/sys/class/power_supply/AC/online"
#define BAT_PATH		"/sys/class/power_supply/BAT0/capacity"
#define SOCKET_DIR		"/run/nicepowerd"
#define DAEMON_SOCKET	"/run/nicepowerd/nicepowerd"
#define CTL_SOCKET		"\0nicepowerctl"

#define KILL_DAEMON		"stop"
#define GET_PROFILE		"get"
#define PROFILE_HIGH	"performance"
#define PROFILE_MID		"balance"
#define PROFILE_LOW		"power"

#define MSG_LEN			sizeof(PROFILE_HIGH)
#define MAX_PATH_LEN	512
#define SHORT_INTERVAL	5
#define LONG_INTERVAL	30
#define XLONG_INTERVAL	1000

#define BAT_LOW_THRESH	20
#define BAT_HIGH_THRESH	80

enum battery_state { bat_high, bat_norm, bat_low };

struct npd_state {
    char default_profile[MSG_LEN];
    char profile_path[MAX_PATH_LEN];
    char active_profile[MSG_LEN];
    enum battery_state bat_state;
    int ac_state;
    int running;
};

int make_named_socket(const char *filename) {
    struct sockaddr_un name;
    int sock;
    size_t size;

    /* Create the socket. */
    sock = socket (PF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror ("socket");
        exit (EXIT_FAILURE);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy (name.sun_path, filename, sizeof (name.sun_path));
    name.sun_path[sizeof (name.sun_path) - 1] = '\0';
    size = SUN_LEN (&name);

    if (bind (sock, (struct sockaddr *) &name, size) < 0) {
        perror ("bind");
        exit (EXIT_FAILURE);
    }

    return sock;
}

void daemonize() {
    // Print status prior to start
    fprintf(stdout, "Starting daemon\n");
    fflush(stdout);

    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    // Set new file permissions
    umask(0);

    // Change directory to root
    chdir("/");

    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
        close (x);
    }
}
