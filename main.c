#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
// #include <sys/stat.h>

#if __UNIX__
#include <unistd.h>
#elif __INIt__
#include <unistd.h>
#elif __MINGW64
#include <direct.h>
#elif _WIN32
#include <direct.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define sleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep(ms) usleep((ms) * 1000)
#endif


char fname[32] = "etc/local_branches_tracking.txt";
char allbfname[32] = "etc/all_remote_branches.txt";
const char* pname = "btmap";

#define RESET "\x1b[0m"

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[37m"

#define GREEN_BRIGHT "\x1b[92m"
#define CYAN_BRIGHT "\x1b[96m"

#define NEGRITO         "\x1b[1m"
#define SUB_LINHADO     "\x1b[4m"
#define INVERTER_CORES  "\x1b[7m"

#define SUCESS_EXIT 0
#define FAIL_EXIT 1
#define null NULL

// Structs region

typedef struct remote_data_t
{
    char remote[128];
    char origin[64];
    char branch[64];
} remote_data;

typedef struct local_data_t {
    char name[128];
    int isCurrent;
} local_data;


// Enum(s) region

typedef enum LOGO_SHOW {
    ShowLogo = 0,
    ShowMadeBy = 1
} logo_show_mode;


int btmapHelpMessage();
int initHelpMessage();
int readAndWriteInFile();
void printErrorMessage();
void printToolVersion(int int1, int int2);
void branchTracking(FILE* poutfile);
int find_local_branch(local_data* locals, int total, const char* branch);
int interactiveRemote(remote_data* list_remote, local_data* list_local, int totalR, int totalL);
int parseRemote(remote_data* list, int max);
int syncLocalBranches(remote_data *remotes, int totalR, local_data *locals, int totalL);
int parseLocal(local_data* list, int max);
int interactiveLocal(local_data* list, int total);
int loopInteract();
void bar_execution(int time, int width, int total_steps, void* generic);
int opcoesInteract(void* generic);
void loadDependencies(char* flname);
void logo(int showLogo, int showMadeBy);
void print_progress(double fraction, int width);
int create_branch(const char* __origin, const char* __branch);
int update_branch(const char* origin, const char* branch);
int safe_update_branch(const char* origin, const char* branch);

FILE* fp;
FILE* outfile;
char branch_buffer[128];
char clean_branch_buffer[128];
char cmd[12] = "git branch";
char firstCharToVerify = '*';
char secondCharToVerify = ' ';

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printErrorMessage();
        return FAIL_EXIT;
    }

    if (strcmp(argv[1], "-h") == 0 || (strcmp(argv[1], "--help") == 0))
    {
        btmapHelpMessage();
        return SUCESS_EXIT;
    }
    else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
    {
        if (argc > 2)
        {
            if (strcmp(argv[2], "-l") == 0 || strcmp(argv[2], "--logo") == 0)
            {
                printToolVersion(0, 1);
                return SUCESS_EXIT;
            }
            else if (strcmp(argv[2], "-nl") == 0 || strcmp(argv[2], "--no-logo") == 0)
            {
                printToolVersion(1, 0);
                return SUCESS_EXIT;
            }
            else {
                printErrorMessage();
                return FAIL_EXIT;
            }
        }
        printToolVersion(1, 0);
        return SUCESS_EXIT;
    }


    if (strcmp(argv[1], "init") == 0)
    {
        if (argc < 3)
        {
            printf("%sErro fatal:%s %sinit requer mais argumentos. tente o comando abaixo para obter ajuda%s\n\n\t%s%s%s%s %s %s-h%s\n", RED, RESET, YELLOW, RESET, BLUE, SUB_LINHADO, pname, RESET, argv[1], GREEN, RESET);
            return FAIL_EXIT;
        }
        if (strcmp(argv[2], "-i") == 0 || strcmp(argv[2], "--interactive") == 0)
        {
            if (argc == 3)
            {
                opcoesInteract((void*)1);
                return SUCESS_EXIT;
            }
        }
        else if (strcmp(argv[2], "-ni") == 0 || strcmp(argv[2], "--no-interactive") == 0)
        {
            if (argc == 3)
            {
                opcoesInteract(NULL);
                return SUCESS_EXIT;
            }
        }
        else if (strcmp(argv[2], "-h") == 0 || strcmp(argv[2], "--help") == 0)
        {
            if (argc == 3)
            {
                initHelpMessage();
                return SUCESS_EXIT;
            }
        }
        else if (strcmp(argv[2], "-L") == 0 || strcmp(argv[2], "--load") == 0)
        {
            opcoesInteract((void*)1);
            return SUCESS_EXIT;
        }
        else if (strcmp(argv[2], "-nL") == 0 || strcmp(argv[2], "--no-load") == 0)
        {
            opcoesInteract(NULL);
            return SUCESS_EXIT;
        }
        else
        {
            if (argc == 2) {
                printf("%sErro fatal:%s %sopcao invalida para 'init'. tente o comando abaixo para obter ajuda%s\n\n\t%s%s%s%s %s %s-h%s\n", RED, RESET, YELLOW, RESET, BLUE, SUB_LINHADO, pname, RESET, argv[1], GREEN, RESET);
                return FAIL_EXIT;
            }
        }
    }
    else
    {
        printf("%sErro fatal:%s %sopcao invalida. tente o comando abaixo para obter ajuda%s\n\n\t%s%s%s%s %s-h%s\n", RED, RESET, YELLOW, RESET, BLUE, SUB_LINHADO, pname, RESET, GREEN, RESET);
        return FAIL_EXIT;
    }

    return SUCESS_EXIT;
}

int safe_update_branch(const char *origin, const char *branch) {
    char cmd[512];

    snprintf(cmd, sizeof(cmd),
        "git checkout %s && "
        "git stash push -u -m \"btmap-autostash\" && "
        "git pull %s %s && "
        "git stash pop",
        branch, origin, branch);

    int rc = system(cmd);
    if (rc != 0) {
        printf("%sErro na atualizacao segura da branch %s%s\n",
               RED, branch, RESET);
        return FAIL_EXIT;
    }

    return SUCESS_EXIT;
}

int update_branch(const char *origin, const char *branch) {
    char cmd[256];

    snprintf(cmd, sizeof(cmd),
             "git checkout %s && git pull %s %s",
             branch, origin, branch);

    int rc = system(cmd);
    if (rc != 0) {
        printf("%sErro ao atualizar a branch %s%s\n",
               RED, branch, RESET);
        return FAIL_EXIT;
    }

    return SUCESS_EXIT;
}

int find_local_branch(local_data *locals, int total, const char *branch) {
    for (int i = 0; i < total; i++) {
        if (strcmp(locals[i].name, branch) == 0)
            return i;
    }
    return -1;
}

int initHelpMessage()
{
    char subcmd[5] = "init";
    printf("%s%sUso:%s %s%s%s %s %s[OPCOES]%s %s[ARGS]%s\n\n", GREEN_BRIGHT, NEGRITO, RESET, BLUE, pname, RESET, subcmd, GREEN, RESET, YELLOW, RESET);
    printf("      %s-[n]i, --[no-]interactive\t\t%sopcao para usar ou nao o modo interativo\n", GREEN, RESET, WHITE, NEGRITO, RESET);
    printf("      %s-[n]L, --[no-]load\t\t%sopcao para usar ou nao o modo interativo\n", GREEN, RESET, WHITE, NEGRITO, RESET);
    printf("      %s-r, --remote\t\t\t%sopcao para indicar o repo remoto nos %sARGS%s\n", GREEN, RESET, YELLOW, RESET);
    printf("      %s-b, --branch\t\t\t%sopcao para indicar a branch local nos %sARGS%s\n\n", GREEN, RESET, YELLOW, RESET);
    printf("   %sARGS:%s\n", YELLOW, RESET);
    printf("      %sorigin\t\t%sindica qual o nome do remote. Veja os exemplos da ferramenta\n", YELLOW, RESET);
    printf("      %sbranch\t\t%sindica qual o nome da branch. Veja os exemplos da ferramenta\n", YELLOW, RESET);
    return SUCESS_EXIT;
}

int btmapHelpMessage()
{
    logo(0, 1);
    printf("%s%sUso:%s %s%s%s %s%s[COMANDOS]%s %s[OPCOES]%s %s[ARGS]%s\n\n", GREEN_BRIGHT, NEGRITO, RESET, BLUE, pname, RESET, WHITE, NEGRITO, RESET, GREEN, RESET, YELLOW, RESET);
    printf("   %s%sCOMANDOS:%s\n", WHITE, NEGRITO, RESET);
    printf("      %s%sinit\t\t%sinicia o mapeamento da ferramenta\n", WHITE, NEGRITO, RESET);
    printf("      %s%sexamples\t\t%smostra alguns exemplos de como usar a ferramenta\n\n", WHITE, NEGRITO, RESET);
    printf("   %sOPCOES:%s\n", GREEN, RESET);
    printf("      %s-h, --help\t\t%smostra essa mensagem de ajuda\n", GREEN, RESET);
    printf("      %s-v, --version\t\t%smostra a versao atual da ferramenta\n", GREEN, RESET);
    printf("      %s-[n]l, --[no-]logo\t%ssubopcao usada em \"--version\". Para mais detalhes, veja os exemplos.\n\n", GREEN, RESET);
    return SUCESS_EXIT;
}

int opcoesInteract(void* generic)
{
    signal(SIGINT, SIG_IGN);

    remote_data rlist[128];
    local_data llist[128];
    int totalR = parseRemote(rlist, 128);
    int totalL = parseLocal(llist, 128);
    printf("%s%sEntrando no modo interativo...%s\n", CYAN, NEGRITO, RESET);
    bar_execution(30, 40, 100, generic);
    printf("\n\t%s!Modo interativo ativo!%s\n\n", GREEN_BRIGHT, RESET);
    char ans[6];
    int result = 0;

    while (1) {
        if (result == -1) break;

        printf("%s%sOpcoes atuais disponiveis: [r,l,q,?]:%s %s%s", BLUE, NEGRITO, RESET, GREEN, NEGRITO);
        scanf("%15s", ans);
        printf("%s", RESET);

        if (strcmp(ans, "?") == 0)
        {
            printf("%sr -> remote (tudo sobre o rastreamento do remote)\n", CYAN_BRIGHT);
            printf("l -> local (tudo sobre o rastreamento do local)\n");
            printf("q -> sair\n");
            printf("? -> exibe essa mensagem de ajuda\n%s", RESET);
            continue;
        }
        else if (strcmp(ans, "q") == 0)
        {
            printf("%s%sSaindo...%s\n", YELLOW, NEGRITO, RESET);
            return SUCESS_EXIT;
        }
        else if (strcmp(ans, "l") == 0)
        {
            result = interactiveLocal(llist, totalL);
            continue;
        }
        else if (strcmp(ans, "r") == 0)
        {

            result = interactiveRemote(rlist, llist, totalR, totalL);
            continue;
        }
        else {
            printf("%sO comando '%s' que voce digitou e desconhecido! %sTente %s%s?%s%s para obter ajuda.%s\n", RED, ans, YELLOW, GREEN, NEGRITO, RESET, YELLOW, RESET);
            continue;
        }
    }
}

static void enable_ansi_on_windows(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

void print_progress(double fraction, int width) {
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    int filled = (int)(fraction * width + 0.5);
    int empty = width - filled;
    int percent = (int)(fraction * 100.0 + 0.5);

    printf("\r");

    if (percent < 100)
        printf("%s%3d%%%s %s[", YELLOW, percent, RESET, BLUE);
    else
        printf("%s%3d%%%s %s[", GREEN, percent, RESET, BLUE);

    if (filled > 0) {
        printf("\x1b[42m");
        for (int i = 0; i < filled; ++i) putchar(' ');
        printf("\x1b[0m");
    }

    for (int i = 0; i < empty; ++i) putchar(' ');

    printf("%s]", BLUE);

    fflush(stdout);
}

void bar_execution(int time, int width, int total_steps, void* generic)
{
    if (!generic)
    {
        sleep(800);
        return;
    }
    enable_ansi_on_windows();

    for (int i = 0; i <= total_steps; ++i) {
        double frac = (double)i / (double)total_steps;
        print_progress(frac, width);
        sleep(time);
    }
}

int create_branch(const char *origin, const char *branch) {
    char cmd[256];

    snprintf(cmd, sizeof(cmd),
             "git checkout -b %s %s/%s",
             branch, origin, branch);

    int rc = system(cmd);
    if (rc != 0) {
        printf("%sErro ao criar a branch %s/%s%s\n",
               RED, origin, branch, RESET);
        return FAIL_EXIT;
    }

    return SUCESS_EXIT;
}


void loadDependencies(char* flname) {
    // int max_tokens = 10;
    int max_tokens = 10;
    // int buffer_size = 50;
    int buffer_size = 50;
    char** tokens = (char**)malloc(sizeof(char*) * max_tokens);
    if (tokens == NULL) {
        perror("Falha na alocacao inicial de tokens");
        return;
    }

    char* cpy = (char*)malloc(sizeof(char) * buffer_size);
    if (cpy == NULL) {
        perror("Falha na alocacao de cpy");
        free(tokens);
        return;
    }
    strncpy(cpy, allbfname, buffer_size - 1);
    cpy[buffer_size - 1] = '\0';


    char* token;
    int token_count = 0;

#if __MINGW64__
    token = strtok(cpy, "\\");
#elif _WIN32
    token = strtok(cpy, "\\");
#elif __UNIX__
    token = strtok(cpy, "/");
#endif
    while (token != NULL && token_count < max_tokens) {
        tokens[token_count] = (char*)malloc((strlen(token) + 1) * sizeof(char));
        if (tokens[token_count] == NULL) {
            for (int i = 0; i < token_count; i++) {
                free(tokens[i]);
            }
            free(tokens);
            free(cpy);
            return;
        }

        strcpy(tokens[token_count], token);

        token = strtok(NULL, "/\\");
        token_count++;
    }

    char buffer[50];
    FILE* file;
    for (int i = 0; i < token_count; i++) {
        if (i == 0)
        {
#if __UNIX__
            snprintf(buffer, sizeof(buffer), "mkdir %s", tokens[i]);
            system(buffer);
#elif __MINGW64__
            snprintf(buffer, sizeof(buffer), "mkdir %s", tokens[i]);
            system(buffer);
#elif __INIT__
            snprintf(buffer, sizeof(buffer), "mkdir %s", tokens[i]);
            system(buffer);
#elif _WIN32
            snprintf(buffer, sizeof(buffer), "dir %s", tokens[i]);
            system(buffer);
#endif
        }
        if (i == 1) {
            file = fopen(flname, "w");
        }
    }
    fclose(file);

    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);
    free(cpy);
}

#define MALLOC_BUFFER_SIZE 40

int interactiveRemote(remote_data* list_remote, local_data* list_local, int totalR, int totalL)
{
    char ans[16];
    int control = 0;
    char* buffer = malloc(sizeof(char) * MALLOC_BUFFER_SIZE);

    if (totalR < 1)
    {
        printf("%s%sNada no repositorio remoto%s\n", RED, NEGRITO, RESET);
        return FAIL_EXIT;
    }

    if (totalR < 2)
        printf("\n%s%s%d%s %sbranch remota encontrada.%s\n", GREEN_BRIGHT, NEGRITO, totalR, RESET, BLUE, RESET);
    else
        printf("\n%s%s%d%s %sbranches remotas encontradas.%s\n", GREEN_BRIGHT, NEGRITO, totalR, RESET, BLUE, RESET);

    int index;
    for (int i = 0; i < totalR; i++)
    {
        int localIndex = find_local_branch(list_local, totalL, list_remote[i].branch);
        printf("\n%sBranch remotas%s %s%s%d%s/%s%d%s:\n", NEGRITO, RESET, GREEN_BRIGHT, NEGRITO, i + 1, RESET, YELLOW, totalR, RESET);
        printf("%s%sRemote completo:%s %s%s%s\n", BLUE, NEGRITO, RESET, YELLOW, list_remote[i].remote, RESET);
        printf("%s%sOrigin%s         : %s%s%s\n", CYAN_BRIGHT, NEGRITO, RESET, YELLOW, list_remote[i].origin, RESET);
        printf("%s%sBranch%s         : %s%s%s\n", CYAN_BRIGHT, NEGRITO, RESET, YELLOW, list_remote[i].branch, RESET);
        for (int j = 0; j < totalL; j++) {
            snprintf(buffer, MALLOC_BUFFER_SIZE, "%s%s%s%s%s%s", !strcmp(list_remote[i].branch, list_local[j].name) ? "c," : "", strcmp(list_remote[i].branch, list_local[j].name) != 0 ? "s,u," : "", (totalR >= 2 || totalL >= 2) ? "p," : "", "q,", "Q,", "?");

            printf("\n%sBranches locais%s %s%s%d%s/%s%d%s:\n", NEGRITO, RESET, GREEN_BRIGHT, NEGRITO, j + 1, RESET, YELLOW, totalL, RESET);
            printf("%s%sBranch:%s %s%s%s\n", BLUE, NEGRITO, RESET, YELLOW, list_local[j].name, RESET);

            // for (int k = 0; k < totalL; k++)
            // {
            //     if (strcmp(list_local[k].name, list_remote[i].branch) == 0)
            //         index++;
            //     else break;
            // }

            while (1)
            {
                if (localIndex == -1) {
                    index = 0; // nao existe a branch local
                    printf("%s[c] criar  [p] pular  [q] sair  [?]%s ",
                       GREEN, RESET);
                }
                else {
                    printf("%s[u] update  [s] safe-update  [p] pular  [q] sair  [?]%s ",
                       GREEN, RESET);
                    index = 1; // ja existe a branch local'
                }
                // printf("%s\n", index);
                printf("%s%sO que deseja fazer [%s]:%s %s%s", BLUE, NEGRITO, buffer, RESET, GREEN, NEGRITO);
                scanf("%15s", ans);
                printf("%s", RESET);

                if (strcmp(ans, "Q") == 0) {
                    printf("%sForcando saida!%s\n", YELLOW, RESET);
                    return -1;
                }
                else if (strcmp(ans, "q") == 0)
                {
                    printf("%sVoltando...%s\n", YELLOW, RESET);
                    return SUCESS_EXIT;
                }
                else if (strcmp(ans, "?") == 0)
                {
                    if (strcmp(list_remote[i].branch, list_local[j].name) && index == 0)
                        printf("%sc  -> cria a branch em contexto\n", CYAN_BRIGHT);
                    if (strcmp(list_remote[i].branch, list_local[j].name) == 0)
                    {
                        printf("%su  -> update (atualiza a branch local com a remota)\n", CYAN_BRIGHT);
                        printf("%ss  -> safe update (atualiza de forma segura com stash)\n", CYAN_BRIGHT);
                    }
                    if (totalR >= 2 || totalL >= 2)
                        printf("%sp  -> pass (ignorar essa atualizacao)\n", CYAN_BRIGHT);
                    printf("q  -> volta para opcoes anteriores\n");
                    printf("Q  -> forca a parada do programa\n");
                    printf("?  -> exibir essa mensagem%s\n", RESET);
                    continue;
                }
                else if (strcmp(ans, "u") == 0)
                {
                    printf("Atualizando %s/%s...\n",
                        list_remote[i].origin, list_local[j].name);
                    continue;
                }
                else if (strcmp(ans, "c") == 0 && index == 0)
                {
                    // printf("%sVoltando...%s\n", YELLOW, RESET);
                    int res = create_branch(list_remote[i].origin, list_remote[i].branch);
                    return res;
                }
                else if (strcmp(ans, "s") == 0)
                {
                    printf("%s%sAtualizacao segura...%s\n", GREEN, NEGRITO, RESET);
                    continue;
                }
                if (strcmp(ans, "p") == 0)
                {
                    if (totalL >= 2 || totalR >= 2)
                    {
                        if (i + 1 == totalR && j + 1 == totalL)
                        {
                            printf("%s%sSaindo...%s\n", YELLOW, NEGRITO, RESET);
                            return SUCESS_EXIT;
                        }
                        printf("%s%sPassando para a proxima%s\n", WHITE, NEGRITO, RESET);
                        break;
                    }
                }

                else {
                    printf("%sO comando '%s' que voce digitou e desconhecido! %sTente %s%s?%s%s para obter ajuda.%s\n", RED, ans, YELLOW, GREEN, NEGRITO, RESET, YELLOW, RESET);
                    continue;
                }
            }
        }
    }
    free(buffer);
}

int parseRemote(remote_data* list, int max)
{
    char* pwd;
    char fbuffer[100];
    FILE* fp = popen("git branch --all", "r");
    if (!fp) {
        printf("Erro fatal: o comando não pode ser executado\n");
        return 0;
    }

#if __UNIX__
    if (getcwd(fbuffer, sizeof(fbuffer)) == NULL) { perror("Erro ao obter o diretorio (getcwd() error)"); return EXIT_FAILURE; }
    snprintf(fbuffer, sizeof(fbuffer), "%s/%s", fbuffer, allbfname);
    // printf("%s\n", fbuffer);
#elif __MINGW64__
    pwd = _getcwd(NULL, 0);
    if (pwd == NULL) {
        perror("Erro ao obter o diretorio (_getcwd() error)");
        return EXIT_FAILURE;
    }
    char* slash = strchr(allbfname, '/');
    *slash = '\\';
    snprintf(fbuffer, sizeof(fbuffer), "%s\\%s", pwd, allbfname);
    // printf("%s\n", fbuffer);
    // printf("E mingw64\n");
#elif __MINGW32__
    pwd = _getcwd(NULL, 0);
    if (pwd == NULL) {
        // perror("Erro ao obter o diretorio (_getcwd() error)");
        return EXIT_FAILURE;
    }
    snprintf(fbuffer, sizeof(fbuffer), "%s\\%s", pwd, allbfname);
    printf("%s\n", fbuffer);
#elif _WIN32
    pwd = _getcwd(NULL, 0);
    if (pwd == NULL) {
        // perror("Erro ao obter o diretorio (_getcwd() error)");
        return EXIT_FAILURE;
    }
    // *slash = '\\';
    snprintf(fbuffer, sizeof(fbuffer), "%s\\%s", pwd, allbfname);
    // printf("%s\n", fbuffer);
#endif

    FILE* outfile = fopen(fbuffer, "w");
    if (outfile) {
        loadDependencies(fbuffer);
    }

    outfile = fopen(fbuffer, "w");

    char buffer_remote[200];
    char clean_buffer_remote[200];

    int count = 0;

    while (fgets(buffer_remote, sizeof(buffer_remote), fp))
    {
        int i = 0, j = 0;
        int skippedFirstSpace = 0;

        memset(clean_buffer_remote, 0, sizeof(clean_buffer_remote));

        for (i = 0; buffer_remote[i] != '\0'; i++)
        {
            if (buffer_remote[i] == '*') continue;

            if (buffer_remote[i] == ' ' && skippedFirstSpace < 2) {
                skippedFirstSpace++;
                continue;
            }

            clean_buffer_remote[j++] = buffer_remote[i];
        }

        clean_buffer_remote[j] = '\0';

        if (strncmp(clean_buffer_remote, "remotes/", 8) != 0)
            continue;

        char* p = clean_buffer_remote + 8;
        char* slash = strchr(p, '/');
        if (!slash) continue;

        *slash = '\0';
        char* origin = p;
        char* branch = slash + 1;
        size_t espaces = strcspn(branch, " ");

        /**
          * O se tiver um espaço na posição 4 do meu ponteiro `branch`, então isso não é qualquer branch,
          * é branch atual que estamos, ou seja, o HEAD.
          * ex: remotes/origin/HEAD -> origin/main
          * como o na criação do ponteiro `branch` eu descartei tudo o que estava antes da última '/', então essa variavél, usando esse contexto, vai ser igual a: HEAD -> origin/main
          */
        if (espaces == 4) {
            //                                                      HEAD -> origin/main
            // tento ver se tem algum caractere igual a " " no índice 4 ^
            //
            // O que precisa ser feito?
            // Se isso for verdadeiro, eu preciso que o programa ignore essa branch.
            continue;
        }
        branch[strcspn(branch, "\n")] = 0;

        snprintf(list[count].remote, sizeof(list[count].remote),
            "remotes/%s/%s", origin, branch);

        strncpy(list[count].origin, origin, sizeof(list[count].origin));
        strncpy(list[count].branch, branch, sizeof(list[count].branch));

        fprintf(outfile, "%s\n", list[count].remote);

        count++;
        if (count >= max) break;
    }

    fclose(outfile);
    pclose(fp);

    if (pwd) free(pwd);
    return count;
}

int parseLocal(local_data* list, int max)
{
    FILE* fp = popen("git branch", "r");
    if (!fp) {
        printf("Erro fatal: o comando não pode ser executado\n");
        return 0;
    }

    char buffer[200];

    int count = 0;

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (count >= max)
            break;

        // identifica branch atual
        int isCurrent = (buffer[0] == '*');

        // pular símbolo '*' e espaços
        char* name = buffer + (isCurrent ? 2 : 2);

        // remove newline
        name[strcspn(name, "\n")] = 0;

        // salva na struct
        strncpy(list[count].name, name, sizeof(list[count].name));
        list[count].isCurrent = isCurrent;

        count++;
    }

    pclose(fp);
    return count;
}

int interactiveLocal(local_data* list, int total) {
    char ans[2];
    remote_data remoteList[128];
    int control = 0;

    if (total < 1)
    {
        printf("%s%sNada no repositorio local%s\n", RED, NEGRITO, RESET);
        return FAIL_EXIT;
    }
    if (total == 1)
        printf("\n%s%s%d%s %sbranch local encontrada.%s\n", GREEN_BRIGHT, NEGRITO, total, RESET, BLUE, RESET);
    else
        printf("\n%s%s%d%s %sbranches locais encontradas.%s\n", GREEN_BRIGHT, NEGRITO, total, RESET, BLUE, RESET);

    for (int i = 0; i < total; i++)
    {
        // for (int j = 0; i < )
        printf("\n%sBranch%s %s%s%d%s/%s%d%s:\n", NEGRITO, RESET, GREEN_BRIGHT, NEGRITO, i + 1, RESET, YELLOW, total, RESET);
        printf("%s%sBranch:%s %s%s%s\n", BLUE, NEGRITO, RESET, YELLOW, list[i].name, RESET);

        while (1)
        {
            printf("%s%sO que deseja fazer [u,s,p,q,Q,?]:%s %s%s", BLUE, NEGRITO, RESET, GREEN, NEGRITO);
            scanf("%15s", ans);

            printf("%s", RESET);

            if (strcmp(ans, "Q") == 0) {
                printf("%sForcando saida!%s\n", YELLOW, RESET);
                return -1;
            }
            else if (strcmp(ans, "q") == 0)
            {
                printf("%sVoltando...%s\n", YELLOW, RESET);
                return SUCESS_EXIT;
            }
            else if (strcmp(ans, "p") == 0)
            {
                printf("%sIgnorado...%s\n", YELLOW, RESET);
                break;
            }
            else if (strcmp(ans, "?") == 0)
            {
                printf("%su  -> update (atualiza a branch local)\n", CYAN_BRIGHT);
                printf("s  -> safe update (atualiza de forma segura com stash)\n");
                if (total >= 2)
                    printf("p  -> pass (ignorar essa atualizacao)\n");
                printf("q  -> volta para opcoes anteriores\n");
                printf("Q  -> forca a parada do programa\n");
                printf("?  -> exibir essa mensagem%s\n", RESET);
                printf("\n%sBranch%s %s%s%d%s/%s%s%d%s:\n", NEGRITO, RESET, GREEN_BRIGHT, NEGRITO, i + 1, RESET, YELLOW, NEGRITO, total, RESET);
                printf("%s%sBranch:%s %s%s%s\n", BLUE, NEGRITO, RESET, YELLOW, list[i].name, RESET);
                continue;
            }
            else if (strcmp(ans, "u") == 0)
            {
                // printf("Atualizando %s/%s em %s...\n",
                    // rlist[i].origin, rlist[i].branch, list[i].name);
                break;
            }
            else if (strcmp(ans, "s") == 0)
            {
                printf("%sAtualizacao segura...%s\n", GREEN, RESET);
                break;
            }
            else if (strcmp(ans, "p") == 0)
            {
                printf("%sPassando para a proxima%s\n", WHITE, RESET);
                if (i == total) {
                    return SUCESS_EXIT;
                }
                break;
            }
            else {
                printf("%sO comando '%s' que voce digitou e desconhecido! %sTente %s%s?%s%s para obter ajuda.%s\n", RED, ans, YELLOW, GREEN, NEGRITO, RESET, YELLOW, RESET);
            }
        }
    }
}

int syncLocalBranches(remote_data *remotes,
                      int totalR,
                      local_data *locals,
                      int totalL)
{
    for (int i = 0; i < totalR; i++) {
        int idx = find_local_branch(
            locals, totalL, remotes[i].branch);

        if (idx == -1) {
            printf("%sCriando %s/%s%s\n",
                   GREEN_BRIGHT,
                   remotes[i].origin,
                   remotes[i].branch,
                   RESET);

            create_branch(
                remotes[i].origin,
                remotes[i].branch
            );
        } else {
            printf("%sAtualizando %s%s\n",
                   CYAN_BRIGHT,
                   remotes[i].branch,
                   RESET);

            safe_update_branch(
                remotes[i].origin,
                remotes[i].branch
            );
        }
    }

    return SUCESS_EXIT;
}


void branchTracking(FILE* poutfile) {
    // int i = 0;
    int index = 0;
    char c[200], letra = '\n';
    char* ans;

    poutfile = fopen(fname, "r");

    fread(c, sizeof(char), 200, poutfile);

    for (int i = 0; i < strlen(c); i++) {
        if (c[i] == letra) {
            index++;
        }
    }

    fclose(poutfile);

    printf("%s%sRastreando branches...%s\n", WHITE, NEGRITO, RESET);
    printf("branches locais encontradas: %s%s%i%s\n\n", GREEN_BRIGHT, NEGRITO, index, RESET);
}

void printErrorMessage()
{
    printf("%sErro fatal:%s %sdigite o comando abaixo para obter ajuda%s\n\n\t%s%s%s%s %s-h%s\n", RED, RESET, YELLOW, RESET, BLUE, SUB_LINHADO, pname, RESET, GREEN, RESET);
}

void printToolVersion(int int1, int int2)
{
    logo(int1, int2);
    printf("%sBranch Tracking Map%s %s(%s%s%sbtmap%s%s)%s %sv1.0.0 -> (1d05224)%s\n", MAGENTA, RESET, NEGRITO, RESET, BLUE, SUB_LINHADO, RESET, NEGRITO, RESET, YELLOW, RESET);
}

void logo(int showLogo, int showMadeBy) {
    if (showLogo == ShowLogo) {
        printf("                      %s%s____  _______ __   ___  ___     ____%s\n", YELLOW, NEGRITO, RESET);
        printf("                     %s%s/ __ )/__  __// |  // / /   |   / __ \\%s\n", YELLOW, NEGRITO, RESET);
        printf("                    %s%s/ __  |  / /  / /|_// / / /| |  / /_/ /%s\n", YELLOW, NEGRITO, RESET);
        printf("                   %s%s/ /_/ /  / /  / /   / / / ___ | / ____/%s\n", YELLOW, NEGRITO, RESET);
        printf("                  %s%s/_____/  /_/  /_/   /_/ /_/  |_|/_/%s\n", YELLOW, NEGRITO, RESET);
        printf("                         %sb t m a p%s   %ss c a n n e r%s\n", RED, RESET, BLUE, RESET);
        if (showMadeBy == ShowMadeBy)
            printf("                                               %s%sMade with %s%s<3%s %sby Artemiz %s\n", YELLOW, NEGRITO, RED, NEGRITO, RESET, YELLOW, RESET);
        printf("\n");
        printf("\n");
    }
    return;
}
