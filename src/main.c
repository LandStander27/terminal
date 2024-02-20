#include "defs.h"
#include "commands.h"

extern char **environ;

static inline void path_prompt() {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		printf("getcwd() err\n");
		return;
	}
	printf(bright_blue "%s", cwd);
}

void ignore_signals(int signo) {
	return;
}

void handle_signals(int signo) {
	if (signo == SIGINT) {
		printf("\n");
		#ifndef _WIN64
		rl_on_new_line();
		rl_redisplay();
		#else
		signal(SIGINT, handle_signals);
		#endif
	}
}

bool find_binary(char* binary, char* binary_path) {
	char* path_var = getenv("PATH");
	char* var_cloned = malloc(sizeof(char)*PATH_MAX*100);
	strcpy(var_cloned, path_var);
	char* path = strtok(var_cloned, ":");
	while ((path = strtok(NULL, ":")) != NULL) {
		DIR* d;
		struct dirent* de;
		d = opendir(path);
		if (!d) {
			continue;
		}
		while ((de = readdir(d)) != NULL) {
			for (int i = 0; de->d_name[i]; i++) {
				de->d_name[i] = tolower(de->d_name[i]);
			}
			if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, binary) != 0) {
				continue;
			}
			char file_path[PATH_MAX];
			strcpy(file_path, path);
			strcat(file_path, "/");
			strcat(file_path, de->d_name);
			struct stat sb;
			if (stat(file_path, &sb) == -1) {
				continue;
			}
			if (sb.st_mode & S_IXUSR) {
				strcpy(binary_path, file_path);
				closedir(d);
				free(var_cloned);
				return true;
			}
		}
		closedir(d);
	}
	free(var_cloned);
	return false;
}

int run_binary(char* binary, char **argv, int argc, char* cmd) {
	bool is_path = false;
	for (int i = 0; binary[i]; i++) {
		#ifdef _WIN64
		if (binary[i] == '\\') {
		#else
		if (binary[i] == '/') {
		#endif
			is_path = true;
			break;
		}
	}
	for (int i = 0; binary[i]; i++) {
		binary[i] = tolower(binary[i]);
	}
	#ifndef _WIN64
	char binary_path[PATH_MAX];
	if (!is_path) {
		if (find_binary(binary, binary_path)) {
			pid_t pid;
			signal(SIGINT, ignore_signals);
			int status = posix_spawn(&pid, binary_path, NULL, NULL, argv, environ);
			if (status == 0) {
				waitpid(pid, &status, 0);
				signal(SIGINT, handle_signals);
			} else {
				return 1;
			}
		} else {
			return 1;
		}
	} else {
		pid_t pid;
		signal(SIGINT, ignore_signals);
		int status = posix_spawn(&pid, binary, NULL, NULL, argv, environ);
		if (status == 0) {
			waitpid(pid, &status, 0);
			signal(SIGINT, handle_signals);
		} else {
			return 1;
		}
	}
	#else
	proc_info pi;
	int status = windows_spawn(binary, cmd, &pi);
	if (status != 0) {
		return status;
	}
	wait_for_process(&pi);
	#endif
	return 0;
}

int run_builtin(char** args, int argc) {
	command_function a = get_command(args[0]);
	if (a != NULL) {
		return (*a)(args, argc);
	}
	return -1;
}

int main() {

	#ifndef _WIN64
	using_history();
	#endif

	signal(SIGINT, handle_signals);
	// char *a[] = { "ping", "10.250.250.5", (char*)0 };
	// run_binary("ping", a, 2, "ping 10.250.250.5");

	char **args;
	wordexp_t p;
	while (true) {

		path_prompt();
		char *inp = readline("\n" bright_green "> " reset);

		wordexp(inp, &p, 0);
		args = p.we_wordv;

		if (p.we_wordc > 0) {
			int return_code = run_builtin(args, p.we_wordc);
			if (return_code == -1) {
				if (run_binary(args[0], args, p.we_wordc, inp) != 0) {
					printf("Unknown command or executable\n");
				}
			}
		}

		printf("\n");

		// if (p.we_wordc > 0 && strcmp(args[0], "exit") == 0) {
		// 	printf("exiting...\n");
		// 	break;
		// }

		#ifndef _WIN64
		add_history(inp);
		#endif

		free(inp);

	}

	wordfree(&p);

	return 0;
}
