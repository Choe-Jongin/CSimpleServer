#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BACKLOG 10
#define MAX_STRING_LEN 60
#define CHSIZE 4
//#define CHSIZE sizeof("─")

void request_hendler(void *arg);
void send_data(FILE *file, char *ct, char *filename);
char *content_type(char *file);
char *matchformat(char *str);
char *strHorLine();

int g_isServerRuning = 1;

int main(int argc, char *argv[])
{
    //listen on sock_fd, new connection on new_fd, default port number
    int sockfd, new_fd, port = 3490;

    if (argc > 1)
        port = atoi(argv[1]);
    printf("PORT : %d\n", port);
    //my address
    struct sockaddr_in serversock;

    //connector addr
    struct sockaddr_in clientsock;
    int sin_size;
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    serversock.sin_family = AF_INET;
    serversock.sin_addr.s_addr = htonl(INADDR_ANY);
    serversock.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serversock,
             sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    //main loop
    while (g_isServerRuning)
    {
        sin_size = sizeof(struct sockaddr_in);
        printf("waiting...\n");
        if ((new_fd = accept(sockfd, (struct sockaddr *)&clientsock, &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }

        char tempstr[MAX_STRING_LEN * CHSIZE] = "default";

        printf("┌%s┐\n", strHorLine());
        printf("%s\n", matchformat("                        <REQUEST>                        "));
        sprintf(tempstr, "server    : got connection from %s:%d", inet_ntoa(clientsock.sin_addr), ntohs(serversock.sin_port));
        printf("%s\n", matchformat(tempstr));
        request_hendler(&new_fd);
        printf("└%s┘ \n", strHorLine());
        close(new_fd);
    }

    //close
    close(sockfd);
    printf("close...\n");

    return 0;
}
void request_hendler(void *arg)
{
    int clnt_sock = *((int *)arg);
    char request[1024];
    char req_line[1024];
    FILE *clnt_read;
    FILE *clnt_write;

    char method[10];
    char ct[15];
    char file_name[64];
    char httpver[24];
    char tempstr[MAX_STRING_LEN * CHSIZE];

    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    fgets(req_line, 1024, clnt_read);
    if (strstr(req_line, "HTTP/") == NULL)
    {
        fclose(clnt_read);
        fclose(clnt_write);
        perror("Not HTTP");
        printf("%s\n", matchformat("Not HTTP"));
        send(clnt_sock, "ERROR!", (int)strlen("ERROR!"), 0);
        return;
    }

	strcpy(request, req_line);
    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(httpver, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));

    sprintf(tempstr, "req line  : %s", request);
    printf("%s\n", matchformat(tempstr));
    sprintf(tempstr, "HTTP ver  : %s", httpver);
    printf("%s\n", matchformat(tempstr));
    sprintf(tempstr, "Method    : %s", method);
    printf("%s\n", matchformat(tempstr));
    sprintf(tempstr, "file_name : %s", file_name);
    printf("%s\n", matchformat(tempstr));

    if (strcmp(method, "GET") != 0)
    {
        send(clnt_sock, "Not Get Method ERROR!", (int)strlen("Not Get Method ERROR!"), 0);
        fclose(clnt_read);
        fclose(clnt_write);
        perror("Not Get\n");

        return;
    }

    fclose(clnt_read);

    if (strcmp(file_name, "shutdown") == 0)
    {
        g_isServerRuning = 0;
        printf("%s\n", matchformat("shutdown server"));
        send(clnt_sock, "The server has been shut down.", (int)strlen("The server has been shut down."), 0);
    }
    else
    {    
		send_data(clnt_write, ct, file_name);
    }
}

void send_data(FILE *fp, char *ct, char *filename)
{
    printf("%s\n", matchformat(""));
    printf("%s\n", matchformat("                        <RESPONSE>                        "));

    int code = 200;
    int length = 20480;
	int type = 0;

    char protocol[100] = "HTTP/1.0 200 OK\r\n";
    char server[100] = "Serve:Linux Web Server \r\n";
    char cnt_len[100] = "Content-length:2048\r\n";
    char cnt_type[100];
    char buf[1024];
    FILE *send_file;

    char tempstr[MAX_STRING_LEN * CHSIZE];

	char strtype[20];
	strcpy(strtype,ct);
	if(strcmp("image",strtok(strtype,"/")) == 0 )
		type = 0;

    sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
    if( type == 0 )
		send_file = fopen(filename, "r");
	else
		send_file = fopen(filename, "rb");

    if (send_file == NULL)
    {
        sprintf(tempstr, "[%s] not found", filename);
        printf("%s\n", matchformat(tempstr));
        send_file = fopen("Error.html", "r");
        sprintf(cnt_type, "Content-type:%s\r\n", "text/html");
        code = 404;
    }

    sprintf(protocol, "HTTP/1.0 %d OK\r\n", code);
    sprintf(cnt_len, "Content-length:%d\r\n", length);

    sprintf(tempstr, "StateCode : %d", code);
    printf("%s\n", matchformat(tempstr));
    sprintf(tempstr, "Contents  : %s", ct);
    printf("%s\n", matchformat(tempstr));
    sprintf(tempstr, "length    : %d", length);
    printf("%s\n", matchformat(tempstr));

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs("\r\n", fp);
    while (fgets(buf, 1024, send_file) != NULL)
    {
        fputs(buf, fp);
        fflush(fp);
    }
    fflush(fp);
    fflush(fp);

	fclose(send_file);
}

//for http header
char *content_type(char *file)
{
    char *extension;
    char file_name[100];

    strcpy(file_name, file);
    strtok(file_name, ".");
    extension = strtok(NULL, ".");
    if (extension == NULL)
    {
        printf("%s\n", matchformat("wrong filename format"));
        return "text/html";
    }

    if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
        return "text/html";
    else if (!strcmp(extension, "jpg") || !strcmp(extension, "jpeg") || !strcmp(extension, "jpe"))
        return "image/jpeg";
    else if (!strcmp(extension, "png"))
        return "image/png";
    else if (!strcmp(extension, "ico"))
        return "image/x-icon";
    else
        return "text/plain";
}

// │ {str} blank │
char *matchformat(char *str)
{
    int len = strlen(str);
	for( int i = 0 ; i < len ; i++ )
		if( str[i] == '\n' || str[i] == '\r' )
			str[i] = ' ';

    char *matchedstr = (char *)malloc(MAX_STRING_LEN * CHSIZE + 1);
    memset(matchedstr, 0, MAX_STRING_LEN * CHSIZE + 1);

    strcat(matchedstr, "│");
    if (len > MAX_STRING_LEN - 2)
        return str;

    strcat(matchedstr, str);
    for (int i = 0; i < MAX_STRING_LEN - len - 2; i++)
    {
        strcat(matchedstr, " ");
    }

    strcat(matchedstr, "│");
    return matchedstr;
}

// ───────────
char *strHorLine()
{
    char *line = (char *)malloc(MAX_STRING_LEN * CHSIZE + 1);
    memset(line, 0, MAX_STRING_LEN * CHSIZE);

    for (int i = 0; i < MAX_STRING_LEN - 2; i++)
        strcat(line, "─");

    return line;
}