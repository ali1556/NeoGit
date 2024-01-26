#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void print_command(int argc, char * const argv[]);
int run_init(int argc, char * const argv[]);

void print_command(int argc, char * const argv[]) {
    for (int i = 0; i < argc; i++) {
        fprintf(stdout, "%s ", argv[i]);
    }
    fprintf(stdout, "\n");
}

int run_init(int argc, char * const argv[]) {
    char cwd[1024];
    char tmp_cwd[1024];
    int exists = 0;
    struct dirent *entry;

    if (getcwd(cwd, sizeof(cwd)) == NULL) return 1;

    do {
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("We had an error trying to open the current directory. Please try again !\n");
            return 1;
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".neogit") == 0)
                exists = 1;
        }
        closedir(dir);

        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL) return 1;
 
        if (strcmp(tmp_cwd, "/") != 0) {
            if (chdir("..") != 0) return 1;
        }

    } while (strcmp(tmp_cwd, "/") != 0);

    if (chdir(cwd) != 0) return 1;
        
    if (!exists) {
        if (mkdir(".neogit", 0755) != 0) return 1;
        return create_configs("mohsen", "mohsenghasemi8156@gmail.com");
    } else {
        perror("neogit repository has already initialized");
    }
    return 0;
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stdout, "That's not a valid command!\n");
        return 1;
    }
    if (strcmp(argv[1], "init") == 0) {
        print_command(argc, argv);
    }

    if (strcmp(argv[1], "init") == 0) {
        run_init(argc, argv);
    }

}