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
