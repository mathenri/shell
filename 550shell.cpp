/*
 * 550shell.cpp
 *
 *  Created on: Oct 1, 2014
 *      Author: mathenri
 *
 *      This program takes a line of program commands separated by the '|'
 *      character from a user and executes the programs in parallel, piping the
 *      output of the first program to the second's input, the second's output
 *      to the third's input, etc.
 */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <fstream>

using namespace std;

/*
 * Removes white space characters from beginning and end of an input string.
 * Returns a pointer to a substring of the input string.
 */
char *trim_white_space(char *inputString) {
	char *end;

	// Remove white space characters from front of string
	while (isspace(*inputString)) {
		inputString++;
	}

	// Remove white space characters from end of string
	end = inputString + strlen(inputString) - 1;
	while (end > inputString && isspace(*end)) {
		end--;
	}

	// Add null terminator
	*(end + 1) = '\0';

	return inputString;
}

/*
 * Takes a vector of commands as input and executes them in parallel. The first
 * program's output will be piped to the second program's input, etc. The last
 * program's output will be written to stdout.
 */
void execute_commands(vector<char *> commands) {

	int nrCommands = commands.size();
	int nrPipes = nrCommands - 1;
	int pipeFds[2 * nrPipes];
	pid_t processId;

	// create the pipes
	for (int i = 0; i < 2 * nrPipes; i++) {
		if (pipe(pipeFds + i * 2) == -1) {
			perror("pipe");
			exit(1);
		}
	}

	/* Fork off all the programs in order and assign their inputs and outputs
	 to the correct pipe file descriptor. */
	for (int commandIndex = 0; commandIndex < nrCommands; commandIndex++) {
		processId = fork();

		// if one of the child processes ...
		if (processId == 0) {

			// ... if this is not the first command ...
			if (commandIndex != 0) {

				// ... setup stdin pipe file descriptor ...
				if (dup2(pipeFds[(commandIndex - 1) * 2], 0) == -1) {
					perror("dup2");
					exit(1);
				}
			}

			// ... if this is not the last command ...
			if (commandIndex != nrCommands - 1) {

				// ... setup stdout pipe file descriptor
				if (dup2(pipeFds[commandIndex * 2 + 1], 1) == -1) {
					perror("dup2");
					exit(1);
				}
			}

			// close all pipes
			for (int i = 0; i < 2 * nrPipes; i++) {
				close(pipeFds[i]);
			}

			// execute the command
			if (execlp(trim_white_space(commands[commandIndex]),
					commands[commandIndex], NULL) == -1) {
				perror("execlp");
				exit(1);
			}

			// ... else if error while forking, terminate program
		} else if (processId < 0) {
			perror("fork");
			exit(1);
		}
	}

	// if parent process, close all of the pipe file descriptors
	for (int i = 0; i < 2 * nrPipes; i++) {
		close(pipeFds[i]);
	}

	// wait for child processes to finish before returning
	wait(NULL);
}

int main() {
	string inputString;

	printf("Welcome! Use ctrl+d (EOF) to terminate program.\n");
	while (true) {
		printf("> ");

		// read a line of command from the user
		if (!getline(cin, inputString)) {
			printf("Terminating program!\n");
			exit(1);
		}

		// divide input line into separate commands and put in vector
		vector<char *> commands;
		const char *delimiter = "|";

		/* input line must be transformed to char pointer format to go as a
		 parameter in strtok() */
		char *inputCharPointer = new char[inputString.size() + 1];
		copy(inputString.begin(), inputString.end(), inputCharPointer);
		inputCharPointer[inputString.size()] = '\0';

		char *command = strtok(inputCharPointer, delimiter);

		while (command != NULL) {
			commands.push_back(command);
			command = strtok(NULL, delimiter);
		}

		// execute the commands
		execute_commands(commands);

		// free inputCharPointer variable from memory
		delete[] inputCharPointer;
	}
	return 0;
}

