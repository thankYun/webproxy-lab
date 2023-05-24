#include "csapp.h"
/**
 * @param argc 받는 인자의 개수
 * @param argv 받은 인자 리스트
*/
int main(int argc, char **argv){
    
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc != 3){                                                          //받은 인자의 개수가 목표치(3개)와 다를 때 사용법 출력 함께 종료
        fprintf(stderr, "usage: %s <host> <port>\n, argv[0]");
        exit(0);
    }
    host = argv[1];                                                         //목표치와 같은 경우 host에 첫번째 인자 저장
    port = argv[2];                                                         //목표치와 같은 경우 host에 두번째 인자 저장

    clientfd = Open_clientfd(host,port);                                    //도움함수인 오픈클라이언트 함수을 통해 서버와 연결, 리턴받은 서버 식별자를 clientfd에 저장한다
    Rio_readinitb(&rio, clientfd);                                          //rio구조체를 초기화하고 rio를 통해 파일 디스크립터 clientfd에 대한 읽기 작업을 수행할 수 있도록 설정

    while (Fgets(buf, MAXLINE, stdin)!= NULL) {                             //반복하여 유저에게서 받은 입력을 buf에 저장, 오류 시 중지
        Rio_writen(clientfd, buf, strlen(buf));                             //파일 디스크립터를 통해 buf에 저장된 데이터 서버로 전송
        Rio_readlineb(&rio, buf, MAXLINE);                                  //rio 구조체를 통해 파일 디스크립터에서 한 줄 문자열 읽어와 buf에 저장, MAXLINE은 버퍼의 최대 크기
        Fputs(buf, stdout);                                                 //buf에 저장된 문자열 표준 출력 stdout에 출력
    }
                                                                            //todo Rio readinitb, Rio_writen Fput 함수 공부 후 주석 추가 필요
    Close(clientfd);                                                        //교재에는 불필요하지만 습관성 명시 목적의 코드라고 적혀 있다.
    exit(0);
}
