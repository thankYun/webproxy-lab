/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
/* 함수 조기 선언 */
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
//* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
//* void serve_static(int fd, char *filename, int filesize);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
//* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
//* void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/**
 * @param argc 받는 인자의 개수
 * @param argv 받은 인자의 목록
 * 연결 요청이 들어오면 연결 소켓을 만들고 doit 함수 실행
*/
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;                                                                                 //clientaddr 구조체(리스트) 크기
  struct sockaddr_storage clientaddr;                                                                  //클라이언트에서 연결요청하였을 때 알 수 있는 클라이언트 연결 소켓 주소

  if (argc != 2) {                                                                                      //메인함수에 오는 인자의 개수는 2개 (./tiny) (포트번호)
    fprintf(stderr, "usage: %s <port>\n", argv[0]);                                                     //메시지 출력 후 탈출
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);                                                                    //주어진 포트 번호로 리스닝 소켓을 열어 파일 디스크립터를 리턴하고 listenfd에 저장
  while (1) {                                                                                           
    clientlen = sizeof(clientaddr);                                                                     //CLIENTADDR 구조체 크기 저장
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                                           //클라이언트에서 받은 연결 요청을 받아 connfd에 저장
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);                     //todo 0 = 도메인 1 = ip 주소
    printf("Accepted connection from (%s, %s)\n", hostname, port);                                      
    doit(connfd);                                                                                       //doit 함수 실행
    Close(connfd);                                                                                      //서버 연결 식별자 종료
  }
}

/**
 * @param fd 
 * 클라이언트의 요청 라인을 확인, 동적-정적 여부를 확인하고 각각의 서버로 보낸다.
*/
void doit(int fd){
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  
  
  /* 클라이언트가 요청 line, 요청 header 분석*/ 
  Rio_readinitb(&rio, fd);                                                                              //rio 버퍼와 fd(connfd) 연결
  Rio_readlineb(&rio, buf, MAXLINE);                                                                    //rio(현재 connfd) 의 string(Request line) buf로 옮김
  printf("Request headers: \n");      
  printf("%s",buf);
  sscanf(buf, "%s %s %s",method, uri, version);
  //* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
  //* if (strcasecmp(method, "GET")) {                                                                  //buf의 내용 GET
  if(!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {                            //buf의 내용 GET or HEAD 판단
    clienterror(fd, method, "501", "NOT implemented", "Tiny does not implement this method");           //하나라도 0이면 종료 후 main으로 이동
    return;
  }
  read_requesthdrs(&rio);                                                                               //요청 읽기

  /*GET request에서 URI 분석*/
  is_static = parse_uri (uri,filename,cgiargs);                                                         //클라이언트 요청 라인의 uri를 통해 정적(1)-동적(0) 컨텐츠 구분
  if (stat(filename, &sbuf)<0){                                                                         //filename == 클라이언트가 요구한 서버의 컨틴츠 디렉토리 및 파일 이름
     clienterror(fd, filename, "404", "Not found", "Tiny coudln't find this file");
     return;
  }

  if (is_static) {                                                                                      //정적 컨텐츠일 때, 1이면 정적 , 0이면 동적 컨텐츠
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){                                         //(일반 파일이 아니다 || 읽기 권한만 있는 파일이 아니다)>사용할 수 있는 파일인지 필터링
      clienterror(fd,filename, "403", "Forbidden", "Tiny couldn't read the program");
      return;
    }
    //* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
    //* serve_static(fd, filename,sbuf.st_size);
    serve_static(fd, filename,sbuf.st_size,method);                                                     //정적 서버에 파일의 크기 전송
  }
  else {                                                                                                //동적 컨텐츠일 때
    if (!(S_ISREG(sbuf.st_mode)) ||  !(S_IXUSR & sbuf.st_mode)){                                        //사용 가능한 컨텐츠인지 필터링
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    //* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
    //* serve_dynamic(fd, filename, cgiargs);
    serve_dynamic(fd, filename, cgiargs, method);                                                        //동적 서버에 인자 같이 보내기
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

/**
 * @param rp 클라이언트 버퍼
 * 클라이언트가 버퍼 rp에 보낸 나머지 요청 헤더들 무시하고 그냥 프린트
*/
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))                                                                            //rp가 끝까지 갈 때까지 계속 출력해 없앤다.
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/**
 * uri에 있는 요청받은 파일의 이름과 요청 인자를 채운다
 * @param uri 받은 uri
 * @param filename 요청받은 파일 이름
 * @param cgiargs 동적 컨텐츠의 실행 파일에 들어갈 인자
*/
int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;

  if (!strstr(uri, "cgi-bin")) {                                                                        //정적 컨텐츠인 경우 1 리턴
    strcpy(cgiargs, "");                                                                                //정적 컨텐츠의 경우 cgiargs가 존재하지 않는다
    strcpy(filename, ".");                                                                              //현재 디렉토리에서 시작
    strcat(filename, uri);                                                                              //uri 넣어줌
    if (uri[strlen(uri)-1] == '/')                                                                      //만약 uri의 마지막에/가 있다면 home.html 붙이기
      strcat(filename, "home.html");                                                              
    return 1;
  }
  else {                                                                                                //동적 컨텐츠인 경우 0 리턴
    ptr =index(uri, '?');                                                                               //
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
  

//* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
//* void serve_static(int fd, char *filename, int filesize){
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
  //* 문제 11.11: Tiny를 확장해서 HTTP HEAD 메소드를 지원하도록 하라. TELENT를 웹 클라이언트로 사용해 작업 결과 체크
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

//* 연습문제 11.7 (tiny를 확장해서 MPG  비디오 파일을 처리하도록 하시오. 실제 브라우저를 사용해 여러분의 결과를 체크하시오)
  else if(strstr(filename, ".mpg"))
    strcpy(filetype, "vidio/mpg");
  else if(strstr(filename, ".mp4"))
    strcpy(filetype, "vidio/mp4");
//* 

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