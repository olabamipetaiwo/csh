#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Provided by the lexer (lex.l) -- do not modify                     */
extern int  yylex(void);
extern char *yytext;

typedef enum {
    STRING       = 1,
    PIPE         = 2,
    REDIRECT_IN  = 3,
    REDIRECT_OUT = 4,
    END_OF_LINE  = 5
} token_t;

/* Command linked list                                                 */
#define MAX_ARGS 32

typedef struct command_s {
    char              *argv[MAX_ARGS]; /* NULL-terminated argument list  */
    char              *in;            /* filename for < redirect, or NULL */
    char              *out;           /* filename for > redirect, or NULL */
    struct command_s  *next;
} command_t;

command_t *first_command = NULL;
command_t *last_command  = NULL;

command_t *add_command(void) {
    command_t *cmd = calloc(1, sizeof(command_t));
    if (!first_command) {
        first_command = last_command = cmd;
    } else {
        last_command->next = cmd;
        last_command = cmd;
    }
    return cmd;
}

void free_commands(void) {
    command_t *c = first_command;
    while (c) {
        command_t *next = c->next;
        for (int i = 0; c->argv[i]; i++) free(c->argv[i]);
        free(c->in);
        free(c->out);
        free(c);
        c = next;
    }
    first_command = last_command = NULL;
}

void print_commands(void) {
    for (command_t *c = first_command; c; c = c->next) {
        printf("  cmd:");
        for (int i = 0; c->argv[i]; i++) printf(" %s", c->argv[i]);
        if (c->in)  printf("  <  %s", c->in);
        if (c->out) printf("  >  %s", c->out);
        printf("\n");
    }
}

/* Lexer helpers                                                       */
token_t lookahead;
char    lexeme[256];

/*
 * consume(): read the next token from the lexer.
 * After consume(), `lookahead` holds the token type
 * and `lexeme` holds the token text.
 */
void consume(void) {
    lookahead = yylex();
    strncpy(lexeme, yytext, sizeof(lexeme) - 1);
    lexeme[sizeof(lexeme) - 1] = '\0';
}

/*
 * match(got, want):
 *   Return 1 if got == want, 0 otherwise. Does NOT consume.
 *   Use when a token is optional (e.g. checking for '|' or '>').
 */
int match(token_t got, token_t want) {
    return got == want;
}

/*
 * expect(want):
 *   If lookahead matches want, consume and return.
 *   Otherwise print an error and exit.
 *   Use when exactly one token is valid at this point.
 *   NOTE: after expect(), lexeme holds the text of the NEXT token.
 *   Capture strdup(lexeme) BEFORE calling expect() if you need the current text.
 */
void expect(token_t want) {
    if (lookahead != want) {
        fprintf(stderr, "csh: parse error near '%s'\n", lexeme);
        exit(1);
    }
    consume();
}

/* ------------------------------------------------------------------ */
/* Parser                                                             */
/*                                                                    */
/* Grammar (from lecture):                                            */
/*   start   ::= pipes "\n"                                           */
/*   pipes   ::= command ("|" command)*                               */
/*   command ::= program in? out?                                     */
/*   program ::= STRING STRING*                                       */
/*   in      ::= "<" STRING                                           */
/*   out     ::= ">" STRING                                           */
/*                                                                    */
/* Key rule: `lexeme` always holds the text of `lookahead`.          */
/* Capture strdup(lexeme) BEFORE consuming past a token.             */
/* ------------------------------------------------------------------ */
void parse(void) {
    consume(); /* prime the lookahead */

command:
    {
        if (!match(lookahead, STRING)) {
            fprintf(stderr, "csh: expected a command name\n");
            exit(1);
        }
        command_t *cmd = add_command();

/* program: first STRING is the program name */
        cmd->argv[0] = strdup(lexeme); /* capture before consuming */
        consume();

        /*
         * TASK 2: Collect remaining arguments.
         *
         * Add a loop here that keeps consuming STRING tokens into
         * cmd->argv[1], cmd->argv[2], ... until lookahead is no
         * longer a STRING.
         *
         * Remember:
         *   - strdup(lexeme) before consume() to capture the text.
         *   - cmd->argv must end with a NULL entry (execvp requires it).
         *
         * Hint:
         *   int argc = 1;
         *   while (match(lookahead, STRING)) {
         *       cmd->argv[argc++] = strdup(lexeme);
         *       consume();
         *   }
         *   cmd->argv[argc] = NULL;
         */
        cmd->argv[1] = NULL; /* placeholder -- replace with loop in Task 2 */

/* in: redirect-in */
        /*
         * TASK 3: Handle "<" redirect.
         *
         * If lookahead is REDIRECT_IN:
         *   1. consume() past the "<".
         *   2. strdup(lexeme) into cmd->in  (this is the filename).
         *   3. consume() past the filename.
         *
         * No helper needed -- just match, consume, strdup, consume.
         */

/* out: redirect-out (provided -- do not modify) */
        if (match(lookahead, REDIRECT_OUT)) {
            consume();                       /* past ">"        */
            cmd->out = strdup(lexeme);       /* capture filename */
            expect(STRING);                  /* validates + advances */
        }

/* pipe: if "|" follows, loop back for the next command */
        if (match(lookahead, PIPE)) {
            consume();
            goto command;
        }
    }

    if (!match(lookahead, END_OF_LINE)) {
        fprintf(stderr, "csh: unexpected token '%s'\n", lexeme);
        exit(1);
    }
    /* Do not consume past END_OF_LINE. */
}

/* Interpreter                                                        */
void run_commands(void) {
    if (!first_command) return;

    enum { READ = 0, WRITE = 1 };

    /* ---- Single command (provided -- do not modify) ---- */
    if (first_command == last_command) {
        command_t *cmd = first_command;
        switch (fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            if (cmd->in) {
                int fd = open(cmd->in, O_RDONLY);
                if (-1 == fd) { perror("open"); _exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd->out) {
                int fd = open(cmd->out,
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (-1 == fd) { perror("open"); _exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(cmd->argv[0], cmd->argv);
            perror("execvp");
            _exit(1);
        default:
            while (wait(NULL) > 0);
            break;
        }
        return;
    }

    /* ---- TASK 4: Two commands connected by a pipe ---- */
    /*
     * Connect first_command | last_command using a real Unix pipe.
     *
     * Steps:
     *
     *   1. int pipefd[2]; pipe(pipefd);
     *
     *   2. Fork child 1  (left side -- runs first_command):
     *        close(pipefd[READ]);
     *        dup2(pipefd[WRITE], STDOUT_FILENO);
     *        close(pipefd[WRITE]);
     *        execvp(first_command->argv[0], first_command->argv);
     *        _exit(1);
     *
     *   3. Fork child 2  (right side -- runs last_command):
     *        close(pipefd[WRITE]);
     *        dup2(pipefd[READ], STDIN_FILENO);
     *        close(pipefd[READ]);
     *        execvp(last_command->argv[0], last_command->argv);
     *        _exit(1);
     *
     *   4. Parent:
     *        close(pipefd[READ]);
     *        close(pipefd[WRITE]);
     *        wait(NULL);
     *        wait(NULL);
     *
     * Once it works, comment out the two parent close() calls and run
     * a piped command again. What happens? Why does it hang?
     */
}

/* Entry point                                                        */
int main(void) {
    printf("csh> ");
    fflush(stdout);

    parse();

    /* Uncomment to debug parser output before running commands: */
    /* print_commands(); return 0; */

    run_commands();
    free_commands();

    return 0;
}
