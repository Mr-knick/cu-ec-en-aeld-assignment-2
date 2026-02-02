#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char* argv[]) {
    openlog("writer.c", LOG_PID | LOG_CONS, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Missing input values");
    }
    syslog(LOG_DEBUG, "Writing <%s> to <%s>", argv[0], argv[1]);

    FILE *fp;
    const char *filename = argv[1];

    fp = fopen(filename, "w");
    if (fp == NULL) {
        syslog(LOG_ERR, "Error opening file");
        return EXIT_FAILURE;
    }

    const char *text = argv[2];
    if (fputs(text, fp) == EOF) {
        syslog(LOG_ERR, "Error writing to file");
        fclose(fp);
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "File %s created/overwritten successfully.\n", filename);

    if (fclose(fp) != 0) {
        syslog(LOG_ERR, "Error closeing file");
        return EXIT_FAILURE;
    }
    
    closelog();

    return EXIT_SUCCESS;
}

