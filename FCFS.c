#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <signal.h>
 
int n1, n2, n3;
int fd[4][2];
double time_last[3];
int c1=-1, c2=-1, c3=-1; /** pid's of child proccesses **/
pid_t p1,p2,p3;
pthread_t t1, t2, t3, t4, t5, t6;

#define READ_END 0
#define WRITE_END 1

void *c1_work(void *);
void *c2_work(void *);
void *c3_work(void *);

void close_pipes(int num);

void pipe_write(int num, void * address, int size);
void pipe_read(int num, void * address, int size);
 
int main(int argc, char *argv[])
{
    char scheduling_algo[30];
    int time_q = -1;
    int wpid, status;
    scanf("%d %d %d", &n1, &n2, &n3);
    scanf("%s", scheduling_algo);

    if (strcmp(scheduling_algo, "RR") == 0)
        scanf("%d", &time_q);

    for (int i = 1; i <= 3; ++i) /* Creating Pipes **/
        if (pipe(fd[i]) == -1) {
            printf("Error while creating pipes\n");
            return 1;
        }

    ((c1 = fork()) && (c2 = fork()) && (c3 = fork())); /* Creating Processess **/

    /* Child 1 **/
    if (c1 == 0) {
        clock_t begin = clock();
        p1 = getpid();
        kill(c2, SIGSTOP);
        kill(c3, SIGSTOP);
        printf("In child1, id is %d\n", getpid());
        pthread_create(&t1, NULL, c1_work, NULL); /* Thread Creation **/
        pthread_join(t1, NULL);
        printf("C1 Completed\n");
        kill(c2, SIGCONT);
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Turnaround Time: %f\n", time_spent);
        printf("Burst Time: %f\n", time_last[0]);
        printf("Waiting Time: %f\n", time_spent-time_last[0]);
    }
    /* Child 2 **/
    else if (c2 == 0) {
        clock_t begin = clock();
        p2 = getpid();
        sleep(1);
        kill(c3, SIGSTOP);
        printf("In child2, id is %d\n", getpid());
        pthread_create(&t2, NULL, c2_work, NULL); /* Thread Creation **/
        pthread_join(t2, NULL);
        printf("C2 Completed\n");
        kill(c3, SIGCONT);
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Turnaround Time: %f\n", time_spent);
        printf("Burst Time: %f\n", time_last[1]);
        printf("Waiting Time: %f\n", time_spent-time_last[1]);
    }
    /* Child 3 **/
    else if (c3 == 0) {
        clock_t begin = clock();
        p3 = getpid();
        sleep(2);
        printf("In child3, id is %d\n", getpid());
        pthread_create(&t3, NULL, c3_work, NULL); /* Thread Creation **/
        pthread_join(t3, NULL);        
        printf("C3 Completed\n");
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Turnaround Time: %f\n", time_spent);
        printf("Burst Time: %f\n", time_last[2]);
        printf("Waiting Time: %f\n", time_spent-time_last[2]);
    }
    /* Parent **/
    else {
        while ((wpid = wait(&status)) > 0);
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
        exit(0);
    }
    return 0;
}
 
void *c1_work(void *arg) {
    clock_t begin = clock();
    long long sum = 0;
    for (int i = 0; i < n1; ++i) {
        int x = rand() % 1000000 + 1;
        //int x = i + 1;
        sum += x;
    }

    pipe_write(1, &sum, sizeof(long long));
    close(fd[1][WRITE_END]);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    time_last[0] = time_spent;
    pthread_exit(NULL);

}
 
void *c2_work(void *arg) {
    clock_t begin = clock();
    int num;
    FILE *fptr;
    if ((fptr = fopen("./input.txt", "r")) == NULL) { /* Initialising filepointer **/
        printf("Error! opening file");
        exit(1);
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
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    time_last[1] = time_spent;
    pthread_exit(NULL);
}
 
void *c3_work(void *arg) {
    clock_t begin = clock();
    int num;
    FILE *fptr;
    long long sum = 0;
    if ((fptr = fopen("./input.txt", "r")) == NULL) { /* Initialising filepointer **/
        printf("Error! opening file");
        exit(2);
    }
    for (int i = 0; i < n3; ++i) { /* Reading from input.txt **/
        fscanf(fptr, "%d", &num);
        sum += num;
    }
    fclose(fptr);

    pipe_write(3, &sum, sizeof(long long));
    close(fd[3][WRITE_END]);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    time_last[2] = time_spent;
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