#include "csapp.h"
/**
 * 텍스트 줄을 읽고 다시 돌려주는 함수
 * @param connfd 서버 연결 식별자
*/
void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d byte\n", (int)n);
        Rio_writen(connfd,buf,n);
    }
}

//todo RIO함수 공부 후 추가 기술