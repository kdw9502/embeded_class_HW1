//
// Created by kdw on 2020-04-22.
//

#define SHARED_KEY1 (key_t) 0x10 /* 공유 메모리 키 */
#define SHARED_KEY2 (key_t) 0x15 /*공유 메모리 키 */
#define SEM_KEY (key_t) 0x20 /* 세마포 키 */
#define IFLAGS (IPC_CREAT )
#define ERR ((struct databuf *) -1)

#define SIZE 2048

struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO };
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO };

struct databuf {	/* data 와 read count 저장 */
    int d_nread;
    char d_buf[SIZE];
};

static int shm_id1, shm_id2, sem_id;
/* 초기화 루틴 들*/
void getseg (struct databuf **p1, struct databuf **p2)  {
    if ((shm_id1 = shmget (SHARED_KEY1, sizeof (struct databuf), 0600 | IFLAGS)) == -1) {
        perror("error shmget\n");
        exit(1);
    }

    if ((shm_id2 = shmget (SHARED_KEY2, sizeof (struct databuf), 0600 | IFLAGS)) == -1){
        perror("error shmget\n");
        exit(1);
    }

    if ((*p1 = (struct databuf *) shmat (shm_id1, 0, 0)) == ERR){
        perror("error shmget\n");
        exit(1);
    }

    if ((*p2 = (struct databuf *) shmat (shm_id2, 0, 0)) == ERR){

        perror("error shmget\n");
        exit(1);
    }
}
int getsem (void)
{
    semun x;
    x.val = 0;
    int id=-1;

    if ((id = semget (SEM_KEY, 2, 0600 | IFLAGS) ) == -1) 	exit(1);
    if (semctl ( id, 0, SETVAL, x) == -1) 		exit(1);
    if (semctl ( id, 1, SETVAL, x) == -1)		exit(1) ;

    return (id);
}


void remobj (void)
{
    if (shmctl (shm_id1, IPC_RMID, 0) == -1)		exit(1);
    if (shmctl (shm_id2, IPC_RMID, 0) == -1)		exit(1);
    if (semctl (sem_id, 0, IPC_RMID, 0) == -1) 		exit(1) ;
}

main () {
    pid_t pid;
    struct databuf *buf1, *buf2;

    sem_id = getsem();	 		// 세마포 생성 및 초기화

    getseg (&buf1, &buf2); 		// 공유 메모리 생성 및 부착

    switch (pid = fork ()) {
        case -1:
            perror("fork");
            break;

        case 0:
            writer (sem_id, buf1, buf2);	// 자식 프로세스, 공유메모리  표준출력
            remobj ();
            break;

        default:			// 부모 프로세스 ,표준입력  공유메모리
            reader (sem_id, buf1, buf2);
            break;
    }
}

// reader – 파일 읽기 처리 (표준입력  공유 메모리)

void reader (int semid, struct databuf *buf1, struct databuf *buf2)
{
    for (;;)
    {
        buf1 ->d_nread = read (0, buf1 -> d_buf, SIZE);

        semop (semid, &v1, 1);
        semop (semid, &p2, 1);

        if (buf1 -> d_nread <= 0)
            return;

        buf2 -> d_nread = read (0, buf2 -> d_buf, SIZE);

        semop (semid, &v1, 1);
        semop (semid, &p2, 1);

        if (buf2 -> d_nread <= 0)
            return;
    }
}

// writer – 파일 쓰기 처리 (공유 메모리  표준출력)

void writer (int semid, struct databuf *buf1, struct databuf *buf2)
{
    for (;;)
    {


        semop (semid, &p1, 1);
        semop (semid, &v2, 1);

        if (buf1 -> d_nread <= 0)
            return;

        write (1, buf1 -> d_buf, buf1 -> d_nread);

        semop (semid, &p1, 1);
        semop (semid, &v2, 1);

        if (buf2 -> d_nread <= 0)
            return;

        write (1, buf2 -> d_buf, buf2 -> d_nread);
    }
}

