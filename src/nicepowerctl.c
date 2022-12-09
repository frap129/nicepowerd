#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
 
#include "nicepower.h"
#define NAME	"nicepowerctl"

void print_help() {
    // Print correct usage of program and its options
    printf("\n");
    printf("Usage: %s [command]\n\n", NAME);
    printf("Commands:\n");
    printf("\tget\t\tGet the active profile name.\n");
    printf("\tperformance\tSet the active profile to Performance.\n");
    printf("\tbalance\t\tSet the active profile to Balanced.\n");
    printf("\tpower\t\tSet the active profile to Low-Power.\n");
    printf("\tstop\t\tStop nicepowerd.\n");
}

int main (int argc , char *argv[]) {
    extern int make_named_socket (const char *name);
    int sock;
    char message[MSG_LEN];
    struct sockaddr_un name;
    size_t size;
    int nbytes;

    // Verify user has given a command, print help otherwise
    if (argc != 2) {
        print_help();
        return 0;    
    }

    // Validate argument
    if (strcmp(argv[1], PROFILE_HIGH) != 0 &&
        strcmp(argv[1], PROFILE_MID) != 0 &&
        strcmp(argv[1], PROFILE_LOW) != 0 && 
        strcmp(argv[1], GET_PROFILE) != 0 &&
        strcmp(argv[1], KILL_DAEMON) != 0) {
        printf("invalid command: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
      
    // Socket setup
    unlink(CTL_SOCKET);
    sock = make_named_socket(CTL_SOCKET);
    name.sun_family = AF_LOCAL;
    strcpy(name.sun_path, DAEMON_SOCKET);
    size = strlen(name.sun_path) + sizeof(name.sun_family);
      
    // Send message to daemon
    nbytes = sendto(sock, argv[1], MSG_LEN, 0, (struct sockaddr *) & name, size);
    if (nbytes < 0) {
        printf("nicepowerd is not running\n");
        exit(EXIT_FAILURE);
    }

    // Check if user sent a get request
    if (strcmp(argv[1], GET_PROFILE) == 0) {
        // Wait for reply
        nbytes = recvfrom(sock, message, MSG_LEN, 0, NULL, 0);
        if (nbytes < 0) {
            perror("error getting profile: ");
            exit(EXIT_FAILURE);
        }

        // Print current profile
        printf("%s\n", message);
    }
  
    // Cleanup
    remove(CTL_SOCKET);
    close(sock);
}
