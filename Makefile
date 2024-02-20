debug:
	mkdir -p bin
	gcc -o ./bin/main src/main.c src/commands.c -Wextra -lreadline -Iinclude -g

build:
	mkdir -p bin
	gcc -o ./bin/main src/main.c src/commands.c -Wextra -lreadline -Iinclude

run:
	mkdir -p bin
	gcc -o ./bin/main src/main.c src/commands.c -Wextra -lreadline -Iinclude
	./bin/main

windows:
	mkdir -p bin
	gcc.exe -o ./bin/main.exe src/main.c src/commands.c -Wextra -Iinclude

windows-run:
	mkdir -p bin
	gcc.exe -o ./bin/main.exe src/main.c src/commands.c -Wextra -Iinclude
	./bin/main.exe

clean:
	rm -rf bin