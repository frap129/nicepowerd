#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
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

int set_profile(char *profile) {
    // Build profile path
    char exec_path[MAX_PATH_LEN + MSG_LEN];
    strcat(exec_path, npd_options->profile_path);
    strcat(exec_path, "/");
    strncat(exec_path, profile, MAX_PATH_LEN);

    // Execute profile script
    fprintf(output, "Executing: %s\n", exec_path);
    system(exec_path);

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
    mkdir(SOCKET_DIR, 0777);
    unlink(DAEMON_SOCKET);
    sock = make_named_socket(DAEMON_SOCKET);
    chmod(DAEMON_SOCKET, 0777);

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

int battery_level() {
    FILE *battery = fopen(BAT_PATH, "r");
    char value[3];
    fgets(value, 4, battery);
    return atoi(value);
}

int nicepowerd(struct npd_state *npd_options) {
    pthread_t thread_id;
    char selected_profile[MSG_LEN];

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
    strncpy(selected_profile, npd_options->default_profile, MSG_LEN);
    strncpy(npd_options->active_profile, npd_options->default_profile, MSG_LEN);
    fflush(output);

    // Create socket listener thread
    pthread_create(&thread_id, NULL, nicepowerctl, NULL);

    // Set profile and start monitoring
    set_profile(npd_options->active_profile);
    while (1) {
        // Check if profile was manually set
        if (strcmp(selected_profile, npd_options->active_profile) != 0) {
            // Sleep longer to maintain user-set profile
            strncpy(selected_profile, npd_options->active_profile, MSG_LEN);
            sleep(LONG_INTERVAL);
            continue;
        }

        // Check battery level
        int battery = battery_level();
        fprintf(output, "Battery Level: %d\n", battery);

        // Check if profile should be switched
        if (battery >= BAT_HIGH_THRESH) {
            npd_options->bat_state = bat_high;
        } else if (battery <= BAT_LOW_THRESH) {
            // If low battery threshold is met, set power profile
            npd_options->bat_state = bat_low;
            strncpy(selected_profile, PROFILE_LOW, MSG_LEN);
            strncpy(npd_options->active_profile, PROFILE_LOW, MSG_LEN);
            set_profile(npd_options->active_profile);
            fprintf(output, "LOW BATTERY: profile set: %s\n", npd_options->active_profile);
            fflush(output);

            // Sleep LONG_INTERVAL to avoid power draw from this daemon
            sleep(XLONG_INTERVAL);
            continue;
        } else {
            npd_options->bat_state = bat_norm;
        }

        fflush(output);
        sleep(SHORT_INTERVAL);
    }

    return 0;
}

int main(int argc , char *argv[]) {
    bool daemon = false;
    int option;

    static struct option long_options[] = {
        { "daemon", no_argument, 0, 'd' },
        { "profile", required_argument, 0, 'p' },
        { "config", required_argument, 0, 'c' },  
        { "help", no_argument, 0, 'h' }
    };
    static const char *short_options = "dp:c:h";
    
    while((option = getopt_long(argc, argv, short_options, long_options, 0)) != -1) {
        switch(option){
            case 'd':
                daemon = true;
                break;
            case 'p':
                strncpy(npd_options->default_profile, optarg, MSG_LEN);
                break;
            case 'c':
                strncpy(npd_options->profile_path, optarg, MAX_PATH_LEN);
                break;                
            case ':':
            case '?':
            default:
                printf("unknown option: %c\n", optopt);
                break;
        }
    }

    // Copy defaults to state
    npd_options = malloc(MAX_PATH_LEN+ (2*MSG_LEN));
    strncpy(npd_options->default_profile, PROFILE_MID, MSG_LEN);
    strncpy(npd_options->profile_path, CONFIG_DIR, MAX_PATH_LEN);
    mkdir(CONFIG_DIR, 760);

    // Start as daemon if asked
    if (daemon)
        daemonize();

    // Catch, ignore and handle signals
    // TODO: Implement a working signal handler
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
        
    // Start main loop
    nicepowerd(npd_options);

    return 0;
}
