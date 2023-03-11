use std::fs::DirEntry;
use shellwords;
use colored::Colorize;
use std::process::exit;
use std::path::Path;
use chrono::prelude::*;
use std::env;
use std::io;
use std::io::Write;
use ctrlc;
use std::fs;
use sysinfo::{ProcessExt, System, SystemExt};
use sublime_fuzzy::best_match;
use clearscreen;
use dircpy::*;
use std::fs::File;

struct Command {
	name: String,
	func: fn(Vec<String>) -> Result<String, String>,
	help: String,
	arg_count: Vec<usize>
}

fn read() -> String {
	let mut buffer: String = String::new();
	let stdin: io::Stdin = io::stdin();
	stdin.read_line(&mut buffer).unwrap();
	return buffer.trim().to_string();
}

fn current_dir() -> String {
	return env::current_dir().unwrap().display().to_string();
}

fn main() {

	let mut commands: Vec<Command> = Vec::new();
	commands.push( Command {
		name: "exit".to_string(),
		help: "Exits the terminal".to_string(),
		arg_count: Vec::from([1]),
		func: |_args: Vec<String>| {
			exit(0);
			// Err("Failed to exit, this should never happen".to_string())
		}
	});

	commands.push( Command {
		name: "ls".to_string(),
		help: "Lists current directory: ls {path}".to_string(),
		arg_count: Vec::from([1, 2]),
		func: |args: Vec<String>| {

			let files: Vec<DirEntry>;
			if args.len() == 2 {
				if !Path::new(&args[1].clone()).is_dir() {
					return Err("Invalid directory".to_string())
				}
				files = fs::read_dir(args[1].clone()).unwrap().collect::<Result<Vec<_>, io::Error>>().unwrap();
			} else {
				files = fs::read_dir(".").unwrap().collect::<Result<Vec<_>, io::Error>>().unwrap();
			}
			

			let mut sizes: Vec<u64> = Vec::new();
			for file in &files {
				sizes.push(file.clone().metadata().unwrap().len());
			}

			let mut max_size: usize = 0;
			for i in sizes {
				let size: usize = i.to_string().len();
				if size > max_size {
					max_size = size;
				}
			}
			if max_size < " <DIR>".len() {
				max_size = " <DIR>".len();
			}

			for file in files {
				let t: DateTime<Local> = file.metadata().unwrap().modified().unwrap().into();
				if file.metadata().unwrap().is_file() {
					println!("{} {}{}{}", t.format("%b %d %H:%M"), file.metadata().unwrap().len(), " ".repeat(max_size+1-file.metadata().unwrap().len().to_string().len()), file.path().display().to_string().split("\\").collect::<Vec<&str>>().last().copied().unwrap());
				} else if file.metadata().unwrap().is_dir() {
					println!("{} <DIR>{}{}", t.format("%b %d %H:%M"), " ".repeat(max_size-4), file.path().display().to_string().split("\\").collect::<Vec<&str>>().last().copied().unwrap().truecolor(21, 71, 220));
				}
				
			}

			return Ok("".to_string())
		}
	});

	commands.push( Command {
		name: "cd".to_string(),
		help: "Changes the current directory: cd {path}".to_string(),
		arg_count: Vec::from([2]),
		func: |args: Vec<String>| {
			let new = Path::new(args[1].as_str());
			if !(env::set_current_dir(&new).is_ok()) {
				return Err("Directory does not exist or access is denied".to_string());
			}
			Ok("".to_string())
		}
	});

	commands.push( Command {
		name: "tasklist".to_string(),
		help: "Lists current tasks {searchterm}".to_string(),
		arg_count: Vec::from([1, 2]),
		func: |args: Vec<String>| {
			let mut s = System::new();
			s.refresh_processes();

			let mut values: Vec<Vec<String>> = Vec::new();

			for (pid, proc) in s.processes() {
				values.push(Vec::from([pid.to_string(), proc.name().to_string(), if proc.exe().to_str().unwrap() != "" {String::from(proc.exe().file_name().unwrap().to_str().unwrap())} else {"".to_string()}, (proc.cpu_usage()*100.0).round().to_string()+"%"]));
			}
			
			let mut max_length: [usize; 3] = [0, 0, 0];
			for i in values.clone() {
				if i[0].len() > max_length[0] {
					max_length[0] = i[0].len();
				}
				if i[1].len() > max_length[1] {
					max_length[1] = i[1].len();
				}
				if i[2].len() > max_length[2] {
					max_length[2] = i[2].len();
				}
			}

			println!("PID{}NAME{}PATH{}CPU", " ".repeat(max_length[0]-3+3), " ".repeat(max_length[1]-4+3), " ".repeat(max_length[2]-4+3));
			println!("{} {} {} {}", "=".repeat(max_length[0]+2), "=".repeat(max_length[1]+2), "=".repeat(max_length[2]+2), "======");

			if args.len() == 1 {
				for i in values {
					println!("{}{}{}{}{}{}{}", i[0], " ".repeat(max_length[0]-i[0].len()+3), i[1], " ".repeat(max_length[1]-i[1].len()+3), i[2], " ".repeat(max_length[2]-i[2].len()+3), i[3]);
				}
			} else {
				for i in values {
					let score = best_match(args[1].as_str(), i[1].as_str()).is_some() || best_match(args[1].as_str(), i[2].as_str()).is_some();
					if score {
						println!("{}{}{}{}{}{}{}", i[0], " ".repeat(max_length[0]-i[0].len()+3), i[1], " ".repeat(max_length[1]-i[1].len()+3), i[2], " ".repeat(max_length[2]-i[2].len()+3), i[3]);
					}
				}
			}


			return Ok("".to_string())
		}
	});

	commands.push( Command {
		name: "taskkill".to_string(),
		help: "Kill a task: taskkill [-pid/-name] {process}".to_string(),
		arg_count: Vec::from([3]),
		func: |args: Vec<String>| {

			if !(["-pid", "-name"].contains(&args[1].to_lowercase().as_str())) {
				return Err("Invalid argument at place 1 (".to_string()+args[1].as_str()+")")
			}

			let mut s = System::new();
			s.refresh_processes();

			let mut exists: bool = false;
			for (pid, proc) in s.processes() {
				if args[1].to_lowercase() == "-pid" {
					if pid.to_string() == args[2] {
						proc.kill();
						exists = true;
						break;
					}
				} else if args[1].to_lowercase() == "-name" {
					if proc.name() == args[2] || proc.exe().to_str().unwrap() == args[2] {
						exists = true;
						proc.kill();
					}
				}
			}
			if !exists {
				return Err("Process does not exist".to_string())
			}

			return Ok("Killed process ".to_string() + args[2].as_str())
		}
	});

	commands.push( Command {
		name: "clear".to_string(),
		help: "Clears the terminal".to_string(),
		arg_count: Vec::from([1]),
		func: |_args: Vec<String>| {
			let r: Result<(), clearscreen::Error> = clearscreen::clear();
			match r {
				Ok(_) => return Ok("".to_string()),
				Err(_) => return Err("Could not clear screen".to_string())
			}
			// return Ok("".to_string())
		}
	});

	commands.push( Command {
		name: "rm".to_string(),
		help: "Deletes a file: rm {file}".to_string(),
		arg_count: Vec::from([2]),
		func: |args: Vec<String>| {
			if !Path::new(&args[1].clone()).is_file() {
				return Err("Invalid file".to_string())
			}
			let r: Result<(), io::Error> = fs::remove_file(args[1].as_str());
			match r {
				Ok(_) => return Ok("Deleted file".to_string()),
				Err(_) => return Err("Could not delete file".to_string())
			}
		}
	});
	commands.push( Command {
		name: "rmdir".to_string(),
		help: "Deletes a directory: rmdir {dir}".to_string(),
		arg_count: Vec::from([2]),
		func: |args: Vec<String>| {
			if !Path::new(&args[1].clone()).is_dir() {
				return Err("Invalid directory".to_string())
			}
			let r: Result<(), io::Error> = fs::remove_dir_all(args[1].as_str());
			match r {
				Ok(_) => return Ok("Deleted directory".to_string()),
				Err(_) => return Err("Could not delete directory".to_string())
			}
		}
	});
	commands.push( Command {
		name: "mkdir".to_string(),
		help: "Makes a directory: mkdir {dir}".to_string(),
		arg_count: Vec::from([2]),
		func: |args: Vec<String>| {
			if Path::new(&args[1].clone()).is_dir() {
				return Err("Directory already exists".to_string())
			}
			let r: Result<(), io::Error> = fs::create_dir(args[1].as_str());
			match r {
				Ok(_) => return Ok("Created directory".to_string()),
				Err(_) => return Err("Could not create directory".to_string())
			}
		}
	});

	commands.push( Command {
		name: "cp".to_string(),
		help: "Copy files and directories: cp {source} {target}".to_string(),
		arg_count: Vec::from([3]),
		func: |args: Vec<String>| {
			if Path::new(&args[1].clone()).is_dir() {
				let r: Result<(), io::Error> = CopyBuilder::new(args[1].as_str(), args[2].as_str()).run();
				match r {
					Ok(_) => return Ok("Copied source".to_string()),
					Err(e) => return Err("Could not copy source: ".to_string() + e.to_string().as_str())
				}
			} else if Path::new(&args[1].clone()).is_file() {
				let r: Result<u64, io::Error> = fs::copy(args[1].as_str(), args[2].as_str());
				match r {
					Ok(_) => return Ok("Copied source".to_string()),
					Err(_) => return Err("Could not copy source".to_string())
				}
			} else {
				return Err("Source does not exist".to_string())
			}

		}
	});

	commands.push( Command {
		name: "mv".to_string(),
		help: "Move files and directories: mv {source} {target}".to_string(),
		arg_count: Vec::from([3]),
		func: |args: Vec<String>| {

			let r: Result<(), io::Error> = fs::rename(args[1].as_str(), args[2].as_str());
			match r {
				Ok(_) => return Ok("Moved source".to_string()),
				Err(e) => return Err("Could not move source: ".to_string() + e.to_string().as_str())
			}

		}
	});

	commands.push( Command {
		name: "touch".to_string(),
		help: "Create a file: touch {file}".to_string(),
		arg_count: Vec::from([2]),
		func: |args: Vec<String>| {
			if Path::new(&args[1].clone()).is_file() {
				return Err("File already exists".to_string())
			}
			let r: Result<File, io::Error> = File::create(args[1].as_str());
			match r {
				Ok(_) => return Ok("Created directory".to_string()),
				Err(_) => return Err("Could not create directory".to_string())
			}
		}
	});

	let help_command = |c: String| {

		if c == "" {
			for i in 0..commands.len() {
				println!("{}: {}", commands[i].name, commands[i].help);
			}
		} else {
			let mut cmd: Vec<[String; 2]> = Vec::new();
			for i in 0..commands.len() {
				// printl;n!("{}: {}", commands[i].name, commands[i].help);
				cmd.push([commands[i].name.clone(), commands[i].help.clone()]);
			}
			for i in cmd {
				let score = best_match(c.as_str(), i[0].as_str()).is_some() || best_match(c.as_str(), i[1].as_str()).is_some();
				if score {
					println!("{}: {}", i[0], i[1]);
				}
			}
		}


	};

	ctrlc::set_handler(move || {
		
	}).unwrap();

	loop {
		print!("{}$ ", current_dir().truecolor(21, 71, 220));
		std::io::stdout().flush().unwrap();

		let input: String = read();
		
		let res = shellwords::split(input.as_str());
		
		match res {
			Ok(_) => (),
			Err(e) => {
				println!("Parse error: {}", e);
				continue;
			}
		}

		let args: Vec<String> = res.unwrap();
		if args.len() == 0 {
			continue;
		}

		for i in 0..commands.len() {
			if args[0].to_lowercase() == "help" {
				help_command(if args.len() != 1 {args[1].to_lowercase()} else {"".to_string()});
				break;
			}
			if commands[i].name == args[0].to_lowercase() {
				let mut valid: bool = false;
				for x in 0..commands[i].arg_count.len() {
					if args.len() == commands[i].arg_count[x] {
						valid = true;
						break;
					}
				}
				if !valid {
					println!("Error: Wrong amount of arguments");
					break;
				}
				let r: Result<String, String> = (commands[i].func)(args.clone());
				match r {
					Ok(a) => println!("{}", a),
					Err(e) => println!("Error: {}", e)
				}

				std::io::stdout().flush().unwrap();
				break;

			}
		}

	}

}

