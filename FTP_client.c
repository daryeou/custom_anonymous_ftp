#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "FTP_client.h"
#define BUFSIZE 2048
#define FILE_BUFSIZE 2048
#define OK 1
#define NO 0
//#define NAMESIZE 20
int mode;

void *send_message(void *arg);
void *recv_message(void *arg);
void error_handling(char *message);
//char name[NAMESIZE] = "[Default]";
void list(char *listCmd, int sock);
void get(char *getCmd, int sock);
void put(char *putCmd, int sock);
void cd(char *cdCmd, int sock);
void rm(char *rmCmd, int sock);

void commandHandle(char *cmd, char *cmd2, int sock);
char message[BUFSIZE];
char *directory = "./";

typedef struct _FtpCommandHandler
{
    char cmd[5];
    void (*handler)(char *, int);
} FtpHandler;

FtpHandler ftpHandler[] = {
    {CMD_LIST, list},
    {CMD_GET, get},
    {CMD_PUT, put},
    {CMD_CD, cd},
    {CMD_RM, rm}};

void commandHandle(char *cmd, char *cmd2, int sock)
{
    int countCmd = sizeof(ftpHandler) / sizeof(FtpHandler);
    for (int i = 0; i < countCmd; i++)
    {
        if (!strncmp(cmd, ftpHandler[i].cmd, strlen(ftpHandler[i].cmd)))
        {
            (*(ftpHandler[i].handler))(cmd2, sock);
            return; //일치하는 문자열이 있으면 함수 종료
        }
    }
    printf("%s %s\n", cmd, cmd2);
}

int main(int argc, char **argv)
{
    mode = 1;
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_result;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    //sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    pthread_create(&snd_thread, NULL, send_message, (void *)sock);
    pthread_create(&rcv_thread, NULL, recv_message, (void *)sock);
    pthread_join(snd_thread, &thread_result);
    pthread_join(rcv_thread, &thread_result);
    close(sock);
    return 0;
}

void *send_message(void *arg) /* 메시지 전송 쓰레드 실행 함수 */
{
    int sock = (int)arg;
    char message[BUFSIZE];
    while (1)
    {
        fgets(message, BUFSIZE, stdin);
        message[strlen(message) - 1] = '\0';
        write(sock, message, strlen(message));
        if (!strcmp(message, "quit"))
        {
            /* 'quit'입력시 종료 */
            close(sock);
            exit(0);
        }
        memset(message, 0, sizeof(message)); //메시지 송신 후 초기화
    }
}

void *recv_message(void *arg) /*메시지 수신 쓰레드 실행 함수*/
{
    int sock = (int)arg;
    char message[BUFSIZE];
    int str_len;
    while (1)
    {
        //str_len = read(sock, message, BUFSIZE - 1);
        while ((str_len = read(sock, message, sizeof(message))) != 0)
        { //클라이언트로부터 명령어 받아오기
            //message[str_len] = 0;
            char *command = strtok(message, " ");
          if(command != NULL){
            commandHandle(command, strtok(NULL, "/0"), sock);
            memset(message, 0, sizeof(message)); //메시지 수신 후 초기화
            printf("\n-----------\n");
            printf("[FTP]: ");
            fflush(stdout);
          }
        }
        if (str_len <= 0)
            break;
    }
    printf("서버가 종료되었습니다.\n");
    close(sock);
    exit(1);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void list(char *listCmd, int sock)
{
    printf("디렉토리 정보:\n %s\n", listCmd);
}
void get(char *getCmd, int sock)
{
    printf("전송받을 파일: %s\n", getCmd);

    int is_use = NO;
    read(sock, &is_use, sizeof(int));
    if (is_use == NO)
    {
        printf("서버에서 파일을 사용중입니다.\n");
        return;
    }

    char filename[100];
    memset(filename, 0, sizeof(filename));
    sprintf(filename, "%s%s", directory, getCmd);
    /*파일 사이즈 확인*/
    long size;
    read(sock, &size, sizeof(long));
    if (!size)
    {
        printf("is not file!\n");
        return;
    }
    else
    {
        printf("파일 크기 %ldByte\n", size);
    }
    size = (long)ceil((double)size / (double)FILE_BUFSIZE);
    if (size == 0)
    {
        size += 1;
    }
    // 파일 생성
    int dest_fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);

    if (!dest_fd)
    {
        perror("Error");
        return;
    }

    if (errno == EEXIST)
    {
        perror("같은 파일명이 이미 존재합니다.\n");
        int i = NO;
        write(sock, &i, sizeof(int));
        close(dest_fd);
        return;
    }
    else
    {
        int i = OK;
        write(sock, &i, sizeof(int));
    }

    int totaln = 0;
    char buffer[FILE_BUFSIZE];
    for (long i = 0; i < size; i++)
    {
        memset(buffer, 0x00, FILE_BUFSIZE);
        int recv_len = read(sock, buffer, FILE_BUFSIZE);
        if (recv_len == 0)
        {
            printf("finish file\n");
            break;
        }
        totaln += write(dest_fd, buffer, recv_len);
        printf("\r진행률 %.0lf%%", (((double)i + 1) / ((double)size)) * 100);
    }
    printf("\n파일 생성 완료\n");
    close(dest_fd);
}

void put(char *putCmd, int sock)
{
    printf("전송할 파일: %s\n", putCmd);

    int is_use = NO;
    read(sock, &is_use, sizeof(int));
    if (is_use == NO)
    {
        printf("서버에서 파일을 사용중입니다.\n");
        return;
    }
    else
    {
    }

    struct stat obj;
    long size;
    int source_fd;
    /* 파일 열기 */
    source_fd = open(putCmd, O_RDONLY);
    if (source_fd == -1)
    {
        size = 0;
        write(sock, &size, sizeof(long));
        perror("file open error \n");
        return;
    }
    else
    {
        /*파일 크기 전송
    --실제ftp의 경우 소켓을 새로 생성했다가 제거--*/
        stat(putCmd, &obj);
        size = obj.st_size;
        printf("파일 크기 : %ldByte\n", size);
        write(sock, &size, sizeof(long));
    }
    size = (long)ceil((double)size / (double)FILE_BUFSIZE);
    if (size == 0)
    {
        size += 1;
    }

    int i = NO;
    read(sock, &i, sizeof(int));
    if (i == 0)
    {
        printf("서버에 이미 같은 파일이름이 있습니다. 전송종료\n");
        return;
    }

    long count = 0;
    while (1)
    {
        char buf[FILE_BUFSIZE];
        memset(buf, 0, FILE_BUFSIZE);
        int file_read_len = read(source_fd, buf, FILE_BUFSIZE); //FILE_BUFSIZE read
        if (file_read_len == EOF | file_read_len == 0 | buf == NULL)
        {
            break;
        }
        else
        {
            printf("\r진행률 %.0lf%% 전달한 횟수: %ld", (((double)count + 1) / ((double)size)) * 100, count+1);
            int chk_write = write(sock, buf, FILE_BUFSIZE);
        }
        count++;
    }
    //sendfile(sock,source_fd,NULL,BUFSIZE);
    close(source_fd);
}

void cd(char *cdCmd, int sock)
{
    printf("%s\n", cdCmd);
}

void rm(char *rmCmd, int sock)
{
    int result = NO;
    read(sock, &result, sizeof(int));
    if (result == OK)
    {
        printf("%s removed\n", rmCmd);
    }
    else
    {
        printf("%s is not removed\n", rmCmd);
    }
}
