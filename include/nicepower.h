#define CONFIG_DIR	 	"/etc/nicepowerd"
#define LOG_PATH	 	"/var/log/nicepowerd"
#define SOCKET_DIR 		"/run/nicepowerd"
#define DAEMON_SOCKET 	"/run/nicepowerd/nicepowerd"
#define GET_PROFILE 	"get"
#define PROFILE_HIGH 	"performance"
#define PROFILE_MID 	"balance"
#define PROFILE_LOW		"power"
#define MSG_LEN			sizeof (PROFILE_HIGH) - 1
#define MAX_PATH_LEN	512
#define SHORT_INTERVAL	5
#define LONG_INTERVAL 	30

struct npd_state {
	char default_profile[MSG_LEN];
    char profile_path[512];
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
