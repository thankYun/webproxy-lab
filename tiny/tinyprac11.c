//내거
/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
// 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
// void serve_static(int fd, char *filename, int filesize);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
// 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
// void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd){
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  
  
  /* 요청 line, 요청 header 읽기*/
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers: \n");
  printf("%s",buf);
  sscanf(buf, "%s %s %s",method, uri, version);
  // 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
  // if (strcasecmp(method, "GET")) {
  if(!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {
    clienterror(fd, method, "501", "NOT implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /*GET request에서 URI 분석*/
  is_static = parse_uri (uri,filename,cgiargs);
  if (stat(filename, &sbuf)<0){
     clienterror(fd, filename, "404", "Not found", "Tiny coudln't find this file");
     return;
  }

  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd,filename, "403", "Forbidden", "Tiny couldn't read the program");
      return;
    }
    // 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
    serve_static(fd, filename,sbuf.st_size);
    serve_static(fd, filename,sbuf.st_size,method);
  }
  else {
    if (!(S_ISREG(sbuf.st_mode)) ||  !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    // 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
    // serve_dynamic(fd, filename, cgiargs);
    serve_dynamic(fd, filename, cgiargs, method);
    }
  }
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE] , body[MAXBUF];
  /*HTTP 응답 몸체*/
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = ""ffffff"">)\r\n",body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* HTTP 응답 출력*/
  sprintf(buf, "HTTP/l.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(body, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;

  if (!strstr(uri, "cgi-bin")) { /*static content*/ 
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else { /*dynamic content*/
    ptr =index(uri, '?');
    if (ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
      strcpy(filename, ".");
      strcat(filename, uri);
    return 0;
  }
}
  

// 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
// void serve_static(int fd, char *filename, int filesize){
void serve_static(int fd, char *filename, int filesize, char *method){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /*응답 헤더를 클라이언트로 보내기*/
  get_filetype(filename ,filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer:Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %d\r\n\r\n",buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Respinse headers:\n");
  printf("%s",buf);
  // 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
  if(strcasecmp(method, "HEAD")==0)
    return;
  // /*응답의 바디를 클라이언트로 보내기*/
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}
  

/**
 * get_filetype - 파일 이름에서 파일 유형 파생
*/
void get_filetype( char *filename, char *filetype){
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpg");

// 연습문제 11.7 (tiny를 확장해서 MPG  비디오 파일을 처리하도록 하시오. 실제 브라우저를 사용해 여러분의 결과를 체크하시오)
  else if(strstr(filename, ".mpg"))
    strcpy(filetype, "vidio/mpg");
  else if(strstr(filename, ".mp4"))
    strcpy(filetype, "vidio/mp4");
// 

  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method){
  char buf[MAXLINE], *emptylist[] = { NULL };

  /*HTTP의 응답의 첫번째 부분 반환*/
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /*child*/
  /*실제 서버는 여기에 모든 CGI 변수를 설정함*/
    setenv("QUERY_STRING", cgiargs, 1);
    // 11.11
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);    //표준 출력을 클라이언트로 리디렉션 - 리디렉션은 사용자와 검색 엔진을 하나의 연결된 앵커에서 다른 URL로 보내는 방법입니다.
    Execve(filename, emptylist, environ); /*Run CGI program*/
  }
  Wait(NULL); /*parent 는 chile를 기다려 거둠 */
}