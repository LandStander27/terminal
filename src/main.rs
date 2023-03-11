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
		help: "Lists current directory".to_string(),
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
		help: "Changes the current directory".to_string(),
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
		help: "Lists current tasks".to_string(),
		arg_count: Vec::from([1]),
		func: |_args: Vec<String>| {
			let mut s = System::new();
			s.refresh_processes();

			let mut values: Vec<Vec<String>> = Vec::new();

			for (pid, proc) in s.processes() {
				values.push(Vec::from([pid.to_string(), proc.name().to_string(), if proc.exe().to_str().unwrap() != "" {String::from(proc.exe().file_name().unwrap().to_str().unwrap())} else {"".to_string()}]));
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

			println!("PID{}NAME{}PATH{}", " ".repeat(max_length[0]-3+3), " ".repeat(max_length[1]-4+3), " ".repeat(max_length[2]-4+3));
			println!("{} {} {}", "=".repeat(max_length[0]+2), "=".repeat(max_length[1]+2), "=".repeat(max_length[2]+2));

			for i in values {
				println!("{}{}{}{}{}", i[0], " ".repeat(max_length[0]-i[0].len()+3), i[1], " ".repeat(max_length[1]-i[1].len()+3), i[2]);
			}

			Ok("".to_string())
		}
	});

	let help_command = || {

		for i in 0..commands.len() {
			println!("{}: {}", commands[i].name, commands[i].help);
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
				help_command();
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

