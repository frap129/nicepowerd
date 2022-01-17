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

#define NAME	"nicepowerd"

static FILE *output;
static struct npd_state *npd_options;
extern int make_named_socket(const char *filename);

int set_profile(char *profile) {
	// Build profile path
	char exec_path[MAX_PATH_LEN];
	strncpy(exec_path, npd_options->profile_path, MAX_PATH_LEN);
	strncat(exec_path, "/", MAX_PATH_LEN);
	strncat(exec_path, profile, MAX_PATH_LEN);
	strncat(exec_path, ".sh", MAX_PATH_LEN);

	// Execute profile script
	char *args[3] = {exec_path, NULL};
	execv(args[0], args);

	return 0;
}

void *nicepowerctl() {
    char message[MSG_LEN];
    int sock;
    struct sockaddr_un name;
    socklen_t size;
    int nbytes;

	fprintf(output, "Starting ctl listener\n");

    // Setup socket
    mkdir(SOCKET_DIR, 0766);
    unlink(DAEMON_SOCKET);
    sock = make_named_socket(DAEMON_SOCKET);

    while (1) {
    	fprintf(output, "Waiting for message from nicepowerctl\n");

        /* Wait for a message. */
        size = sizeof(name);
        nbytes = recvfrom(sock, message, MSG_LEN, 0,
                           (struct sockaddr *) & name, &size);

		// Ignore message transmission errors
        if (nbytes < 0) {
			continue;
        }

     	// Check if message is a get request
        if (strcmp(message, GET_PROFILE) == 0) {
        	// Reply with active profile
        	nbytes = sendto(sock, npd_options->active_profile, nbytes, 0,
        	                 (struct sockaddr *) & name, size);
        } else {
        	// Set current profile
        	strncpy(npd_options->active_profile, message, MSG_LEN);
        	set_profile(npd_options->active_profile);
        	fprintf(output, "profile set: %s\n", npd_options->active_profile);
        }
    }
}

int nicepowerd(struct npd_state *npd_options) {
    pthread_t thread_id;
	char prev_profile[MSG_LEN];

	// Setup logging
#ifndef DEBUG
    output = fopen(LOG_PATH, "w");
#else
    output = stdout;
#endif
	fprintf(output, "Using default profile: %s\n", npd_options->default_profile);
	fprintf(output, "Using profile directory: %s\n", npd_options->profile_path);

	// Initialize profile vars
	npd_options->active_profile = malloc(MSG_LEN);
	strncpy(prev_profile, npd_options->default_profile, MSG_LEN);
	strncpy(npd_options->active_profile, npd_options->default_profile, MSG_LEN);
	fflush(output);

	// Create socket listener thread
    pthread_create(&thread_id, NULL, nicepowerctl, NULL);

    // Set profile and start monitoring
	set_profile(npd_options->active_profile);
    while (1) {
		// Check if profile was manually set
    	if (strcmp(prev_profile, npd_options->active_profile) != 0) {
    		// Sleep longer to maintain user-set profile
    		strncpy(prev_profile, npd_options->active_profile, MSG_LEN);
    		sleep(LONG_INTERVAL);
    		continue;
    	}
    	
		// Do something
		fprintf(output, "I\'ll manage profiles here\n");
		fflush(output);
		sleep(SHORT_INTERVAL);
    }

	return 0;
}

int main(int argc , char *argv[]) {
	// Copy defaults to state
	npd_options = malloc(MAX_PATH_LEN+ (2*MSG_LEN));
	strncpy(npd_options->default_profile, PROFILE_MID, MSG_LEN);
	strncpy(npd_options->profile_path, CONFIG_DIR, MAX_PATH_LEN);
	mkdir(CONFIG_DIR, 760);
	
    int option;
    while((option = getopt(argc, argv, ":d:p:")) != -1) {
        switch(option){
         	case 'd':
         		strncpy(npd_options->default_profile, optarg, MSG_LEN);
            	break;
         	case 'p': //here f is used for some file name
           		strncpy(npd_options->profile_path, optarg, MAX_PATH_LEN);
            	break;
            default:
         	case '?':
            	printf("unknown option: %c\n", optopt);
            	break;
      }
   }

    // Print status prior to start
	fprintf(stdout, "Starting daemon\n");
	fflush(stdout);

#ifndef DEBUG
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

#ifndef DEBUG
    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
        close (x);
    }
#endif

    // Start daemon
    nicepowerd(npd_options);
	return 0;
}
