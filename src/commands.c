#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#ifndef _WIN64
#include <pwd.h>
#endif

#define blue "\x1b[34m"
#define green "\x1b[32m"
#define reset "\x1b[0m"

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
	str[9] = '\0';

}

void human_readable(char* str, long long bytes) {
	str[0] = '\0';
	if (bytes >= (long long)1099511627776) {
		sprintf(str, "%.2LF", bytes/(long double)1099511627776.0);
		strcat(str, "TB");
		return;
	}
	if (bytes >= (long long)1073741824) {
		sprintf(str, "%.2LF", bytes/(long double)1073741824.0);
		strcat(str, "GB");
		return;
	}
	if (bytes >= (long long)1048576) {
		sprintf(str, "%.2LF", bytes/(long double)1048576.0);
		strcat(str, "MB");
		return;
	}
	if (bytes >= (long long)1024) {
		sprintf(str, "%.2LF", bytes/(long double)1024.0);
		strcat(str, "KB");
		return;
	}
	sprintf(str, "%d", bytes);
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
			printf("stat() err: %s\n", entry->d_name);
			continue;
		}

		char size[50];
		human_readable(size, (long double)s.st_size);
		int len;
		if ((len = strlen(size)) > most_size_len) {
			most_size_len = len;
		}

	}

	closedir(d);
	d = opendir(cwd);
	while ((entry = readdir(d)) != NULL) {
		struct stat s;
		if (stat(entry->d_name, &s) != 0) {
			printf("stat() err: %s\n", entry->d_name);
			continue;
		}
		char mode[7];
		snprintf(mode, 7, "%o", s.st_mode);
		snprintf(mode, 7, "%s", mode+strlen(mode)-3);

		char to_str[10];
		mode_to_str(to_str, mode);

		char size[50];
		human_readable(size, (long double)s.st_size);

		char date[20];
		strftime(date, 20, "%b %d %H:%M", localtime(&(s.st_mtime)));

		printf("%s  ", to_str);
		for (int i = 0; i < most_size_len-(int)strlen(size); i++) {
			printf(" ");
		}
		if (S_ISDIR(s.st_mode)) {
			printf("%s  %s  " blue "%s" reset "\n", size, date, entry->d_name);
		} else if (s.st_mode & S_IXUSR) {
			printf("%s  %s  " green "%s" reset "\n", size, date, entry->d_name);
		} else {
			printf("%s  %s  %s\n", size, date, entry->d_name);
		}
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
	struct stat s;

	char size[50];
	char (*sizes)[50] = malloc((amount+1)*sizeof(char[50]));
	int sizes_len = 0;

	char (*modes)[10] = malloc((amount+1)*sizeof(char[10]));
	int modes_len = 0;

	struct passwd** users = malloc((amount+1)*sizeof(struct passwd*));
	int users_len = 0;

	int len;
	struct passwd* user;
	char mode[7];
	char to_str[10];
	for (int i = 0; i < amount; i++) {
		if (stat(list[i]->d_name, &s) != 0) {
			printf("stat() err: %s\n", list[i]->d_name);
			sizes_len++;
			modes_len++;
			users_len++;
			continue;
		}

		snprintf(mode, 7, "%o", s.st_mode);
		snprintf(mode, 7, "%s", mode+strlen(mode)-3);

		mode_to_str(to_str, mode);
		strcpy(modes[modes_len], to_str);
		modes_len++;

		human_readable(size, s.st_size);
		strcpy(sizes[sizes_len], size);
		sizes_len++;
		if ((len = strlen(size)) > most_size_len) {
			most_size_len = len;
		}

		user = getpwuid(s.st_uid);
		users[users_len] = user;
		users_len++;
		if ((len = strlen(user->pw_name)) > most_owner_len) {
			most_owner_len = len;
		}
	}

	for (int i = 0; i < amount; i++) {
		if (strcmp(list[i]->d_name, ".") == 0 || strcmp(list[i]->d_name, "..") == 0) {
			continue;
		}
		if (stat(list[i]->d_name, &s) != 0) {
			printf("stat() err: %s\n", list[i]->d_name);
			continue;
		}

		printf("%s  %s  ", modes[i], users[i]->pw_name);
		for (int j = 0; j < most_size_len-(int)strlen(sizes[i]); j++) {
			printf(" ");
		}

		char date[20];
		strftime(date, 20, "%b %d %H:%M", localtime(&(s.st_mtime)));
		if (S_ISDIR(s.st_mode)) {
			printf("%s  %s  " blue "%s" reset "\n", sizes[i], date, list[i]->d_name);
		} else if (s.st_mode & S_IXUSR) {
			printf("%s  %s  " green "%s" reset "\n", sizes[i], date, list[i]->d_name);
		} else {
			printf("%s  %s  %s\n", sizes[i], date, list[i]->d_name);
		}
		free(list[i]);
	}
	free(sizes);
	free(modes);
	free(users);
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
