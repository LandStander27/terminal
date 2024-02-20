#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef _WIN64
#include <wordexp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <spawn.h>
#include <sys/wait.h>
#else
#include <windows.h>
#include <stringapiset.h>
#include <shellapi.h>
#endif

#ifdef _WIN64
typedef struct {
	char **we_wordv;
	int we_wordc;
} wordexp_t;

int wordexp(char* cmd, wordexp_t* out, int flags) {

	int required_size = MultiByteToWideChar(CP_UTF8, 0, cmd, (int)strlen(cmd), NULL, 0);

	wchar_t wide_str[required_size+10];
	MultiByteToWideChar(CP_UTF8, 0, cmd, (int)strlen(cmd), wide_str, required_size);

	int argc;
	wchar_t **argv;
	argv = CommandLineToArgvW(wide_str, &argc);

	out->we_wordv = malloc(sizeof(char*)*(argc+1));
	for (int i = 0; i < argc; i++) {
		int required_size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
		char normal[required_size+10];
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, normal, required_size, NULL, NULL);
		out->we_wordv[i] = malloc(sizeof(char)*255);
		strcpy(out->we_wordv[i], normal);
	}
	for (int i = 0; i < (int)strlen(out->we_wordv[argc-1]); i++) {
		if (out->we_wordv[argc-1][i] < 0) {
			out->we_wordv[argc-1][i] = '\0';
			break;
		}
	}
	out->we_wordv[argc] = '\0';
	out->we_wordc = argc;

	return 0;
}

void wordfree(wordexp_t* in) {
	for (int i = 0; i < in->we_wordc; i++) {
		free(in->we_wordv[i]);
	}
	free(in->we_wordv);
}

int getline(char **lineptr, size_t *n, FILE *stream) {
	char *bufptr = NULL;
    char *p = bufptr;
    int size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

char* readline(char* prompt) {
	printf("%s", prompt);
	size_t line_max = 255;
	char *inp = (char*)malloc(sizeof(char)*line_max);

	size_t bytes_read = getline(&inp, &line_max, stdin);
	inp[bytes_read-1] = '\0';

	return inp;
}

typedef STARTUPINFO startup_info;
typedef PROCESS_INFORMATION proc_info;

int windows_spawn(char* binary, char* cmd, proc_info* pi) {
	startup_info si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(pi, sizeof(*pi));

	int code = CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi);
	if (code) {
		return 0;
	}
	return 1;
}

void wait_for_process(proc_info* pi) {
	WaitForSingleObject(pi->hProcess, INFINITE);
	CloseHandle(pi->hProcess);
	CloseHandle(pi->hThread);
}

#endif

#define red "\x1b[31m"
#define cyan "\x1b[36m"

#define bright_blue "\x1b[38;5;39m"
#define bright_green "\x1b[38;5;46m"

#define dim "\x1b[2m"
#define bold "\x1b[1m"

#define reset "\x1b[0m"

#endif