Intro:
   This project involves implementing a UNIX Shell using the C programming language. The shell serves as the user interface of an operating system, supporting features like external commands, pipes, and file redirection.

Learning Objectives:

   Understand the system call ABI to UNIX systems.
   Work with UNIX file descriptors for pipes, file input/output, and appending.
   Sharpen systems programming skills through a large project.

Project Requirements:

External Commands (Deliverable 1):

   Implement execution of external commands using system calls like fork, execvp, etc.
   Shell should wait for external command completion.

Pipes (Deliverable 2):

   Handle an arbitrary number of commands piped together.
   Support various pipe scenarios like command1 | command2 | command3 | command4.

File Redirection (Deliverable 2):

   Handle file redirection using >, >>, or <.
   Create files if they don't exist for > and >>.
   Support < at the beginning and >> or > at the end of a pipeline.
   Terminal Interaction

   Implement an input loop using readline.
   Customize the prompt to display :) or :( based on the last exit status.
   Parse input commands, supporting pipelines, and file operations.
   Builtin Commands

Implement builtin commands like cd and exit.
Print appropriate error messages for invalid inputs.
exit takes zero or one argument for exit code.
cd takes one argument, the directory to change to.

Starter Code:

Provided input loop, input parser, and builtin commands.
Entry point into code is dispatch_external_command in src/dispatcher.c.
Reference Executables:

Reference executables shell_d1 and shell_d2 provided for understanding behavior.
