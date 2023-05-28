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
#include <math.h>
#include <ctype.h>

int firstPipe[2];
int secondPipe[2];
int thirdPipe[2];

enum procese
{
    Copil1 = 1,
    Parinte,
    Copil2
};
enum procese proces;

int alarmCalled = 0;

int nrCaractere = 0;
int litereMici[26];
int litereDistincte[26];

clock_t t;

void printTimeElapsed()
{
    clock_t newTime = clock() - t;
    double time_taken = ((double)newTime) / CLOCKS_PER_SEC; // in seconds

    if (proces == Copil1)
    {
        printf("In copil au trecut: %f secunde\n", time_taken);
    }

    if (proces == Parinte)
    {
        printf("In Parinte au trecut: %f secunde\n", time_taken);
    }
}

void alertHandler(int sigInt)
{
    alarmCalled++;
    if (proces == Copil1)
    {
        kill(0, SIGUSR1);
        alarm(1);
    }
    printTimeElapsed();
}

void handler(int sigInt)
{

    if (proces == Copil2)
    {
        printf("Caractere mici citite: %d\n", nrCaractere);

        for (int i = 0; i < 26; i++)
        {

            if (!litereMici[i])
                continue;

            printf("Litera '%c' apare de %d ori\n", 'a' + i, litereMici[i]);
        }
        printf("--------------------------\n");
        nrCaractere = 0;
        memset(litereMici, 0, sizeof(litereMici));
    }
}

int main()
{

    pid_t pid;
    char *input;
    size_t bufferSize = 32;
    signal(SIGUSR1, handler);
    signal(SIGALRM, alertHandler);

    pipe(firstPipe);
    pipe(secondPipe);
    pipe(thirdPipe);
    pid = fork();

    if (pid == 0)
    {
        proces = Copil1;

        t = clock();

        close(firstPipe[1]);
        close(secondPipe[0]);
        close(thirdPipe[0]);
        close(thirdPipe[1]);

        alarm(1);
        char ch;

        while (read(firstPipe[0], &ch, 1))
        {
            if (ch >= 'a' && ch <= 'z')
            {
                write(secondPipe[1], &ch, 1);
            }
        }

        while (alarmCalled < 5)
            ;

        close(secondPipe[1]);
        close(firstPipe[0]);
    }
    else
    {
        pid_t pid2 = fork();
        if (pid2 == 0)
        {
            proces = Copil2;

            int fileDes;
            fileDes = creat("statistica.txt", 0777);

            dup2(fileDes, 1);

            close(firstPipe[0]);
            close(firstPipe[1]);
            close(secondPipe[1]);
            close(thirdPipe[0]);

            char ch;
            int indexToInsert;

            while (read(secondPipe[0], &ch, 1))
            {

                indexToInsert = ch - 'a';
                litereMici[indexToInsert]++;
                litereDistincte[indexToInsert]++;
                nrCaractere++;
            }
            close(secondPipe[0]);

            int litereDistincteCount = 0;
            for (int i = 0; i < 26; i++)
            {

                if (!litereDistincte[i])
                    continue;

                litereDistincteCount++;
            }

            write(thirdPipe[1], &litereDistincteCount, sizeof(litereDistincteCount));

            close(thirdPipe[1]);
            close(fileDes);
        }
        else
        {
            proces = Parinte;

            t = clock();

            int fileDes = open("data.txt", O_RDONLY);

            close(firstPipe[0]);
            close(secondPipe[0]);
            close(secondPipe[1]);
            close(thirdPipe[1]);

            dup2(fileDes, 0);
            alarm(5);

            char ch;

            while (1)
            {
                ch = getchar();

                if (!isspace(ch))
                {
                    write(firstPipe[1], &ch, 1);
                }

                if (feof(stdin) || alarmCalled)
                {
                    printf("Am terminat de citit\n");
                    close(firstPipe[1]);
                    break;
                }
            }

            while (!alarmCalled)
                ;

            int caractereDistincteDeLaCopil;

            read(thirdPipe[0], &caractereDistincteDeLaCopil, sizeof(caractereDistincteDeLaCopil));

            printf("Caractere distincte citite de la copil: %d\n", caractereDistincteDeLaCopil);

            close(thirdPipe[0]);
            close(fileDes);
        }
    }

    return 0;
}