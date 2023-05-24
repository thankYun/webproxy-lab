#include "csapp.h"
#include "echo.c"
/**
 * @param argc 받는 인자의 개수
 * @param argv 받은 인자 리스트
*/
int main(int argc, char **argv){                                                        

    int listenfd, connfd;                                                                               //리스닝 소켓과 연결된 클라이언트 소켓의 파일 디스크립터 선언
    socklen_t clientlen;                                                                                //클라이언트 주소 길이 저장할 clientlen 선언
    struct sockaddr_storage clientaddr;                                                                 //클라이언트 주소 정보 저장하는 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];                                                //클라이언트의 호스트 이름과 포트 번호 저장 배열

    if (argc != 2) {                                                                                    //프로그램 실행 시 인자의 개수가 두개가 아닌 경우
        fprintf(stderr, "usage: %s <port>\n", argv[0]);                                                 //사용법 출력 후 종료
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);                                                                  //주어진 포트 번호로 리스닝 소켓을 열어 파일 디스크립터를 리턴하고 listenfd에 저장
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);                                                    //주소 길이 초기화
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                                       //클라이언트의 연결을 받고, 연결된 클라이언트 소켓의 파일 디스크립터를 얻는다
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);  //클라이언트 호스트 이름과 포트 번호를 얻는다
        printf ( "Connected to (%s, %s)\n", client_hostname, client_port);                              //연결된 클라이언트 정보 출력
        echo(connfd);                                                                                   //클라이언트와 통신하는 echo 함수 호출
        Close(connfd);                                                                                  //클라이언트 소켓 닫기
    } 
    exit(0);
}