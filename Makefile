debug:
	mkdir -p bin
	gcc -o ./bin/terminal src/main.c src/commands.c -Wextra -lreadline -Iinclude -g

build:
	mkdir -p bin
	gcc -o ./bin/terminal src/main.c src/commands.c -Wextra -lreadline -Iinclude

run:
	mkdir -p bin
	gcc -o ./bin/terminal src/main.c src/commands.c -Wextra -lreadline -Iinclude
	./bin/terminal

windows:
	mkdir -p bin
	gcc.exe -o ./bin/terminal.exe src/main.c src/commands.c -Wextra -Iinclude

windows-run:
	mkdir -p bin
	gcc.exe -o ./bin/terminal.exe src/main.c src/commands.c -Wextra -Iinclude
	./bin/terminal.exe

clean:
	rm -rf bin

deps:
	sudo apt install -y git make gcc libreadline-dev