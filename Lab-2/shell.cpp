/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <iostream>
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int fd[2];
    pipe(fd);   //fd[0] = read, fd[1] = write

    // Create child to run first command
    pid_t pid1 = fork();

    if (pid1 < 0) {
        // error state
        cout << "First fork failed." << endl;
    } else if (pid1 == 0) {
        // pid == 0, running in CHILD process

        // In child, redirect output to write end of pipe
        dup2(fd[1], STDOUT_FILENO);

        // Close the read end of the pipe on the child side.
        close(fd[0]);

        // In child, execute the command
        execvp(cmd1[0], cmd1);
    } else {
        // pid > 0, running in PARENT process
    }

    // Create another child to run second command
    pid_t pid2 = fork();

    if (pid2 < 0) {
        cout << "Second fork failed" << endl;
    } else if (pid2 == 0) {
        // pid == 0, running in CHILD process

        // In child, redirect input to the read end of the pipe
        dup2(fd[0], STDIN_FILENO);

        // Close the write end of the pipe on the child side.
        close(fd[1]);

        // Execute the second command.
        execvp(cmd2[0], cmd2);
    } else {
        // pid > 0, running in PARENT process
    }
    // Reset the input and output file descriptors of the parent.
    close(fd[0]);
    close(fd[1]);
}
