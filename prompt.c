#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

int main(int argc, char **argv)
{
    // Print Version and Exit Instruction
    puts("Lispy version 0.0.0.0.1");
    puts("Press Ctrl+C to exit\n");

    // Infinite loop
    while (1)
    {
        // Output the prompt and get input
        char *input = readline("lispy> ");

        // Add input to history (retrieved with up and down arrows)
        add_history(input);

        // Echo back the input
        printf("You typed %s\n", input);

        // Free retrieved input
        free(input);
    }

    return 0;
}