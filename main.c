#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#define ZISH_NUM_KAWAII_SMILEYS 6
static char *kawaii_smileys[ZISH_NUM_KAWAII_SMILEYS] = {
    "(▰˘◡˘▰)",
    "♥‿♥",
    "(✿ ♥‿♥)",
    ".ʕʘ‿ʘʔ.",
    "◎[▪‿▪]◎",
    "≧◡≦"
};

enum status_code {
    STAT_NORMAL,
    STAT_EXIT
};

static void zish_repl(void);
// static char *zish_get_line(void);
static char **zish_split_line(char *line);
static enum status_code zish_exec(char **args);
static enum status_code zish_launch(char **args);

static enum status_code zish_cd(char **args);
static enum status_code zish_help(char **args);
static enum status_code zish_exit(char **args);

static void zish_register_interrupt_handler(void);
static void zish_interrupt_handler(int signo);

#define ZISH_NUM_BUILTINS 3
static char *builtin_str[ZISH_NUM_BUILTINS] = {
    "cd",
    "help",
    "exit"
};

static enum status_code (*builtin_func[ZISH_NUM_BUILTINS])(char **) = {
    &zish_cd,
    &zish_help,
    &zish_exit
};

int main(void)
{
    // Init
    // zish_register_interrupt_handler();

    srand(time(NULL));

    // REPL
    zish_repl();

    // Cleanup

    return EXIT_SUCCESS;
}

static void zish_repl(void)
{
    char  *line = NULL;
    char **args = NULL;
    enum status_code status = STAT_NORMAL;

    do {
        // Get input
        int kawaii_smiley_index = rand() % ZISH_NUM_KAWAII_SMILEYS;
        char prompt[70 + sizeof(kawaii_smileys[kawaii_smiley_index])];

        sprintf(
            prompt,
            "\033[38;5;057mAwaiting your command, senpai. \033[38;5;197m%s \033[38;5;255m",
            kawaii_smileys[kawaii_smiley_index]
        );

        line = readline(prompt);
        if (!line) {
            perror("zish");
            continue;
        }

        args = zish_split_line(line);

        status = zish_exec(args);

        free(line);
        free(args);
    } while (status != STAT_EXIT);
}

/*
#define ZISH_LINE_BUFSIZE 256
static char *zish_get_line(void)
{
    char  *line   = malloc(ZISH_LINE_BUFSIZE);
    char  *linep  = line;
    size_t lenmax = ZISH_LINE_BUFSIZE;
    size_t len    = lenmax;
    int    c;

    if (!line)
        return NULL;

    for (;;) {
        c = fgetc(stdin);
        if (c == EOF)
            break;

        if (c == '\033' && fgetc(stdin) == '[') {
            switch(fgetc(stdin)) {
                case 'A':
                    // Arrow up
                    printf("Up\n");
                    break;
                case 'B':
                    // Arrow down
                    printf("Down\n");
                    break;
                case 'C':
                    // Arrow right
                    printf("Right\n");
                    break;
                case 'D':
                    // Arrow left
                    printf("Left\n");
                    break;
            }
            continue;
        }

        if (--len == 0) {
            len = lenmax;
            lenmax *= 3;
            lenmax /= 2;
            char *linen = realloc(linep, lenmax);

            if (!linen) {
                free(linep);
                return NULL;
            }
            line  = linen + (line - linep);
            linep = linen;
        }

        if ((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}
*/

#define ZISH_TOKEN_BUFSIZE 64
#define ZISH_TOKEN_DELIMS " \t\n\r"
static char **zish_split_line(char *line)
{
    int    bufsize = ZISH_TOKEN_BUFSIZE;
    int    pos     = 0;
    char **tokens  = malloc(bufsize * sizeof(*tokens));
    char  *token   = NULL;

    if (!tokens) {
        perror("zish");
    }

    token = strtok(line, ZISH_TOKEN_DELIMS);
    while (token) {
        tokens[pos] = token;
        ++pos;

        if (pos >= bufsize) {
            bufsize *= 3;
            bufsize /= 2;
            tokens   = realloc(tokens, bufsize * sizeof(*tokens));

            if (!tokens) {
                perror("zish");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, ZISH_TOKEN_DELIMS);
    }

    tokens[pos] = NULL;
    return tokens;
}

static enum status_code zish_exec(char **args)
{
    if (!args[0]) {
        // Empty command
        return STAT_NORMAL;
    }

    for (size_t i = 0; i < ZISH_NUM_BUILTINS; ++i) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return zish_launch(args);
}

static enum status_code zish_launch(char **args)
{
    pid_t pid;

    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("zish");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("zish");
    } else {
        // Parent process
        do {
           waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return STAT_NORMAL;
}

/*
  Builtins
 */
static enum status_code zish_cd(char **args)
{
    if (!args[1]) {
        fprintf(stderr, "zish: expected argument to `cd`\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("zish");
        }
    }

    return STAT_NORMAL;
}

static enum status_code zish_help(char **args)
{
    printf(
        "zish is a shell\n"
        "(c) Niclas Meyer\n\n"
        "Twype the pwogwam name and then pwess enter, onii-chan. (^._.^)~\n"
        "Builtwins:\n"
    );

    for (size_t i = 0; i < ZISH_NUM_BUILTINS; ++i) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use `man` for more inwos on other pwogwams.\n");

    return STAT_NORMAL;
}

static enum status_code zish_exit(char **args)
{
    printf("Sayounara, Onii-chan! (._.)\n");
    return STAT_EXIT;
}

static void zish_register_interrupt_handler(void)
{
    if (signal(SIGINT, &zish_interrupt_handler) == SIG_ERR) {
        perror("zish");
        exit(EXIT_FAILURE);
    }
}

static void zish_interrupt_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\n\033[38;5;057mIf you wanna go, try `exit`, onii-chan.\033[38;5;255m\n");
    }

    zish_register_interrupt_handler();
}

