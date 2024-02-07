#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLENNAME 256
#define MAXLENLINE 1024

void custom_print(const char *word, int off, pid_t curpid)
{
    int i;
    for(i=0; i<off; i++){
        printf("\t");
    }
    printf("%s (%u)\n", word, curpid);
}

int main (int argc, char *argv[])
{
    if(argc==1)
    {
        printf("Run with a node name\n");
        exit(1);
    }
    else if(argc>3)
    {
        printf("Too many arguments\n");
        exit(1);
    }
    else
    {
        char name[MAXLENNAME];
        char buffer[MAXLENLINE];

        sprintf(name, "%s", argv[1]);
        name[strlen(argv[1])] = '\0';

        int off = 0;
        if(argc==3)
        {
            off = atoi(argv[2]);
        }

        FILE *fp = fopen("treeinfo.txt", "r");

        if(fp==NULL)
        {
            printf("File not found\n");
            exit(1);
        }

        int found = 0, nchild = 0;
        char * token = NULL;
        
        while(fgets(buffer, 1023, fp)!=NULL)
        {
            token = strtok(buffer, " ");

            if(token!=NULL && strcmp(token, name)==0)
            {
                found = 1;
                token = strtok(NULL, " ");
                nchild = atoi(token);
                break;
            }

            memset(buffer, 0, MAXLENLINE);
        }

        if(!found)
        {
            printf("City %s not found\n", name);
            exit(1);
        }

        pid_t cpid, mypid, parpid;

        mypid = getpid();
        parpid = getppid();

        custom_print(name, off, mypid);
        off++;

        char offstr[10];
        sprintf(offstr, "%d", off);

        int i;
        for(i=0; i<nchild; i++)
        {
            token = strtok(NULL, " ");
            cpid = fork();

            if (cpid == 0) {
                mypid = getpid();
                parpid = getppid();

                sprintf(name, "%s", token);
                if(token[strlen(token)-1]=='\n')
                {
                    name[strlen(token)-1] = '\0';
                }

                char * argsexec[] = {"proctree", name, offstr, NULL};
                
                execv("proctree", argsexec);

                printf("This statement is not executed if execv succeeds.\n");
                exit(i);
            }
            else {
                waitpid(cpid, NULL, 0);
            }
        }

        fclose(fp);
    }
    
    exit(0);
}