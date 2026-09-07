
# DSH — Unix-like Shell

**Author:** Antonio Carano  
**Language:** C

## Overview

DSH is a lightweight Unix-like shell implemented in C.

The shell provides the basic mechanisms required to execute commands and manage processes, together with support for background execution, command history, pipes and output redirection.

The project focuses on low-level Unix process management and makes use of system calls and libraries such as `fork()`, `execv()`, `waitpid()`, `pipe()`, `dup2()` and GNU Readline.

## Features

DSH currently supports:

- execution of external commands;
- executable lookup through a configurable `PATH`;
- foreground and background processes;
- background execution using `&`;
- cleanup of terminated background processes;
- command history with the last 10 commands;
- history navigation using the arrow keys;
- `history` built-in command;
- output redirection using `>`;
- output append using `>>`;
- pipes using `|`;
- `setpath` built-in command;
- `exit` built-in command.

---

## Requirements

The shell requires:

- a Unix-like operating system;
- a C compiler such as GCC;
- GNU Readline.

On systems where GNU Readline is not already installed, the corresponding development library must be installed before compilation.

---

## Compilation

Compile the shell with:

    gcc dsh.c -lreadline -o dsh

Then start it with:

    ./dsh

An interactive prompt will be displayed:

    dsh$

---

# Command Execution

Commands entered in the shell are tokenized and executed by creating a new process using `fork()`.

For example:

    dsh$ ls

The child process executes the requested program, while the parent process normally waits for its termination.

The main execution logic is handled by:

    do_exec()

If the command contains an absolute path, it is executed directly:

    dsh$ /bin/ls

Otherwise, DSH searches for the executable using its internal `PATH`.

---

# PATH Resolution

DSH maintains an internal PATH initialized as:

    /bin/:/usr/bin/

When a command is entered without an absolute path, the shell searches for the executable inside the directories contained in the PATH.

For example:

    dsh$ ls

The `path_lookup()` function checks the available directories and uses:

    access(path, X_OK)

to verify whether the corresponding file exists and is executable.

Once the executable is found, its path is passed to `execv()`.

The PATH can be changed using the built-in `setpath` command:

    dsh$ setpath /bin/:/usr/bin/

The operation is handled by:

    set_path()

---

# Background Execution

DSH supports background execution using the `&` operator.

For example:

    dsh$ sleep 10 &

When `&` is present as the last argument, the command is executed in background.

The shell therefore immediately returns control to the user:

    dsh$ sleep 10 &
    dsh$

The `sleep` process continues running while DSH remains available for additional commands.

## Implementation

Before executing a command, DSH checks the last argument.

If the last argument is `&`, it is removed from the argument list before the program is executed.

Conceptually:

    if(strcmp(last_element, "&") == 0)
        arg_list[i - 1] = NULL;

This is necessary because `&` is interpreted by the shell and must not be passed to the executed program.

The command is then executed through a child process created with:

    fork()

For a normal foreground command, the parent process waits for the child:

    waitpid(pid, NULL, 0);

For a background command, the parent does not perform a blocking wait.

Instead, the PID of the child process is stored:

    add_child_process(pid);

The shell can therefore immediately continue its main loop and accept another command.

---

# Background Process Tracking

DSH keeps track of background processes using an array of process IDs:

    pid_t child_processes[MAX_CHILD_PROCESSES];

The number of currently tracked processes is maintained through:

    int num_child_processes;

Two functions manage this structure:

    add_child_process()

adds a new background process to the list, while:

    remove_child_process()

removes a process after it has terminated.

This allows DSH to keep track of child processes without blocking the interactive shell.

---

# Zombie Process Handling

Background processes require additional handling after their termination.

When a child process terminates, the operating system keeps a small amount of information about it until its parent retrieves its termination status.

Without this operation, terminated children would remain as zombie processes.

DSH periodically checks for terminated children using:

    waitpid(-1, &status, WNOHANG);

The `WNOHANG` option is important because it allows the shell to check for terminated processes without blocking.

The cleanup is performed by:

    void handle_zombies() {
        int status;
        pid_t pid;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            remove_child_process(pid);
        }
    }

The function is called during the main shell loop.

If one or more background processes have terminated, their status is collected and their PID is removed from the internal process list.

This allows DSH to manage background processes while keeping the shell responsive.

---

# Command History

DSH maintains a history containing the last **10 commands** entered by the user.

The history can be displayed using:

    dsh$ history

For example:

    1: ls
    2: pwd
    3: echo hello
    4: sleep 10 &
    5: history

The history is stored internally using:

    char history[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];

where:

    #define MAX_HISTORY_SIZE 10

The number of stored commands is tracked through:

    int history_count;

---

## History Management

New commands are added through:

    add_to_history()

While fewer than 10 commands are stored, new commands are simply appended to the history.

Once the maximum size is reached, the oldest command is removed.

The remaining commands are shifted:

    history[1] -> history[0]
    history[2] -> history[1]
    ...
    history[9] -> history[8]

The newest command is then stored in:

    history[9]

As a result, the shell always keeps the 10 most recent commands.

The history can be printed using:

    print_history()

---

# Arrow-Key History Navigation

DSH uses GNU Readline to provide interactive command-line input.

The required headers are:

    #include <readline/readline.h>
    #include <readline/history.h>

Readline history support is initialized when the shell starts:

    using_history();

Commands are registered using:

    add_history(command);

This allows previously entered commands to be retrieved directly from the prompt.

The **Up Arrow** retrieves older commands:

    ↑

The **Down Arrow** moves back toward more recent commands:

    ↓

This provides command-line interaction similar to common Unix shells.

---

# Input Handling

Interactive input is handled through GNU Readline.

Instead of manually reading characters from standard input, DSH uses:

    char* input = readline(prompt_string);

The resulting command is copied into the shell input buffer:

    strncpy(buf, input, buf_size);

The memory allocated by Readline is then released:

    free(input);

Using Readline provides interactive command editing and history navigation while keeping the input handling logic simple.

---

# Output Redirection

DSH supports output redirection using the `>` operator.

For example:

    dsh$ ls > files.txt

The shell detects the redirection operator, separates the output filename from the command and executes the operation through:

    do_redir()

The output file is opened and standard output is redirected using:

    dup2(fileno(out), 1);

The executed program therefore writes to the specified file instead of the terminal.

---

# Append Redirection

The `>>` operator can be used to append output to an existing file.

For example:

    dsh$ echo hello >> output.txt

Unlike `>`, the existing contents of the file are preserved and the new output is appended.

Both forms of output redirection are handled by:

    do_redir()

using different file opening modes.

---

# Pipes

DSH supports communication between two commands through the `|` operator.

For example:

    dsh$ ls | grep txt

The shell creates a pipe using:

    pipe(pipefd);

Two child processes are then created.

The first process executes the command on the left side of the pipe and redirects its standard output to the pipe.

The second process executes the command on the right side and receives the pipe as its standard input.

The redirections are performed using:

    dup2()

This allows the output produced by one process to be used directly as the input of another process.

Pipe execution is handled by:

    do_pipe()

---

# Built-in Commands

DSH currently provides three built-in commands.

## `history`

Displays the commands stored in the current history:

    dsh$ history

## `setpath`

Changes the PATH used by DSH to locate executable programs:

    dsh$ setpath /bin/:/usr/bin/

## `exit`

Terminates the shell:

    dsh$ exit

---

# Usage Examples

## Execute a command

    dsh$ ls

## Execute using an absolute path

    dsh$ /bin/ls

## Run a process in background

    dsh$ sleep 10 &

The prompt becomes immediately available again.

## Redirect output to a file

    dsh$ ls > files.txt

## Append output to a file

    dsh$ echo hello >> files.txt

## Connect two commands through a pipe

    dsh$ ls | grep txt

## Display command history

    dsh$ history

## Navigate through previous commands

Use the **Up Arrow** and **Down Arrow** keys directly from the prompt.

## Change the executable search path

    dsh$ setpath /bin/:/usr/bin/

## Exit the shell

    dsh$ exit

---

# Main Functions

| Function | Purpose |
|---|---|
| `main()` | Runs the main shell loop and parses commands |
| `prompt()` | Reads interactive input using GNU Readline |
| `do_exec()` | Executes standard commands |
| `do_redir()` | Handles `>` and `>>` output redirection |
| `do_pipe()` | Handles command pipes |
| `exec_rel2abs()` | Executes a command after resolving its path |
| `path_lookup()` | Searches for executables using the internal PATH |
| `set_path()` | Updates the shell PATH |
| `add_to_history()` | Stores commands in the 10-command history |
| `print_history()` | Displays stored commands |
| `add_child_process()` | Tracks a new background process |
| `remove_child_process()` | Removes a terminated background process |
| `handle_zombies()` | Collects terminated background child processes |
| `panic()` | Handles fatal errors |

