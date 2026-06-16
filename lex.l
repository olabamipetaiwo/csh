%{
#include <string.h>

typedef enum {
    STRING       = 1,
    PIPE         = 2,
    REDIRECT_IN  = 3,
    REDIRECT_OUT = 4,
    END_OF_LINE  = 5
} token_t;
%}

%%
[ \t]+          { /* skip whitespace */ }
\|              { return PIPE; }
"<"             { return REDIRECT_IN; }
">"             { return REDIRECT_OUT; }
\n              { return END_OF_LINE; }
[^ \t\n|<>]+   { return STRING; }
%%

int yywrap(void) { return 1; }
