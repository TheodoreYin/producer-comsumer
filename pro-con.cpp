#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstdlib>

#define N_PRODUCER  5
#define MAX_GOODS  5

class queue
{
private:
    int front_p ,back_p;
    int storage[MAX_GOODS];
public:
    queue() : front_p(0), back_p(0) {}
    void init() { front_p = 0; back_p = 0; }
    void push_back(int item) { back_p = (back_p + 1 ) % MAX_GOODS; storage[back_p] = item; }
    int front() { return storage[(front_p + 1) % MAX_GOODS]; }
    void pop_front() {front_p = (front_p + 1) % MAX_GOODS; }
};


sem_t *pro_sem, *con_sem, *full_sem, *empty_sem;
queue *goods;

int get_shm()
{
    int shmid;
    if ((shmid = shmget(1234, sizeof(queue), IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }
    if ((goods = (queue *)shmat(shmid, NULL, 0)) == (queue *) -1) {
        perror("shmat");
        exit(1);
    }
    goods->init();
    return shmid;

}
void attach_shm(int shmid)
{
    if ((shmid = shmget(1234, sizeof(queue), 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }
    if ((goods = (queue *)shmat(shmid, NULL, 0)) == (queue *) -1) {
        perror("shmat");
        exit(1);
    }
}

int init_all_sem()
{
    pro_sem = sem_open("/pc", O_CREAT, 0644, 1);
    con_sem = sem_open("/cc", O_CREAT, 0644, 1);
    empty_sem = sem_open("/ec", O_CREAT, 0644, 0);
    full_sem = sem_open("/fc", O_CREAT, 0644, MAX_GOODS);
    return !(pro_sem == SEM_FAILED || con_sem == SEM_FAILED || full_sem == SEM_FAILED || empty_sem == SEM_FAILED);
}

void close_all_sem()
{
    sem_close(pro_sem);
    sem_close(con_sem);
    sem_close(full_sem);
    sem_close(empty_sem);
    sem_unlink("/pc");
    sem_unlink("/cc");
    sem_unlink("/ec");
    sem_unlink("/fc");
}

void produce()
{
    sem_wait(full_sem);
    sem_wait(con_sem);
    int new_item = std::rand() % 100;
    sleep(1);
    goods->push_back(new_item);
    printf("New Item %d has been produced\n", new_item);
    sem_post(con_sem);
    sem_post(empty_sem);
}

void consume()
{
    sem_wait(empty_sem);
    sem_wait(con_sem);
    sleep(1);
    for (int i = 0; i < 3; i++)
    {
        if (goods->front() % 3 == i)
        {
            printf("Item %d is consumed by %d\n", goods->front(), i);
            goods->pop_front();
            break;
        }
    }
    sem_post(con_sem);
    sem_post(full_sem);
}





int main()
{
    int shmid = get_shm();
    close_all_sem();
    if (! init_all_sem())
        exit(-1);

    int process_code = -1;
    int pidlist[N_PRODUCER + 3];
    int pid;
    for (int i = 0; i < N_PRODUCER + 1; i++)
    {
        pid = fork();
        if (!pid)
        {
            std::srand(i);
            attach_shm(shmid);
            process_code = i;
            break;
        }
        pidlist[i] = pid;
    }
    if (!pid)
    {
        if (process_code < N_PRODUCER)
        {
            printf("Producer %d starts\n", process_code);
            for (int j = 0; j < 2; j++)
                produce();
        }
        else
        {
            for (int j = 0; j < 10; j++)
            {
                printf("Consumer starts\n");
                consume();
            }
        }
        printf("Process %d ends\n", process_code);
        exit(3);
    }
    else
        for (int i = 0; i < N_PRODUCER + 3; i++)
        {
            waitpid(pidlist[i], NULL, 0);
        }
    close_all_sem();

    return 0;
}