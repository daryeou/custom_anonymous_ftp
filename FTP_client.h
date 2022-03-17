#define CMD_LIST "ls"
#define CMD_GET "get"
#define CMD_PUT "put"
#define CMD_CD "cd"
#define CMD_RM "rm"

#define MODE_DEBUG 1
#define MODE_NORMAL 0

extern int mode;

void debug(char *msg) {
        if (mode == MODE_DEBUG) {
                printf("[debug] : %s \n", msg);
        }
}