#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#include "nicepower.h"


#define SOCKET_DIR 		"/run/nicepowerd"
#define DAEMON_SOCKET 	"/run/nicepowerd/nicepowerd"
#define GET_PROFILE 	"get"
#define PROFILE_HIGH 	"performance"
#define PROFILE_MID 	"balance"
#define PROFILE_LOW		"power"
#define MSG_LEN			sizeof (PROFILE_HIGH) - 1
#define SHORT_INTERVAL	5
#define LONG_INTERVAL 	30

extern int make_named_socket(const char *filename);

int set_profile(char *profile) {
	// TODO
	return 0;
}

void *nicepowerctl(void *arg) {
    char message[MSG_LEN];
    int sock;
    struct sockaddr_un name;
    socklen_t size;
    int nbytes;

	printf("Starting ctl listener\n");

	// Get active profile pointer
	char **profile = (char**)arg;

    // Setup socket
    mkdir(SOCKET_DIR, 0760);
    unlink(DAEMON_SOCKET);
    sock = make_named_socket(DAEMON_SOCKET);

    while (1) {
    	fprintf(stdout, "waiting for ctl message\n");
    	
        /* Wait for a message. */
        size = sizeof (name);
        nbytes = recvfrom (sock, message, MSG_LEN, 0,
                           (struct sockaddr *) & name, &size);

		// Ignore message transmission errors
        if (nbytes < 0) {
			continue;
        }

     	// Check if message is a get request
        if (strcmp(message, GET_PROFILE) == 0) {
        	// Reply with active profile
        	nbytes = sendto (sock, *profile, nbytes, 0,
        	                 (struct sockaddr *) & name, size);
        } else {
        	// Set current profile
        	strncpy(*profile,  message, MSG_LEN);
        	set_profile(*profile);
        }
    }
}

int nicepowerd() {
    pthread_t thread_id;
	char prev_profile[MSG_LEN] = PROFILE_MID;
	char profile[MSG_LEN] = PROFILE_MID;

	// Create socket listener thread
    pthread_create(&thread_id, NULL, nicepowerctl, &profile);

    // Set profile and start monitoring
	set_profile(profile);
    while (1) {
		// Check if profile was manually set
    	if (strcmp(prev_profile, profile) != 0) {
    		// Sleep longer to maintain user-set profile
    		strncpy(prev_profile, profile, MSG_LEN);
    		sleep(LONG_INTERVAL);
    		continue;
    	}
    	
		// Do something
		fprintf(stdout, "I\'ll manage profiles here\n");
		fflush(stdout);
		sleep(SHORT_INTERVAL);
    }

	return 0;
}

int main(int argc , char *argv[]) {
    
	fprintf(stdout, "Starting daemon\n");
	fflush(stdout);

#ifdef DEBUG
    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        exit(EXIT_FAILURE);
#endif

    // Catch, ignore and handle signals
    // TODO: Implement a working signal handler
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // Set new file permissions
    umask(0);

    /* Change the working directory to the root directory
        or another appropriated directory */
    chdir("/");

#ifdef DEBUG
    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
        close (x);
    }
#endif

    // Start daemon
    nicepowerd();
	return 0;
}
