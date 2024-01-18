#include <stdio.h>
#include <stdlib.h>

//구조체 이름 생략.
typedef struct {        
    int pid;            //ID.
    int arrival_time;   //도착시간.
    int code_bytes;     //코드길이(Byte).
} PROCESS;              //구조체 변수임을 명확히 하기위해 대문자로 표현.

int main(int argc, char *argv[]) {
    PROCESS pro = {0};              //구조체 변수 모두 0으로 초기화.
    unsigned char buf[2] = {0};     //buf[0]=Operation, buf[1]=Length.

    /*  <내가 읽어보려고 쓴 fread함수 설명>
        1. sizeof(PROCESS)는 12Byte 크기이다.
        2. sizeof(PROCESS)=12Byte 크기의 데이터 1개를 stdin으로부터 
           읽어 들여서 구조체변수 pro에 저장하라.
        3. fread함수는 실제로 읽어 들인 데이터의 갯수를 반환한다.
           (읽어들인 바이트 수가 아니라 갯수이다.) 함수의 호출이 성공을 
           하고 요청한 분량의 데이터가 모두 읽혀지면 1이 반환된다.
    */
    while(fread(&pro, sizeof(PROCESS), 1, stdin) == 1) {
        //fprintf(stdout,"%#X %#X %#X\n", pro.pid, pro.arrival_time, pro.code_bytes);     //Test Case
        fprintf(stdout,"%d %d %d\n", pro.pid, pro.arrival_time, pro.code_bytes);          //실제 출력

        for(int i=0;i<(pro.code_bytes)/(2);i++) {                                         //코드길이는 짝수이기때문에 2로 나눠도 항상 정수이다.
            fread(buf, sizeof(unsigned char), 2, stdin);                                  //1Byte단위로 2개를 읽어들어야 한다.(cause, 1개의 Tuple = 2Byte)
            //fprintf(stdout, "%#X %#X\n", buf[0], buf[1]);                               //Test Case
            fprintf(stdout, "%d %d\n", buf[0], buf[1]);                                   //실제 출력
        }

        if(feof(stdin)!=0)      //파일의 끝에 도달한 경우
            break;
        else
            continue;
    }

    return 0;
}
