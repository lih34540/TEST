/**
 * \author {AUTHOR}
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "sensor_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

FILE *open_db(char *filename, bool append)
{
    if (filename == NULL) return NULL;

    const char *mode = append ? "a" : "w";
    FILE *fp = fopen(filename, mode);

    if (fp == NULL) {
        return NULL;
    }

    return fp;
}

int insert_sensor(FILE *f,
                  sensor_id_t id,
                  sensor_value_t value,
                  sensor_ts_t ts)
{
    if (f == NULL) return -1;

    int ret = fprintf(f, "%u,%lf,%ld\n",
                      id,
                      value,
                      (long)ts);

    fflush(f);

    if (ret < 0) {
        return -1;
    }
    return 0;
}

int close_db(FILE *f)
{
    if (f == NULL) return -1;

    fclose(f);
    return 0;
}

static int log_pipe[2] = { -1, -1 };
static pid_t log_child = -1;

/**
 * Write one formatted log line to the log file.
 * Format:
 *   <seq> <epoch_time> <message>
 */
static void write_log_line(FILE *fp, int seq, const char *msg)
{
    time_t now = time(NULL);
    fprintf(fp, "%d %ld %s", seq, (long)now, msg);
    fflush(fp);
}

int create_log_process(void)
{
    if (pipe(log_pipe) < 0) {
        return -1;
    }

    log_child = fork();
    if (log_child < 0) {
        return -1;
    }

    /* Child process: logger */
    if (log_child == 0) {

        close(log_pipe[1]);   // close write end

        FILE *rf = fdopen(log_pipe[0], "r");
        if (rf == NULL) {
            exit(EXIT_FAILURE);
        }

        /* Create a NEW gateway.log (overwrite old one) */
        FILE *lf = fopen("gateway.log", "w");
        if (lf == NULL) {
            exit(EXIT_FAILURE);
        }

        char line[256];
        int seq = 1;

        while (1) {
            if (fgets(line, sizeof(line), rf) == NULL) {
                break;
            }

            /* EXIT message ends logger */
            if (strcmp(line, "EXIT\n") == 0) {
                break;
            }

            write_log_line(lf, seq, line);
            seq++;
        }

        fclose(lf);
        fclose(rf);
        exit(EXIT_SUCCESS);
    }

    /* Parent process */
    close(log_pipe[0]);   // close read end
    return 0;
}

int write_to_log_process(char *msg)
{
    if (log_pipe[1] < 0 || msg == NULL) return -1;

    size_t len = strlen(msg);
    ssize_t written = write(log_pipe[1], msg, len);

    if (written != (ssize_t)len) {
        return -1;
    }
    return 0;
}

int end_log_process(void)
{
    if (log_child <= 0) return -1;

    /* send termination message */
    write_to_log_process("EXIT\n");

    close(log_pipe[1]);   // close write end
    waitpid(log_child, NULL, 0);

    log_child = -1;
    log_pipe[0] = -1;
    log_pipe[1] = -1;

    return 0;
}