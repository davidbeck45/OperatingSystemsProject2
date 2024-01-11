#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
static int pipe_recurse(struct command *pipeline, int lastCmd, bool start);
static int pipe_out(struct command *pipeline, int fin, int *fd);
static int out(struct command *pipeline, int fin, int *fd);

static int dispatch_external_command(struct command *pipeline)
{
	bool start = true;
	int endCmd = -1;
	int x = pipe_recurse(pipeline, endCmd, start);
	return x;


	return 0;
}
// Helper function for executing an external command and redirecting its output to stdout
static int out(struct command *pipeline, int fin, int *fd){
    pid_t pid = fork(); // create a new child process
    if(pid == 0){ // if this is the child process
        if(fin!= -1){ // if a file input was specified
            dup2(fin,STDIN_FILENO); // redirect stdin to the specified file
        }
        dup2(fd[0], STDOUT_FILENO); // redirect stdout to the write end of the pipe
        execvp(pipeline->argv[0], pipeline->argv); // execute the external command
        exit(1); // if execvp fails, exit the child process with an error status
    }
    else{ // if this is the parent process
        int status;
        pid_t result;
        do {
            result = waitpid(pid, &status, 0); // wait for the child process to finish
        } while (result == -1 && errno == EINTR); // retry if waitpid was interrupted by a signal
        if(result == -1){ // if waitpid failed
            fprintf(stderr, "failed to wait for child process");
        }
        if(!WIFEXITED(status)){ // if the child process did not exit normally
            fprintf(stderr, "child process did not exit normally");
        }
        return WEXITSTATUS(status); // return the exit status of the child process
    }
}

// Helper function for executing an external command and redirecting its output to a pipe
static int pipe_out(struct command *pipeline, int fin, int *fd){
    if(pipe(fd) < 0){ // create a pipe and check for errors
        fprintf(stderr, "Did not pipe/cant pipe\n");
    }
    pid_t pid = fork(); // create a new child process
    if(pid == 0){ // if this is the child process
        close(fd[0]); // close the read end of the pipe
        dup2(fin, STDIN_FILENO); // redirect stdin to the specified file
        dup2(fd[1], STDOUT_FILENO); // redirect stdout to the write end of the pipe
        execvp(pipeline->argv[0], pipeline->argv); // execute the external command
        exit(1); // if execvp fails, exit the child process with an error status
    }else{ // if this is the parent process
        int status;
        pid_t result;
        do {
            result = waitpid(pid, &status, 0); // wait for the child process to finish
        } while (result == -1 && errno == EINTR); // retry if waitpid was interrupted by a signal
        if(result == -1){ // if waitpid failed
            fprintf(stderr, "failed to wait for child process");
        }
        if(!WIFEXITED(status)){ // if the child process did not exit normally
            fprintf(stderr, "child process did not exit normally");
        }
    }
    close(fd[1]); 
    return fd[0]; // return the read end of the pipe
}

static int pipe_recurse(struct command *pipeline, int lastCmd, bool start) {
    int fin; // file descriptor for input
    int fd[2]; // array for file descriptors for the pipe
    int status; // return status of the command

    // If an input file is specified and this is the first command in the pipeline
    if (pipeline->input_filename != NULL && start) {
        // Open the input file for reading
        fin = open(pipeline->input_filename, O_RDONLY, 0644);
        // If the file could not be opened, print an error message
        if (fin == -1) {
            fprintf(stderr, "Input file does not exist \n");
        }
    } 
    // If this is not the first command in the pipeline
    else if (!start) {
        // Set the input file descriptor to the output of the previous command
        fin = lastCmd;
    } 
    // If no input file is specified and this is the first command in the pipeline
    else {
        // Use standard input as the input file descriptor
        fin = STDIN_FILENO;
    }

    
    if (pipeline->output_type == COMMAND_OUTPUT_STDOUT) {
        // If the output type is stdout, set the file descriptor to stdout and run the command
        fd[0] = STDOUT_FILENO;
        status = out(pipeline, fin, fd);
    } else if (pipeline->output_type == COMMAND_OUTPUT_FILE_TRUNCATE) {
        // If the output type is to truncate a file, open the file for writing, truncate it, set the file descriptor, and run the command
        fd[0] = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd[0] == -1) {
            fprintf(stderr, "failed to open output file");
        }
        status = out(pipeline, fin, fd);
        close(*fd);
    } else if (pipeline->output_type == COMMAND_OUTPUT_FILE_APPEND) {
        // If the output type is to append to a file, open the file for writing and appending, set the file descriptor, and run the command
        fd[0] = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd[0] == -1) {
            fprintf(stderr, "failed to open output file");
        }
        status = out(pipeline, fin, fd);
        close(*fd);
    } else if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
        // If the output type is to pipe to another command, create a pipe, set the file descriptor, and run the command, then recursively call pipe_recurse with the next command as the argument
        fd[0] = pipe_out(pipeline, fin, fd);
        close(fd[1]);
        status = pipe_recurse(pipeline->pipe_to, fd[0], false);
    } else {
        // If the output type is invalid, print an error message and return -1
        fprintf(stderr, "Invalid output type\n");
        status = -1;
    }

    // Close the input file descriptor if this is not the first command in the pipeline or if an input file was specified
    if (!start || pipeline->input_filename != NULL) {
        close(fin);
    }

    // Return the status of the command
    return status;
}
/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
