#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char fname[20] = "etc/branches.txt";

#define SUCESS_EXIT 0
#define FAIL_EXIT 1
#define null NULL

int main(void)
{
    FILE* fp;
    FILE* outfile;
    char buffer[128];
    char clean_buffer[128];
    char cmd[12] = "git branch";
    char firstCharToVerify = '*';
    char secondCharToVerify = ' ';

    fp = popen(cmd, "r");

    if (fp == null)
    {
        printf("Erro fatal: o comando não pode ser executado\n");
        return FAIL_EXIT;
    }

    outfile = fopen(fname, "w");

    if (outfile == null)
    {
        printf("Erro fatal ao abrir o arquivo de saída");
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
        int i, j;
        j = 0;
        int skippedFirstSpace = 0;

        for (i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == firstCharToVerify) {
                continue;
            }
            
            if (buffer[i] == secondCharToVerify && skippedFirstSpace == 0) {
                skippedFirstSpace = 1;
                continue;
            }

            if (buffer[i] == secondCharToVerify && skippedFirstSpace == 1)
            {
                skippedFirstSpace = 2;
                continue;
            }

            clean_buffer[j] = buffer[i];
            j++;
        }
        
        clean_buffer[j] = '\0';

        fprintf(outfile, "%s", clean_buffer);
    }
    
    pclose(fp);
    fclose(outfile);

    return SUCESS_EXIT;
}
