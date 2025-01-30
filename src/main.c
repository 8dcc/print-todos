
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LENGTH(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define ENSURE_ALLOCATION(PTR)                                                 \
    do {                                                                       \
        if ((PTR) == NULL) {                                                   \
            fprintf(stderr,                                                    \
                    "Error allocating '%s': %s\n",                             \
                    #PTR,                                                      \
                    strerror(errno));                                          \
            return NULL;                                                       \
        }                                                                      \
    } while (0)

/*----------------------------------------------------------------------------*/

static const char* todos[] = {
    "TODO", "HACK", "REVIEW", "FIXME", "DELME", "DEBUG",
};

/*----------------------------------------------------------------------------*/

static char* get_next_comment(FILE* fp) {
    size_t comment_sz  = 100;
    size_t comment_pos = 0;
    char* comment      = malloc(comment_sz);
    ENSURE_ALLOCATION(comment);

    enum {
        COMMENT_NONE,
        COMMENT_MULTI,     /* ... */
        COMMENT_SINGLE,    // ...
    } in_comment = COMMENT_NONE;

    for (int c = 0, last_c = 0; (c = fgetc(fp)) != EOF; last_c = c) {
        if (comment_pos + 2 >= comment_sz) {
            comment_sz *= 2;
            comment = realloc(comment, comment_sz);
            ENSURE_ALLOCATION(comment);
        }

        if (c == '\\' || last_c == '\\')
            continue;

        if (in_comment != COMMENT_NONE) {
            /* Inside a comment */
            comment[comment_pos++] = c;

            if ((in_comment == COMMENT_SINGLE && c == '\n') ||
                (in_comment == COMMENT_MULTI && last_c == '*' && c == '/')) {
                /* Comment end */
                if (in_comment == COMMENT_MULTI)
                    comment[comment_pos++] = '\n';
                break;
            }
        } else if (last_c == '/' && (c == '/' || c == '*')) {
            /* Comment start */
            comment[comment_pos++] = last_c;
            comment[comment_pos++] = c;
            in_comment = (c == '/') ? COMMENT_SINGLE : COMMENT_MULTI;
        }
    }

    if (comment_pos == 0) {
        free(comment);
        return NULL;
    } else {
        comment[comment_pos] = '\0';
        return comment;
    }
}

static bool has_todo(const char* comment) {
    for (size_t i = 0; i < LENGTH(todos); i++)
        if (strstr(comment, todos[i]) != NULL)
            return true;

    return false;
}

static bool print_file_todos(FILE* dst, FILE* src) {
    char* comment;
    while ((comment = get_next_comment(src)) != NULL) {
        if (has_todo(comment))
            fprintf(dst, "%s", comment);

        free(comment);
    }

    return true;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr,
                "Not enough arguments.\n"
                "Usage: %s [FILE...]\n",
                argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char* filename = argv[i];

        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
            fprintf(stderr,
                    "Could not open '%s': %s\n",
                    filename,
                    strerror(errno));
            return 1;
        }

        if (!print_file_todos(stdout, fp))
            return 1;

        fclose(fp);
    }

    return 0;
}
