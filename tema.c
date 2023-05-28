#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

int firstPipe[2];
int secondPipe[2];

int alarmCalled = 0;
int seconds = 0;
int position = 0;
int parentFinishedWriting = 0;

int nrCaractere = 0;
int litereMici[26];
clock_t t;

void printTimeElapsed()
{

    clock_t newTime = clock() - t;
    double time_taken = ((double)newTime) / CLOCKS_PER_SEC; // in seconds
    if (position == 4)
    {
        printf("In copil Seconds elapsed: %f \n", time_taken);
    }
    if (position == 2)
    {
        printf("In parinte Seconds elapsed: %f \n", time_taken);
    }
}

void resetAlarm()
{
    if (alarmCalled)
    {
        alarmCalled = 0;
        alarm(1);
    }
}

void alertHandler(int sigInt)
{
    alarmCalled = 1;
    seconds++;
    if (position == 4)
    {
        kill(0, SIGUSR1);
    }
    // if (position == 2)
    // {
    //     printf("Merge alarma in parinte\n");
    // }
    printTimeElapsed();
}

void handler(int sigInt)
{

    if (position == 5)
    {

        printf("Caractere mici citite: %d\n", nrCaractere);

        for (int i = 0; i < 26; i++)
        {

            if (!litereMici[i])
                continue;

            printf("Litera '%c' apare de %d ori\n", 97 + i, litereMici[i]);
        }
        printf("--------------------------\n");
        nrCaractere = 0;
        memset(litereMici, 0, sizeof(litereMici));
    }
}

void secondHandler(int sigInt)
{
    if (position == 4)
    {
        printf("Am trimis semnal la primul copil\n");
        parentFinishedWriting = 1;
    }
}

int main()
{

    pid_t pid;
    char *input;
    size_t bufferSize = 32;
    signal(SIGUSR1, handler);
    signal(SIGUSR2, secondHandler);

    pipe(firstPipe);
    pipe(secondPipe);
    pid = fork();

    if (pid == 0)
    {
        signal(SIGALRM, alertHandler);

        position = 4;

        t = clock();
        close(firstPipe[1]);
        close(secondPipe[0]);
        alarm(1);
        char buffer[2000];
        char ch;

        while (!parentFinishedWriting)
        {
            printf("Nu citi\n");
            resetAlarm();
        }

        int nr;

        read(firstPipe[0], buffer, sizeof(buffer));

        for (int i = 0; i < strlen(buffer); i++)
        {

            if (buffer[i] >= 'a' && buffer[i] <= 'z')
            {
                write(secondPipe[1], &buffer[i], 1);
            }
        }

        while (seconds < 5)
        {
            resetAlarm();
        }

        close(firstPipe[0]);
        close(secondPipe[1]);
    }
    else
    {
        pid_t pid2 = fork();
        if (pid2 == 0)
        {
            int fileDes;
            fileDes = creat("statistica.txt", 0777);

            dup2(fileDes, 1);

            position = 5;
            close(firstPipe[0]);
            close(firstPipe[1]);
            close(secondPipe[1]);

            char ch;
            int nr;
            int indexToInsert;

            while ((nr = read(secondPipe[0], &ch, 1)))
            {

                indexToInsert = ch - 97;
                litereMici[indexToInsert]++;
                nrCaractere++;
            }

            close(secondPipe[0]);
        }
        else
        {

            signal(SIGALRM, alertHandler);

            position = 2;
            t = clock();
            int fileDes = open("data.txt", O_RDONLY);
            char *cuvant = NULL;
            size_t bufsize = 32;

            close(firstPipe[0]);
            close(secondPipe[0]);
            close(secondPipe[1]);

            dup2(fileDes, 0);
            alarm(1);

            while (1)
            {
                resetAlarm();
                ssize_t nread = getline(&cuvant, &bufsize, stdin);

                write(firstPipe[1], cuvant, strlen(cuvant) + 1);

                if (feof(stdin))
                {
                    printf("Am terminat de citit\n");
                    break;
                }
            }

            kill(pid, SIGUSR2);

            while (seconds < 5)
            {
                resetAlarm();
            }

            while (wait(NULL) > 0)
            {
                printf("astept sa se termine copilu\n");
            }

            close(fileDes);
            close(firstPipe[1]);
        }
    }

    return 0;
}