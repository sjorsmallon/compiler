#include "types.h"
#include "array.h"
// needed to stop whining about fopen being deprecated.
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>

void process_line(char *line, size_t length);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = 1024;
    char *line = malloc(buffer_size);
    if (line == NULL) {
        perror("Unable to allocate buffer");
        exit(EXIT_FAILURE);
    }

    while (1) {
        size_t len = 0;
        int c;

        while ((c = fgetc(file)) != EOF && c != '\n') {
            if (len + 1 >= buffer_size) {
                buffer_size *= 2;
                char *new_line = realloc(line, buffer_size);
                if (new_line == NULL) {
                    perror("Unable to reallocate buffer");
                    free(line);
                    exit(EXIT_FAILURE);
                }
                line = new_line;
            }
            line[len++] = c;
        }

        if (ferror(file)) {
            perror("Error reading file");
            free(line);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (len > 0 || c == '\n') {
            line[len] = '\0';
            process_line(line, len);
        }

        if (c == EOF) {
            break;
        }
    }

    free(line);
    fclose(file);
    return 0;
}

void process_line(char *line, size_t length) {

    for (size_t idx = 0; idx < length; ++idx) {
        // what kind of character are we dealing with? just a character? a number?
        char c = line[idx];

        switch (c)
        {
            case ';' :
            {
                // end of an expression?
            }
            case '=' :
            {

            } 

            default:
            {
                if (is_alpha)
                {

                }
                if (is_numerical())
                {
                    
                }
            };
        }

    }

}