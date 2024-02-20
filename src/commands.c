#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#define CD 5863276
#define EXIT 6385204799
#define LS 5863588

typedef int (*command_function)(char**, int);

void mode_to_str(char* str, char* mode) {

	str[0] = '\0';

	for (int i = 0; i < 3; i++) {
		char temp[2] = { mode[i], '\0' };
		int num = atoi(temp);
		char current[4] = "---";
		if (num >= 4) {
			num -= 4;
			current[0] = 'r';
		}
		if (num >= 2) {
			num -= 2;
			current[1] = 'w';
		}
		if (num >= 1) {
			num -= 1;
			current[2] = 'x';
		}
		strcat(str, current);
	}

}

long long hash(const char *str) {
	long long hash = 5381;
	int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

int cd(char** argv, int argc) {
	if (argc == 1) {
		printf("Incorrect usage\ncd [directory]\n");
		return 1;
	}

	struct stat s;
	if (stat(argv[1], &s) != 0 || !S_ISDIR(s.st_mode)) {
		printf("Invalid directory\n");
		return 2;
	}

	if (chdir(argv[1]) != 0) {
		printf("chdir() error\n");
		return 3;
	}

	return 0;

}

int ls(char** argv, int argc) {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		printf("getcwd() err\n");
		return 2;
	}

	DIR *d = opendir(cwd);

	struct dirent *entry;
	while ((entry = readdir(d)) != NULL) {
		struct stat s;
		if (stat(entry->d_name, &s) != 0) {
			printf("stat() err\n");
			closedir(d);
			return 3;
		}
		char mode[7];
		snprintf(mode, 7, "%o", s.st_mode);
		snprintf(mode, 7, "%s", mode+strlen(mode)-3);

		char to_str[10];
		mode_to_str(to_str, mode);

		printf("%s, %s\n", to_str, entry->d_name);
	}

	closedir(d);
}

int shell_exit(char** argv, int argc) {
	exit(0);
}

command_function get_command(char* cmd) {
	switch (hash(cmd)) {
		case CD:
			return &cd;
		case EXIT:
			return &shell_exit;
		case LS:
			return &ls;
		default:
			return NULL;
	}
}
