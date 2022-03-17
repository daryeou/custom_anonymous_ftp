#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <dirent.h> //디렉토리 관련 헤더파일
#include <sys/timeb.h>
#include <time.h>   //시간 관련 헤더파일
#include <stdarg.h> // va_list, va_start, va_arg, va_end가 정의된 헤더 파일
#include "FTP_server.h"
#define BUFSIZE 2048
#define FILE_BUFSIZE 2048
#define OK 1
#define NO 0
#define CHECK 0
#define ADD 1
#define DEL 2
int mode;
/*#define STR_SWITCH_BEGIN(key) \
{ \
　　static stdext::hash_map< string, int > s_hm; \
　　static bool s_bInit = false; \
　　bool bLoop = true; \
　　while(bLoop) \
　　{ \
　　　　int nKey = -1; \
　　　　if(s_bInit) { nKey=s_hm[key]; bLoop = false; } \
　　　　switch(nKey) \
　　　　{ \
　　　　　　case -1: {

#define CASE(token)  } case __LINE__: if(!s_bInit) s_hm[token] = __LINE__; else {
#define DEFAULT()    } case 0: default: if(s_bInit) {

#define STR_SWITCH_END()   \
　　　　　　}      \
　　　　}       \
　　　　if(!s_bInit) s_bInit=true; \
　　}        \
}*/

void *clnt_connection(void *arg[2]);
void send_message(char *message, int len);
void error_handling(char *message);
void logsave(char *logstr);
void logTime(char *ip, char *logstr);
void sendMsg(int sock, int str_len, int count, ...);
void list(char *listCmd, int sock, char *directory);
void get(char *getCmd, int sock, char *directory);
void put(char *putCmd, int sock, char *directory);
void cd(char *cdCmd, int sock, char *directory);
void rm(char *rmCmd, int sock, char *directory);
void quit(char *quitCmd, int sock, char *directory);
void commandHandle(char *cmd, char *cmd2, int sock, char *directory);
int clnt_number = 0;
int clnt_socks[10]; //동시접속제한 10대

pthread_mutex_t mutx, mutx_list;
FILE *fp; //전송할 파일
char *download_list[10] = {
    0,
}; //다운로드 중인 리스트 mutex로 잠글것
char *upload_list[10] = {
    0,
}; //업로드 중인 리스트 mutex로 잠글것
int download_number = 0;
int upload_number = 0;

typedef struct _FtpCommandHandler
{
    char cmd[5];
    void (*handler)(char *, int, char *);
} FtpHandler;

FtpHandler ftpHandler[] = {
    {CMD_LIST, list},
    {CMD_GET, get},
    {CMD_PUT, put},
    {CMD_CD, cd},
    {CMD_QUIT, quit},
    {CMD_RM, rm}};

/*qsort를 위한 문자열 비교함수*/
int compare(const void *A, const void *B)
{
    return strcmp((*(struct dirent **)A)->d_name, (*(struct dirent **)B)->d_name);
}

int main(int argc, char **argv)
{
    mode = MODE_DEBUG; //디버그 모드
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;
    pthread_t thread; //스레드

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    if (pthread_mutex_init(&mutx, NULL))
        error_handling("mutex init error"); //뮤텍스 초기화 실패시 에러
    if (pthread_mutex_init(&mutx_list, NULL))
        error_handling("mutex init error"); //뮤텍스 초기화 실패시 에러

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");
    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_number++] = clnt_sock;
        pthread_mutex_unlock(&mutx);
        /*새로운 클라이언트 로그*/
        char logmsg[BUFSIZE];
        sprintf(logmsg, "%d socket create", clnt_sock);
        logTime(inet_ntoa(clnt_addr.sin_addr), logmsg);
        printf("%s\n", logmsg);

        /*스레드 생성*/
        void *args[2] = {clnt_sock, (struct sockaddr *)&clnt_addr}; //소켓번호와 IP주소를 같이 넘김
        pthread_create(&thread, NULL, clnt_connection, args);
    }
    return 0;
}

/*Send Directory information
params: Folder name, socket, root Directory*/
void list(char *listCmd, int sock, char *directory)
{
    /*현재 디렉토리 출력*/
    DIR *dir = opendir(directory);
    struct dirent *ent[1000] = {
        0,
    }; //NULL로 초기화
    if (dir != NULL)
    {
        int count = 0; //파일 또는 디렉토리 개수 count
        for (count = 0; (ent[count] = readdir(dir)) != NULL; count++)
        { //ent배열에 디렉토리 삽입
            //printf ("%s\n", (*(ent[count])).d_name);
            printf("\rtotal files: %4d", count);
        }
        /*디렉토리가 담긴 ent배열을 파일명으로 퀵정렬*/
        qsort(ent, count, sizeof(*ent), compare); //또는 scandir(".",&dirent,0,alphasort);

        /*클라이언트로 디렉토리 정보 전송*/
        char msg[BUFSIZE];           //클라이언트로 보낼 디렉토리 정보
        memset(msg, 0, sizeof(msg)); //메시지를 NULL로 초기화
        for (int i = 0; i < count; i++)
        {
            //printf ("%s\n", ent[i]->d_name); //정렬한 ent배열 출력
            sprintf(msg, "%s\n%s", msg, ent[i]->d_name);
        }
        sendMsg(sock, BUFSIZE, 2, "ls", msg);
        //closedir (dir);
    }
    else
    {
        //error_handling("can't load Directory"); //디렉토리 로드 실패시 에러
        sendMsg(sock, BUFSIZE, 2, "ls", "Wrong Directory");
    }
}

/*File Sending to Client
params: File name, socket, root Directory*/
void get(char *getCmd, int sock, char *directory)
{
    printf("get: %s\n", getCmd);
    struct stat obj;
    long size;
    int source_fd;

    sendMsg(sock, BUFSIZE, 2, "get", getCmd); //실행

    /* 디렉토리+파일 */
    char filename[100];
    memset(filename, 0, sizeof(filename));
    sprintf(filename, "%s%s", directory, getCmd);
    /*이미 사용중인 파일인지 검사*/
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    if (check_upload(filename, CHECK))
    {
        int i = NO;
        write(sock, &i, sizeof(int));
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    else
    {
        check_download(filename, ADD); //파일을 사용가능하면 리스트에 삽입.
        int i = OK;
        write(sock, &i, sizeof(int));
    }

    source_fd = open(filename, O_RDONLY);
    if (source_fd == -1)
    {
        size =0;
        write(sock, &size, sizeof(long));
        perror("file open error \n");
        check_download(filename, DEL);
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    else
    { //실제 존재하는 파일일 경우.
        /*파일 크기 전송
    --실제ftp의 경우 소켓을 새로 생성했다가 제거--*/
        stat(filename, &obj);
        size = obj.st_size;
        printf("file size : %ldByte\n", obj.st_size);
        write(sock, &size, sizeof(long));
    }

    size = (long)ceil((double)size / (double)FILE_BUFSIZE);
    if (size == 0)
    {
        size += 1;
    }

    /*클라이언트에서 오류가 발생하면 취소*/
    int i = NO;
    read(sock, &i, sizeof(int));
    printf("code : %d\n", i);
    if (i == 0)
    {
        printf("client has same file. file transfer end.\n");
        check_download(filename, DEL);
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    pthread_mutex_unlock(&mutx_list); //mutex 잠금해제
    
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
            printf("\rsending size:%d", file_read_len); //읽은 버퍼만큼 출력
            int chk_write = write(sock, buf, FILE_BUFSIZE);
        }
    }
    //sendfile(sock,source_fd,NULL,BUFSIZE);
    close(source_fd);
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    check_download(filename, DEL);
    pthread_mutex_unlock(&mutx_list); //mutex 잠금해제
    printf("\nfile transfer finish\n");
}

/*Receive File
params: File name, socket, root Directory*/
void put(char *putCmd, int sock, char *directory)
{
    sendMsg(sock, BUFSIZE, 2, "put", putCmd);
    printf("Receiving file: %s\n", putCmd);

    // 파일 생성
    char filename[100];
    memset(filename, 0, sizeof(filename));
    sprintf(filename, "%s%s", directory, putCmd);
    /*이미 사용중인 파일인지 검사*/
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    if (check_download(filename, CHECK) || check_upload(filename, CHECK))
    {
        int i = NO;
        write(sock, &i, sizeof(int));
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    else
    {
        check_upload(filename, ADD);
        int i = OK;
        write(sock, &i, sizeof(int));
    }

    long size = 0;
    read(sock, &size, sizeof(long));
    if (size <= 0)
    {
        printf("is not file!\n");
        check_upload(filename, DEL);
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    else
    {
        size = (long)ceil((double)size / (double)FILE_BUFSIZE);
        if (size == 0)
        {
            size += 1;
        }
    }
    printf("총 전달할 횟수:%ld\n",size);

    int dest_fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if (!dest_fd)
    {
        check_upload(filename, DEL);
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        perror("Error ");
        return;
    }

    if (errno == EEXIST)
    {
        perror("같은 파일명이 이미 존재합니다.");
        int i = NO;
        write(sock, &i, sizeof(int));
        check_upload(filename, DEL);
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        close(dest_fd);
        return;
    }
    else
    {
        int i = OK;
        write(sock, &i, sizeof(int));
    }
    pthread_mutex_unlock(&mutx_list); //mutex 잠금해제

    int totaln = 0;
    char buffer[FILE_BUFSIZE];
    for (long i = 0; i < size; i++)
    {
        memset(buffer, 0, FILE_BUFSIZE);
        int recv_len = read(sock, buffer, FILE_BUFSIZE);
        if (recv_len == 0)
        {
            printf("finish file\n");
            break;
        }
        totaln += write(dest_fd, buffer, recv_len);
        printf("\rReceive: %.0lf%% 전달받은 횟수 %ld", (((double)i+1) / ((double)size)) * 100,i+1);
    }
    printf("\nReceive finish\n");
    close(dest_fd);
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    check_upload(filename, DEL);
    pthread_mutex_unlock(&mutx_list); //mutex 잠금해제
    printf("Download finish\n");
}

/*Set Root Directory
params: Folder name OR Root command, socket, root Directory*/
void cd(char *cdCmd, int sock, char *directory)
{
    sendMsg(sock, BUFSIZE, 2, "cd root: ", cdCmd);
    directory[strlen(directory) - 1] = NULL;

    if (strcmp(cdCmd, ".") == 0)
    {
        strcpy(directory, "./");
        return;
    }
    else if (strcmp(cdCmd, "..") == 0)
    {
        int i = 0;
        char *temp[100] = {
            0,
        };
        char tmp[100] = {
            0,
        };
        tmp[0] = '.';
        temp[i] = strtok(directory, "/");
        while (temp[i] != NULL)
        {
            i++;
            temp[i] = strtok(NULL, "/");
        }
        for (int k = 1; k < (i - 1); k++)
        {
            sprintf(tmp, "%s/%s", tmp, temp[k]);
        }
        sprintf(tmp, "%s/", tmp);
        strcpy(directory, tmp);
        return;
    }

    cdCmd = strtok(cdCmd, "/");
    if (cdCmd[0] == '/')
    {
        cdCmd = strtok(NULL, "/");
    }
    while (cdCmd != NULL)
    {
        sprintf(directory, "%s/%s", directory, cdCmd);
        cdCmd = strtok(NULL, "/");
    }
    sprintf(directory, "%s/", directory);
}

/*Leave Client Alert
params: NULL, socket, root Directory*/
void quit(char *quitCmd, int sock, char *directory)
{
    printf("%d socket close success\n", sock);
}

/*Remove File
params: File name, socket, root Directory*/
void rm(char *rmCmd, int sock, char *directory)
{
    sendMsg(sock, BUFSIZE, 2, "rm", rmCmd); //실행

    /* 디렉토리+파일 */
    char filename[100];
    memset(filename, 0, sizeof(filename));
    sprintf(filename, "%s%s", directory, rmCmd);

    /*다운로드 업로드 사용중인 파일 리스트에 추가*/
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    if (check_download(filename, CHECK) || check_upload(filename, CHECK))
    {
        int i = NO;
        printf("%s is use, not removed\n", rmCmd);
        write(sock, &i, sizeof(int));
        pthread_mutex_unlock(&mutx_list); //mutex잠금 해제
        return;
    }
    check_download(filename, ADD);
    check_upload(filename, ADD);
    pthread_mutex_unlock(&mutx_list); //mutex 잠금해제
    
    /*삭제*/
    int result = remove(filename);

    /*다운로드 업로드 리스트에서 제거*/
    pthread_mutex_lock(&mutx_list); //mutex 잠금
    check_download(filename, DEL);
    check_upload(filename, DEL);
    pthread_mutex_unlock(&mutx_list); //mutex 잠금

    if (result == 0)
    {
        printf("%s removed\n", rmCmd);
        int i = OK;
        write(sock, &i, sizeof(int));
    }
    else
    {
        printf("%s is not removed\n", rmCmd);
        int i = NO;
        write(sock, &i, sizeof(int));
    }
}

/*Command Handler
params: first command arg, second command arg, socket num ,root Directory*/
void commandHandle(char *cmd, char *cmd2, int sock, char *directory)
{
    printf("Socket Num: %d, Command: %s %s\n", sock, cmd, cmd2);
    int countCmd = sizeof(ftpHandler) / sizeof(FtpHandler);
    for (int i = 0; i < countCmd; i++)
    {
        if (!strncmp(cmd, ftpHandler[i].cmd, strlen(ftpHandler[i].cmd)))
        {
            (*(ftpHandler[i].handler))(cmd2, sock, directory);
            return;
        }
    }
    sendMsg(sock, BUFSIZE - 1, 2, "Wrong_command:", cmd);
}

/*Client Connection
Run in Thread
params: [ clnt_sock, (struct sockaddr_in *)]*/
void *clnt_connection(void *arg[2])
{
    char directory[100] = "./";
    char *ClientIp = inet_ntoa(((struct sockaddr_in *)arg[1])->sin_addr);
    int clnt_sock = (int)arg[0];
    int str_len = 0;
    char message[BUFSIZE];
    sendMsg(clnt_sock, BUFSIZE - 1, 3, "Welcome_to_FTP_Server\n", " ", "---Command---\nls\nget\nput\ncd\nquit");
    while ((str_len = read(clnt_sock, message, sizeof(message))) != 0)
    {                               //클라이언트로부터 명령어 받아오기
      
        logTime(ClientIp, message); //전달받은 메시지 기록
        //sendMsg(clnt_sock,str_len,1,message);
        char *command = strtok(message, " ");
      if(command != NULL){
        commandHandle(command, strtok(NULL, "/0"), clnt_sock, directory);
        memset(message, 0, sizeof(message)); //메시지 전송 후 초기화
      }
    }
    pthread_mutex_lock(&mutx); //mutex 잠금
    for (int i = 0; i < clnt_number; i++)
    {
        /* 클라이언트 연결 종료 시 */
        if (clnt_sock == clnt_socks[i])
        {
            for (; i < clnt_number - 1; i++)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_number--;
    pthread_mutex_unlock(&mutx); //mutex 잠금해제
    printf("%d close socket\n", clnt_sock);
    logTime(ClientIp, "close socket"); //전달받은 메시지 기록
    close(clnt_sock);                  //연결이 끊어지면 소켓 종료
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
/*Save Log 
params: char *message */
void logsave(char *logstr)
{
    //file open & date
    FILE *fp = fopen("server.log", "a+");
    fprintf(fp, "%s\n", logstr);
    fclose(fp);
}
//time log
void logTime(char *ip, char *logstr)
{
    char logmsg[BUFSIZE];
    time_t current_time;
    struct timeb timebuffer;
    struct tm *date1;
    ftime(&timebuffer);
    current_time = timebuffer.time;
    date1 = localtime(&current_time);
    sprintf(logmsg, "[%s] %d년 %d월 %d일 %d시 %d분: %s ", ip, date1->tm_year + 1900, date1->tm_mon + 1, date1->tm_mday, date1->tm_hour, date1->tm_min, logstr);
    logsave(logmsg);
}
//전송 함수
//TODO 디렉토리정보가 arg인자에 BUFSIZE인 2048바이트까지만 전송됨. 수정바람
void sendMsg(int sock, int str_len, int count, ...)
{
    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    va_list ap;
    va_start(ap, count);
    char *arg = va_arg(ap, char *);
    for (int i = 0; i < count && arg != NULL; i++)
    {
        sprintf(msg, "%s %s", msg, arg); //띄어쓰기로 strtok하므로 반드시 띄어쓰기 하나는 포함할것.
        arg = va_arg(ap, char *);
    }
    printf("-----Socket : %d  message send-----\n%s", sock, msg);
    write(sock, msg, str_len);
    printf("\n-----Socket : %d  send finish-----\n", sock);
}
/*Avoid duplication*/
int check_download(char *filename, int mode)
{
    switch (mode)
    {
    case CHECK:
        for (int i = 0; i < download_number; i++)
        {
            if (strcmp(filename, download_list[i]) == 0)
            {
                return 1;
            }
        }
        break;
    case ADD:
        download_list[download_number] = filename;
        download_number++;
        break;
    case DEL:
        for (int i = 0; i < download_number; i++)
        {
            if (strcmp(filename, download_list[i]) == 0)
            {
                download_list[i] = download_list[i + 1];
            }
        }
        download_number--;
    }
    return 0;
}
/*Avoid duplication*/
int check_upload(char *filename, int mode)
{
    switch (mode)
    {
    case CHECK:
        for (int i = 0; i < upload_number; i++)
        {
            if (strcmp(filename, upload_list[i]) == 0)
            {
                pthread_mutex_unlock(&mutx_list); //mutex 잠금해제
                return 1;
            }
        }
        break;
    case ADD:
        upload_list[upload_number] = filename;
        upload_number++;
        break;
    case DEL:
        for (int i = 0; i < upload_number; i++)
        {
            if (strcmp(filename, upload_list[i]) == 0)
            {
                upload_list[i] = upload_list[i + 1];
            }
        }
        upload_number--;
    }
    return 0;
}