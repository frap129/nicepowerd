#include <stdbool.h>
#include <signal.h>
#include <getopt.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "../include/nicepower.h"

#define NAME	"nicepowerd"

static struct npd_state *state;
static FILE *output;

int activate_profile(char profile[]) {
    // Build profile path
    char exec_path[MAX_PATH_LEN + MSG_LEN];
    strcpy(exec_path, state->profile_path);
    strcat(exec_path, "/");
    strncat(exec_path, profile, MAX_PATH_LEN);

    // Execute profile script
    fprintf(output, "Executing: %s\n", exec_path);
    system(exec_path);

    return 0;
}

int set_profile(char profile[]) {
    strncpy(state->active_profile, profile, MSG_LEN);
    return activate_profile(profile);
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

    while (state->running) {
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
            nbytes = sendto(sock, state->active_profile, nbytes, 0,
                             (struct sockaddr *) & name, size);
        } else if (strcmp(message, KILL_DAEMON) == 0) {
            state->running = 0;
        } else {
            // Set current profile
            strncpy(state->active_profile, message, MSG_LEN);
            activate_profile(state->active_profile);
            fprintf(output, "profile set: %s\n", state->active_profile);
        }
    }
    return NULL;
}

void update_power_state(struct npd_state *state) {
    // Update AC state
    FILE *ac = fopen(AC_PATH, "r");
    char charging[1];
    fgets(charging, 2, ac);
    fclose(ac);
    state->ac_state = atoi(charging);

    // Update battery state
    FILE *battery = fopen(BAT_PATH, "r");
    char charge[3];
    fgets(charge, 4, battery);
    fclose(battery);
    int percent = atoi(charge);
    if (percent >= BAT_HIGH_THRESH) {
        state->bat_state = bat_high;
    } else if (percent <= BAT_LOW_THRESH) {
        state->bat_state = bat_low;
    } else {
        state->bat_state = bat_norm;
    }
}

int nicepowerd(struct npd_state *state) {
    pthread_t thread_id;
    char selected_profile[MSG_LEN];

    // Setup logging
#ifndef DEBUG
    output = fopen(LOG_PATH, "w");
#else
    output = stdout;
#endif
    fprintf(output, "Using profile directory: %s\n", state->profile_path);

    // Initialize profile vars
    strncpy(selected_profile, PROFILE_MID, MSG_LEN);
    strncpy(state->active_profile, PROFILE_MID, MSG_LEN);
    fflush(output);

    // Create socket listener thread
    pthread_create(&thread_id, NULL, nicepowerctl, NULL);

    // Set profile and start monitoring
    set_profile(state->active_profile);
    while (state->running) {
        // Check if profile was manually set
        if (strcmp(selected_profile, state->active_profile) != 0) {
            // Sleep longer to maintain user-set profile
            strncpy(selected_profile, state->active_profile, MSG_LEN);
            sleep(XLONG_INTERVAL);
        }

        // Check battery level
        update_power_state(state);

        // Check if profile should be switched
        if (state->bat_state == bat_high && state->ac_state && strcmp(state->active_profile, PROFILE_HIGH) != 0) {
            strncpy(selected_profile, PROFILE_HIGH, MSG_LEN);
            set_profile(selected_profile);
            fprintf(output, "CHARGING: profile set: %s\n", state->active_profile);
            fflush(output);
        } else if (state->bat_state == bat_low && !state->ac_state) {
            if (strcmp(state->active_profile, PROFILE_LOW) != 0) {
                // If low battery threshold is met, set power profile
                strncpy(selected_profile, PROFILE_LOW, MSG_LEN);
                set_profile(selected_profile);
                fprintf(output, "LOW BATTERY: profile set: %s\n", state->active_profile);
                fflush(output);
            }

            // Sleep LONG_INTERVAL to avoid power draw from this daemon
            sleep(XLONG_INTERVAL);
        } else if ((state->bat_state == bat_norm || !state->ac_state) && strcmp(state->active_profile, PROFILE_MID) != 0) {
            strncpy(selected_profile, PROFILE_MID, MSG_LEN);
            set_profile(selected_profile);
            fprintf(output, "CHARGING or BATTERY OK: profile set: %s\n", state->active_profile);
            fflush(output);
        }

        sleep(SHORT_INTERVAL);
    }

    return 0;
}

int main(int argc , char *argv[]) {
    bool daemon = false;
    int option;

    static struct option long_options[] = {
        { "daemon", no_argument, 0, 'd' },
        { "config", required_argument, 0, 'c' },  
        { "help", no_argument, 0, 'h' }
    };
    static const char *short_options = "dc:h";
    
    while((option = getopt_long(argc, argv, short_options, long_options, 0)) != -1) {
        switch(option){
            case 'd':
                daemon = true;
                break;
            case 'c':
                strncpy(state->profile_path, optarg, MAX_PATH_LEN);
                break;                
            case ':':
            case '?':
            default:
                printf("unknown option: %c\n", optopt);
                break;
        }
    }

    // Copy defaults to state
    state = malloc(MAX_PATH_LEN+(2*MSG_LEN)+2*sizeof(int));
    strncpy(state->profile_path, CONFIG_DIR, MAX_PATH_LEN);
    mkdir(CONFIG_DIR, 760);

    // Start as daemon if asked
    if (daemon)
        daemonize();

    // Catch, ignore and handle signals
    // TODO: Implement a working signal handler
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
        
    // Start main loop
    state->running = 1;
    nicepowerd(state);
    free(state);

    return 0;
}
