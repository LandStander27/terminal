#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#ifndef _WIN64
#include <pwd.h>
#endif

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

void human_readable(char* str, long double bytes) {
	str[0] = '\0';
	if (bytes >= 1099511627776.0) {
		sprintf(str, "%.2LF", bytes/1099511627776.0);
		strcat(str, "TB");
		return;
	}
	if (bytes >= 1073741824.0) {
		sprintf(str, "%.2LF", bytes/1073741824.0);
		strcat(str, "GB");
		return;
	}
	if (bytes >= 1048576.0) {
		sprintf(str, "%.2LF", bytes/1048576.0);
		strcat(str, "MB");
		return;
	}
	if (bytes >= 1024.0) {
		sprintf(str, "%.2LF", bytes/1024.0);
		strcat(str, "KB");
		return;
	}
	sprintf(str, "%.0LF", bytes);
	strcat(str, "B");
	return;
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

	#ifdef _WIN64
	struct dirent *entry;

	DIR *d = opendir(cwd);

	int most_size_len = 0;
	while ((entry = readdir(d)) != NULL) {
		struct stat s;
		if (stat(entry->d_name, &s) != 0) {
			printf("stat() err\n");
			closedir(d);
			return 3;
		}

		char size[50];
		human_readable(size, (long double)s.st_size);
		int len;
		if ((len = strlen(size)) > most_size_len) {
			most_size_len = len;
		}

		#ifndef _WIN64
		struct passwd* user = getpwuid(s.st_uid);
		if ((len = strlen(user->pw_name)) > most_owner_len) {
			most_owner_len = len;
		}
		#endif

	}

	closedir(d);
	d = opendir(cwd);
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

		char size[50];
		human_readable(size, (long double)s.st_size);

		#ifndef _WIN64
		struct passwd* user = getpwuid(s.st_uid);
		printf("%s  %s  ", to_str, user->pw_name);
		for (int i = 0; i < most_size_len-(int)strlen(size)+most_owner_len-(int)strlen(user->pw_name); i++) {
			printf(" ");
		}
		printf("%s  %s\n", size, entry->d_name);
		#else
		printf("%s  ", to_str);
		for (int i = 0; i < most_size_len-(int)strlen(size); i++) {
			printf(" ");
		}
		printf("%s  %s\n", size, entry->d_name);
		#endif
	}

	closedir(d);
	#else

	int most_size_len = 0;
	int most_owner_len = 0;

	struct dirent** list;
	int amount = scandir(cwd, &list, NULL, alphasort);
	if (amount == -1) {
		printf("scandir() err\n");
		return 4;
	}
	for (int i = 0; i < amount; i++) {
		struct stat s;
		if (stat(list[i]->d_name, &s) != 0) {
			printf("stat() err\n");
			free(list);
			return 3;
		}

		char size[50];
		human_readable(size, (long double)s.st_size);
		int len;
		if ((len = strlen(size)) > most_size_len) {
			most_size_len = len;
		}

		struct passwd* user = getpwuid(s.st_uid);
		if ((len = strlen(user->pw_name)) > most_owner_len) {
			most_owner_len = len;
		}
	}
	for (int i = 0; i < amount; i++) {
		struct stat s;
		if (stat(list[i]->d_name, &s) != 0) {
			printf("stat() err\n");
			free(list);
			return 3;
		}
		char mode[7];
		snprintf(mode, 7, "%o", s.st_mode);
		snprintf(mode, 7, "%s", mode+strlen(mode)-3);

		char to_str[10];
		mode_to_str(to_str, mode);

		char size[50];
		human_readable(size, (long double)s.st_size);

		struct passwd* user = getpwuid(s.st_uid);
		printf("%s  %s  ", to_str, user->pw_name);
		for (int i = 0; i < most_size_len-(int)strlen(size)+most_owner_len-(int)strlen(user->pw_name); i++) {
			printf(" ");
		}
		printf("%s  %s\n", size, list[i]->d_name);
		free(list[i]);
	}
	free(list);
	#endif
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
