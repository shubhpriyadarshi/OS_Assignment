#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
 
int n1, n2, n3;
int fd[4][2];
int c1=-1, c2=-1, c3=-1; /** Dash of child proccesses **/
pid_t p1=-1, p2=-1, p3=-1; /** pid's of child proccesses **/
int time_q = -1;
pthread_t t1, t2, t3, m1, m2, m3, s1, s2, s3;

#define READ_END 0
#define WRITE_END 1

#define BILLION 1000000000.0

/** Monitor functions **/
void *RR_p_fun1(void *);
void *RR_p_fun2(void *);
void *RR_p_fun3(void *);

/** Monitor functions **/
// void *c1_monitor(void *);
// void *c2_monitor(void *);
// void *c3_monitor(void *);

/** Work Functions **/
void *c1_work(void *);
void *c2_work(void *);
void *c3_work(void *);

pid_t quantum(pid_t, int);

void close_pipes(int num);
void pipe_write(int num, void * address, int size);
void pipe_read(int num, void * address, int size);
 
int main(int argc, char *argv[])
{
    char scheduling_algo[30];
    int wpid, status;
    scanf("%d %d %d", &n1, &n2, &n3);
    scanf("%s", scheduling_algo);

    for (int i = 1; i <= 3; ++i) /* Creating Pipes **/
        if (pipe(fd[i]) == -1) {
            printf("Error while creating pipes\n");
            return 1;
        }

    if (strcmp(scheduling_algo, "FCFS") == 0) {
        ((c1 = fork()) && (c2 = fork()) && (c3 = fork())); /* Creating Processess **/

        /* Child 1 **/
        if (c1 == 0) {
            printf("In child1, id is %d\n", getpid());
            // pthread_create(&t1, NULL, c1_monitor, NULL); /* Thread Creation **/
            pthread_join(t1, NULL);
        }
        /* Child 2 **/
        else if (c2 == 0) {
            printf("In child2, id is %d\n", getpid());
            // pthread_create(&t2, NULL, c2_monitor, NULL); /* Thread Creation **/
            pthread_join(t2, NULL);
        }
        /* Child 3 **/
        else if (c3 == 0) {
            printf("In child3, id is %d\n", getpid());
            // pthread_create(&t3, NULL, c3_monitor, NULL); /* Thread Creation **/
            pthread_join(t3, NULL);
        }
        /* Parent **/
        else {
            while ((wpid = wait(&status)) > 0)
                ;
            printf("In main, id is %d\n", getpid());
            
            long long sum_from_c1 = -1;

            pipe_read(1, &sum_from_c1, sizeof(long long));
            close(fd[1][READ_END]);
            printf("Result from c1 is %lld\n", sum_from_c1);

            char message_from_c2[30];
            int sz;

            pipe_read(2, &sz, sizeof(int));
            pipe_read(2, message_from_c2, sizeof(char) * sz + 1);
            close(fd[2][READ_END]);

            printf("Message recieved from C2 : %s\n", message_from_c2);

            long long sum_from_c3 = -1;

            pipe_read(3, &sum_from_c3, sizeof(long long));
            close(fd[3][READ_END]);

            printf("Sum from c3 is : %lld\n", sum_from_c3);
            close_pipes(0);
        }
    }
    else if ( strcmp(scheduling_algo, "RR") == 0 ) {
        scanf("%d", &time_q);

        pthread_create(&s1, NULL, RR_p_fun1, NULL); 
        pthread_create(&s2, NULL, RR_p_fun2, NULL);
        pthread_create(&s3, NULL, RR_p_fun3, NULL);

        //printf("In child3, id is %d\n", getpid());

        int pid_loop = 0;
        int shmid;
        char *shmPtr; 
        while(1){

        if( (shmid = shmget(2041, 32, 0666 | IPC_CREAT)) == -1 )
            exit(1);
        shmPtr = shmat(shmid, 0, 0);
        if (shmPtr == (char *) -1)
            exit(2);
        shmPtr[0] = '0' + (pid_loop+1)%3;
        sleep(time_q);
        }

        printf("In main, id is %d\n", getpid());
        long long sum_from_c1 = -1;
        pipe_read(1, &sum_from_c1, sizeof(long long));
        close(fd[1][READ_END]);
        printf("Result from c1 is %lld\n", sum_from_c1);
            char message_from_c2[30];
        int sz;
        pipe_read(2, &sz, sizeof(int));
        pipe_read(2, message_from_c2, sizeof(char) * sz + 1);
        close(fd[2][READ_END]);
        printf("Message recieved from C2 : %s\n", message_from_c2);
        long long sum_from_c3 = -1;
        pipe_read(3, &sum_from_c3, sizeof(long long));
        close(fd[3][READ_END]);
        printf("Sum from c3 is : %lld\n", sum_from_c3);
        if( shmctl(shmid, IPC_RMID, NULL) == -1 ){
            perror("shmctl");
            exit(-1);
        }

        if( shmctl(shmid, IPC_RMID, NULL) == -1 ){
            perror("shmctl");
            exit(-1);
        }
        close_pipes(0);

    }
    return 0;
}

void *RR_p_fun1(void *arg){
    // pthread_create(&m1, NULL, c3_monitor, NULL); /* Thread Creation **/
    if (fork() == 0){
        c1 = getpid();
        pthread_create(&t1, NULL, c1_work, NULL);
    }
    else {
        while(1){
        shrm();
        if (shmPtr[0] == '0'){
            kill(c1, SIGCONT);
            quantum(c1, time_q);
            kill(c1, SIGSTOP);
            }
          else
              sleep(time_q);
        }
    }
}

void *RR_p_fun2(void *arg){
    // pthread_create(&m1, NULL, c3_monitor, NULL); /* Thread Creation **/
    if (fork() == 0){
        c2 = getpid();
        pthread_create(&t2, NULL, c2_work, NULL);
    }
    else {
        while(1){
        if (shmPtr[0] == '1'){
            kill(c2, SIGCONT);
            quantum(c2, time_q);
            kill(c2, SIGSTOP);
            }
            else {
                sleep(time_q);
            }
        }
    }
}

void *RR_p_fun3(void *arg){
    // pthread_create(&m1, NULL, c3_monitor, NULL); /* Thread Creation **/
    if (fork() == 0){
        c3 = getpid();
        pthread_create(&t3, NULL, c3_work, NULL);
    }
    else {
        while(1){
        if (shmPtr[0] == '2'){
            kill(c3, SIGCONT);
            quantum(c3, time_q);
            kill(c3, SIGSTOP);
            }
            else {
                sleep(time_q);
            }
        }
    }
}

/** Function that executes a proccess for a fixed quanta of time **/
pid_t quantum(pid_t pid, int QUANTUM) {
    struct timespec st,et;
    // struct timeval st, et;
    unsigned long long elapsed = 0;
    int wstatus;
    clock_gettime(CLOCK_REALTIME, &st);

    while((waitpid(pid, &wstatus, WNOHANG) == 0) {
    clock_gettime(CLOCK_REALTIME, &et);
        elapsed = (et.tv_sec-st.tv_sec)+(et.tv_nsec-st.tv_nsec)/BILLION;

        if (elapsed >= QUANTUM) {
            return pid;
        }
    }
}

// void *c1_monitor(void *arg) {
//         while(1){
//             for (int i = 0, i <= 3, )
//             if (sharedMemVar == atoi(i)){
//                 kill(pid, SIGCONT);
//                 quantum(pid, )
//                 kill(pid, SIGSTOP);
//             }
//         }
//     }

// void *c2_monitor(void *arg) {
//     while(1){
        
//     }
// }

// void *c3_monitor(void *arg) {
//     while(1){
        
//     }
// }
 
void *c1_work(void *arg) {
    long long sum = 0;
    for (int i = 0; i < n1; ++i) {
        int x = rand() % 1000000 + 1;
        // int x = i + 1;
        sum += x;
    }

    pipe_write(1, &sum, sizeof(long long));
    close(fd[1][WRITE_END]);

    

    pthread_exit(NULL);
}
 
void *c2_work(void *arg) {
    int num;
    FILE *fptr;
    if ((fptr = fopen("./input.txt", "r")) == NULL) { /* Initialising filepointer **/
        printf("Error! opening file");
        exit(5);
    }
    for (int i = 0; i < n2; ++i) { /* Reading from input.txt **/
        fscanf(fptr, "%d", &num);
        printf("%d\n", num);
    }
    fclose(fptr);
    char ok[20] = "Done Printing";
    int sz = strlen(ok);

    pipe_write(2, &sz, sizeof(int));
    pipe_write(2, ok, sizeof(ok));
    close(fd[2][WRITE_END]);

    pthread_exit(NULL);
}
 
void *c3_work(void *arg) {
    int num;
    FILE *fptr;
    long long sum = 0;
    if ((fptr = fopen("./input.txt", "r")) == NULL) { /* Initialising filepointer **/
        printf("Error! opening file");
        exit(6);
    }
    for (int i = 0; i < n3; ++i) { /* Reading from input.txt **/
        fscanf(fptr, "%d", &num);
        sum += num;
    }
    fclose(fptr);

    pipe_write(3, &sum, sizeof(long long));
    close(fd[3][WRITE_END]);

    pthread_exit(NULL);
}

/* Closes all pipes except the one specified in argument **/
void close_pipes(int num){
    for (int i = 1; i <= 3; ++i) {
        if (i == num)
            continue;
        close(fd[i][READ_END]);
        close(fd[i][WRITE_END]);
    }
}

/* Writes into a pipe **/
void pipe_write(int num, void * address, int size){
    close_pipes(num);
    close(fd[num][READ_END]);
    if (write(fd[num][WRITE_END], address, size) == -1) {
        printf("Error at writing in c%d\n",num);
    }
}

/* Reads from a pipe **/
void pipe_read(int num, void * address, int size){
    close(fd[num][WRITE_END]);
    if ( read(fd[num][READ_END], address, size )== -1 ) {
        printf("Error at reading result from c%d\n",num);
    }
}

char shrm(){
    if( (shmid = shmget(2041, 32, 0)) == -1 )
        exit(7); 
                
    shmPtr = shmat(shmid, 0, 0); 
                
    if (shmPtr == (char *) -1)
        exit(8);
    printf ("\nChild Reading ....\n\n");
                
    for (int n = 0; n < 26; n++)
        putchar(shmPtr[n]);
    putchar('\n');

}