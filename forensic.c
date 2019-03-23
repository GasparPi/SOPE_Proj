#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <dirent.h>

#include "print_info.h"
#include "logs.h"

int is_directory(const char *path);
int check_command(int argc, char *argv[], char command[]);
void sigint_handler(int sig_no); 
int process_dir(const char *path, bool read_sub_dir, char *hash_commands, int log_file_des);

int main(int argc, char *argv[], char *envp[])
{
    int file_des;
    pid_t pid;
    struct stat file_stat;

    if (argc < 2 || argc > 9)
    {
        perror("Wrong number of possible arguments.");
        exit(1);
    }

    //Command: -v
    int log_file_des;
    int log_command_index = check_command(argc, argv, "-v");
    if (log_command_index != -1)
    {
        char *log_file_name = (char *)malloc(sizeof(char) * 80);
        log_file_name = getenv("LOGFILENAME");
        
        log_file_des = open(log_file_name, O_WRONLY | O_CREAT | O_APPEND, 0750);
    }
    else
        log_file_des = -1;

    //Command: -o
    int output_command_index = check_command(argc, argv, "-o");
    if (output_command_index != -1)
    {
        char output_file_name[20];
        int output_file_des, stdout_copy;

        strcpy(output_file_name, argv[output_command_index + 1]);
        output_file_des = open(output_file_name, O_WRONLY | O_CREAT, 0750);

        stdout_copy = dup(STDOUT_FILENO);
        dup2(output_file_des, STDOUT_FILENO);
    }

    //Command: -h
    char *hash_commands = (char *)malloc(sizeof(char) * 20);
    int hash_command_index = check_command(argc, argv, "-h");
    if (hash_command_index != -1)
        strcpy(hash_commands, argv[hash_command_index + 1]);
    else
        hash_commands = NULL;

    if (!is_directory(argv[argc - 1]))
    {
        if ((file_des = open(argv[argc - 1], O_RDONLY)) == -1)
        {
            perror("File does not exist.");
            exit(1);
        }

        pid = fork();
        if (pid == 0)
        { //Child

            if (log_command_index != -1)
            {
                char act[] = "ANALYZED ";
                strcat(act, argv[argc - 1]);
                register_log(log_file_des, getpid(), "ANALYZED ");
            }

            print(argv[argc - 1], hash_commands, log_file_des);
            perror("Error: print_info!");
            exit(1);
        }
        else
        { //Parent
            wait(NULL);
            exit(0);
        }
    }
    else {
        //Command: -r
        int subdir_command_index = check_command(argc, argv, "-r");
        if (subdir_command_index != -1)
            process_dir(argv[argc - 1], true, hash_commands, log_file_des);
        else
            process_dir(argv[argc - 1], false, hash_commands, log_file_des);
    }
}

int check_command(int argc, char *argv[], char command[])
{
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(command, argv[i]))
            return i;
    }

    return -1;
}

int is_directory(const char *path)
{
    struct stat file_stat;
    if (stat(path, &file_stat) != 0)
    {
        perror("Error reading file info.");
        return -1;
    }

    return S_ISDIR(file_stat.st_mode);
}

int process_dir(const char *path, bool read_sub_dir, char *hash_commands, int log_file_des)
{
    pid_t pid;
    DIR *dirp;
    struct dirent *direntp;
    char sub_path[200];

    if ((dirp = opendir(path)) == NULL)
    {
        perror(path);
        exit(1);
    }

    while ((direntp = readdir(dirp)) != NULL)
    {
        sprintf(sub_path, "%s/%s", path, direntp->d_name);
        if (!is_directory(sub_path))
            print(sub_path, hash_commands, log_file_des);
        else if (is_directory(sub_path) && read_sub_dir)
        {
            pid = fork();
            if (pid == 0)
            { //Child
                process_dir(path, read_sub_dir, hash_commands, log_file_des);
                exit(1);
            }
            else
            { //Parent
                continue;
            }
        }
    }

    return 0;
}
