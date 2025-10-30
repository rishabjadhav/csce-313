#include <iostream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include <vector>
#include <string>
#include <string.h>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    for (;;) {

        char datetime[64]; // datetime buffer
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        char cwd[1024]; // working directory buffer
        getcwd(cwd, sizeof(cwd));

        strftime(datetime, sizeof(datetime), "%b %d %H:%M:%S", t);

        string usr;
        if (getenv("USER") == NULL) {
            usr = "root";
        } else {
            usr = getenv("USER");
        }

        cout << YELLOW << datetime << " " << usr << ":" << cwd << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // cd handling here
        if (input.substr(0, 2) == "cd") {
            // cd command given:
            string path = input.substr(3);

            // set oldpwd env if no cd hyphen
            if (path != "-") {
                char cd[1024];
                if (getcwd(cd, sizeof(cd)) != NULL) {
                    setenv("OLDPWD", cd, 1);
                }
            }

            if (path == "-") {
                // go to last directory
                if (getenv("OLDPWD") != NULL) {
                    path = getenv("OLDPWD");
                    cout << path << endl;
                } else {
                    continue;
                }
	        }

            chdir(path.c_str());

            continue;
        }

        // get tokenized commands from user input
        // initializes tknr with input to hold all commands as vector of Command objects
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
         // for (auto cmd : tknr.commands) {
             // for (auto str : cmd->args) {
                 // cerr << "|" << str << "| ";
             // }
             // if (cmd->hasInput()) {
                 // cerr << "in< " << cmd->in_file << " ";
             // }
             // if (cmd->hasOutput()) {
                 // cerr << "out> " << cmd->out_file << " ";
             // }
             // cerr << endl;
         // }

        int numCommands = tknr.commands.size();

        // piping like this makes a lot of sense, each command's output is the next's input when seperated by pipes
        std::vector<int> pipefds(2 * std::max(0, numCommands - 1));

        // init all pipes
        for (int i = 0; i < numCommands - 1; ++i) {
            if (pipe(&pipefds[i * 2]) < 0) {
                perror("pipe");
            }
        }

        // track child pids so the parent (the shell) can wait for them
        std::vector<pid_t> child_pids;

        // iterate through each command
        for (int i = 0; i < numCommands; ++i) {
            pid_t cpid = fork();
            if (cpid < 0) {  // error check
                perror("command fork");
            }

            if (cpid == 0) {  // in child process

                // if not first command, default the input of each command comes from STDIN (prev command output)
                if (i > 0) {
                    dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
                }

                // if not last command, default the output of each command to write to STDOUT (next command input)
                if (i < numCommands - 1) {
                    dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
                }

                // override in case of file input, now we read from the file instead of STDIN
                if (tknr.commands.at(i)->hasInput()) {
                    int fd = open(tknr.commands.at(i)->in_file.c_str(), O_RDONLY);
                    if (fd < 0) {
                        perror("open input file");
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                // override in case of file output, now we write to the file instead of STDOUT
                if (tknr.commands.at(i)->hasOutput()) {
                    int fd = open(tknr.commands.at(i)->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("open output file");
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                // close all pipe fds in child
                for (int j = 0; j < 2 * (numCommands - 1); j++) {
                    close(pipefds[j]);
                }

                // collect all args of this command
                std::vector<char*> vargs;
                for (size_t j = 0; j < tknr.commands.at(i)->args.size(); ++j) {
                    vargs.push_back(const_cast<char*>(tknr.commands.at(i)->args.at(j).c_str()));
                }
                vargs.push_back(nullptr);

                if (tknr.commands.at(i)->isBackground()) {
                    // is in background
                    pid_t bgpid = fork();
                    
                    if (bgpid < 0) {perror("background fork");}

                    if (bgpid == 0) {
                        // child task, background process
                        if (execvp(vargs[0], vargs.data()) < 0) {
                            perror("execvp bg");
                            exit(2);
                        }
                    } else {
                        // parent task, move on 
                        continue;
                    }
                }

                if (execvp(vargs[0], vargs.data()) < 0) {
                    perror("execvp");
                    exit(2);
                }
            } else {
                // parent process

                // log child_pid and move on
                child_pids.push_back(cpid);
            }
        }

        // parent: close all pipe fds - not needed anymore
        for (int j = 0; j < 2 * (numCommands - 1); j++) {
            close(pipefds[j]);
        }

        // wait for all child processes
        for (pid_t p : child_pids) {
            int status = 0;
            waitpid(p, &status, 0);
        }
    }
}
