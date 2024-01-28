#include <stdio.h>
#include <glob.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
int numbOfAdds = 0;
#define CONFIG_FILE "/Users/alinr/Desktop/config.txt"
bool is_neogit_initialized() {
    char cwd[1024];

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        return false;
    }

    do {
        DIR *dir = opendir(".neogit");
        if (dir != NULL) {
            closedir(dir);
            return true;  
        }

        char parent_dir[1024];
        strcpy(parent_dir, dirname(cwd));

        if (strcmp(parent_dir, cwd) == 0) {
            break;  
        }

        strcpy(cwd, parent_dir);

    } while (true);

    return false;  
}

// FUNCTION DECLARATIONS 
int config(char *username, char *email, int isGlobal);
int run_init(int argc, char * const argv[]);
//
int run_add(int argc, char * const argv[]);
int is_directory(char *path);
int add_to_staging(char *path);
int add_directory_contents(char *dir_path);
//
int compareFiles(const char *file1Path, const char *file2Path);
//
int run_reset(int argc, char * const argv[]);
int reset_file(char *filepath);
int undo_last_add();
//
int is_file_in_staging(const char *filename);
int status();
//////////////////////////////////////////////////////
// config & init
int config(char *username, char *email, int isGlobal) {
    FILE *file = fopen(CONFIG_FILE, "r+");  

    if (file == NULL) {
        perror("Error opening config file");
        return 1;
    }

    char line[256];
    char newLine[256];
    char prefix[20];
    int found = 0;

    if (isGlobal) {
        sprintf(prefix, "global_%s", strcmp(username, "") == 0 ? "email" : "username");

        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, prefix) != NULL) {
                sprintf(newLine, "%s : %s\n", prefix, strcmp(username, "") == 0 ? email : username);
                fseek(file, -strlen(line), SEEK_CUR);
                fprintf(file, "%s", newLine);
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(file, "%s : %s\n", prefix, strcmp(username, "") == 0 ? email : username);
        }
    }

    fclose(file);
    return 0;
}
int create_configs(char *username, char *email) {
    FILE *file = fopen(".neogit/config", "w");
    if (file == NULL) return 1;

    fprintf(file, "username: %s\n", username);
    fprintf(file, "email: %s\n", email);
    fprintf(file, "last_commit_ID: %d\n", 0);
    fprintf(file, "current_commit_ID: %d\n", 0);
    fprintf(file, "branch: %s", "master");
    fclose(file);

    if (mkdir(".neogit/files", 0755) != 0) return 1;
    
    if (mkdir(".neogit/commits", 0755) != 0) return 1;

    file = fopen(".neogit/staging", "w");
    fclose(file);

    file = fopen(".neogit/tracks", "w");
    fclose(file);

    file = fopen(".neogit/allAdds", "w");
    fclose(file);

    return 0;
}
int get_user_config(char *username, char *email, int isGlobal) {
    if (isGlobal) {
        printf("Enter global username: ");
        scanf("%s", username);
        printf("Enter global email: ");
        scanf("%s", email);
    } else {
        printf("Enter local username: ");
        scanf("%s", username);
        printf("Enter local email: ");
        scanf("%s", email);
    }

    return 0;
}
int run_init(int argc, char *const argv[]) {
    char cwd[1024];
    char tmp_cwd[1024];
    int exists = 0;
    struct dirent *entry;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 1;

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

        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL)
            return 1;

        if (strcmp(tmp_cwd, "/") != 0) {
            if (chdir("..") != 0)
                return 1;
        }

    } while (strcmp(tmp_cwd, "/") != 0);

    if (chdir(cwd) != 0)
        return 1;

    if (!exists) {
        if (mkdir(".neogit", 0755) != 0)
            return 1;

        char username[1000];
        char email[1000];
        int isGlobal = 0;

        if (argc > 2 && strcmp(argv[2], "-global") == 0) {
            isGlobal = 1;
        }

        if (get_user_config(username, email, isGlobal) != 0) {
            perror("Error getting user configuration");
            return 1;
        }
        printf("Neogit initialized successfully!\n");

        return create_configs(username, email);
    } else {
        printf("Neogit repository has already been initialized");
    }
    return 0;
}
// add 
int run_add(int argc, char * const argv[]) {
    if (argc < 3) {
        perror("Please specify at least one file or directory");
        return 1;
    }

    int forceFlag = 0;
    if (argc > 3 && strcmp(argv[2], "-f") == 0) {
        forceFlag = 1;
    }
    if (argc > 3 && strcmp(argv[2], "-f") != 0) {
        printf("You need to use neogit add -f in order to add multiple files!\n");
        return 0;
    }

    for (int i = forceFlag ? 3 : 2; i < argc; ++i) {
        glob_t glob_result;
        if (glob(argv[i], GLOB_ERR | GLOB_MARK, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc; ++j) {
                if (add_to_staging(glob_result.gl_pathv[j]) != 0 && !forceFlag) {
                    fprintf(stderr, "%s doesn't exist! \n", glob_result.gl_pathv[j]);
                }
            }
            globfree(&glob_result);
        } else {
            if (!forceFlag) {
                fprintf(stderr, "Error expanding wildcard pattern for %s\n", argv[i]);
            }
        }
    }

    return 0;
}
int is_directory(char *path) {
    DIR *dir = opendir(path);
    if (dir != NULL) {
        closedir(dir);
        return 1;
    }
    return 0;
}
int add_to_staging(char *path) {
    if (access(path, F_OK) == -1) {
        return 1;
    }

    if (is_directory(path)) {
        if (add_directory_contents(path) != 0) {
            return 1;
        }
    } else {
        FILE *file = fopen(".neogit/staging", "r");
        if (file == NULL) return 1;

        char command[1000];
        while (fgets(command, sizeof(command), file) != NULL) {
            int length = strlen(command);

            if (length > 0 && command[length - 1] == '\n') {
                command[length - 1] = '\0';
            }

            if (strcmp(path, command) == 0) {
                fclose(file);
                return 0;
            }
        }
        fclose(file);

        file = fopen(".neogit/staging", "a");
        if (file == NULL) return 1;

        fprintf(file, "%s\n", path);
        printf("%s added to staging area!\n", path);
        fclose(file);
        // add to allAdds 
        file = fopen(".neogit/allAdds", "a");
        if (file == NULL) return 1;

        fprintf(file, "%s\n", path);
        fclose(file);
        // add to tracks
        file = fopen(".neogit/tracks", "a");
        if (file == NULL) return 1;

        fprintf(file, "%s\n", path);
        fclose(file);
    }

    return 0;
}
int add_directory_contents(char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Error opening directory");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char entry_path[1000];
            snprintf(entry_path, 1000, "%s/%s", dir_path, entry->d_name);
            if (add_to_staging(entry_path) != 0) {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}
//function to compare
int compareFiles(const char *file1Path, const char *file2Path) {
    FILE *file1 = fopen(file1Path, "rb");
    FILE *file2 = fopen(file2Path, "rb");

    if (file1 == NULL || file2 == NULL) {
        perror("Error opening files");
        return -1;  
    }

    fseek(file1, 0, SEEK_END);
    long fileSize1 = ftell(file1);
    fseek(file1, 0, SEEK_SET);

    fseek(file2, 0, SEEK_END);
    long fileSize2 = ftell(file2);
    fseek(file2, 0, SEEK_SET);

    if (fileSize1 != fileSize2) {
        fclose(file1);
        fclose(file2);
        return 1;  
    }

    char *buffer1 = malloc(fileSize1);
    char *buffer2 = malloc(fileSize2);

    if (buffer1 == NULL || buffer2 == NULL) {
        perror("Error allocating memory");
        fclose(file1);
        fclose(file2);
        return -1;  
    }

    fread(buffer1, 1, fileSize1, file1);
    fread(buffer2, 1, fileSize2, file2);

    fclose(file1);
    fclose(file2);

    int result = memcmp(buffer1, buffer2, fileSize1);

    free(buffer1);
    free(buffer2);

    return (result == 0) ? 0 : 1;  
}
// reset 
int run_reset(int argc, char * const argv[]) {
    if (argc < 3) {
        perror("Please specify at least one file or directory");
        return 1;
    }

    int forceFlag = 0;
    if (argc > 3 && strcmp(argv[2], "-f") == 0) {
        forceFlag = 1;
    }

    for (int i = forceFlag ? 3 : 2; i < argc; ++i) {
        glob_t glob_result;
        if (glob(argv[i], GLOB_ERR | GLOB_MARK, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc; ++j) {
                if (reset_file(glob_result.gl_pathv[j]) != 0 && !forceFlag) {
                    fprintf(stderr, "%s doesn't exist in the staging area!\n", glob_result.gl_pathv[j]);
                }
            }
            globfree(&glob_result);
        } else {
            if (!forceFlag) {
                fprintf(stderr, "Error expanding wildcard pattern for %s\n", argv[i]);
            }
        }
    }

    return 0;
}
int reset_file(char *filepath) {
    FILE *staging_file = fopen(".neogit/staging", "r");
    if (staging_file == NULL) return 1;

    FILE *tmp_file = fopen(".neogit/tmp_staging", "w");
    if (tmp_file == NULL) {
        fclose(staging_file);
        return 1;
    }

    FILE *allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL) {
        fclose(staging_file);
        fclose(tmp_file);
        return 1;
    }

    int found = 0;
    char line[1000];

    // Iterate through the staging file
    while (fgets(line, sizeof(line), staging_file) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        if (strcmp(filepath, line) == 0) {
            found = 1;
        } else {
            fprintf(tmp_file, "%s\n", line);
        }
    }

    fclose(staging_file);
    fclose(tmp_file);

    if (!found) {
        // If the file was not found in the staging area, clean up and return an error
        remove(".neogit/tmp_staging");
        fclose(allAddsFile);
        fprintf(stderr, "%s is not in the staging area!\n", filepath);
        return 1;
    }

    remove(".neogit/staging");
    rename(".neogit/tmp_staging", ".neogit/staging");

    // Open the allAdds file for writing
    FILE *newAllAddsFile = fopen(".neogit/tmp_allAdds", "w");
    if (newAllAddsFile == NULL) {
        fclose(allAddsFile);
        fprintf(stderr, "Error opening tmp_allAdds file for writing.\n");
        return 1;
    }


    while (fgets(line, sizeof(line), allAddsFile) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        int entry_number;
        if (sscanf(line, "%d . %[^\n]", &entry_number, entry_filepath) != 2) {
            continue;  // Skip invalid lines
        }

        if (strcmp(filepath, entry_filepath) == 0) {
            found = 1;
        } else {
            fprintf(newAllAddsFile, "%d . %s\n", entry_number, entry_filepath);
        }
    }

    fclose(allAddsFile);
    fclose(newAllAddsFile);

    if (!found) {
        // If the file was not found in allAdds, clean up and return an error
        remove(".neogit/tmp_allAdds");
        fprintf(stderr, "%s is not in the allAdds file!\n", filepath);
        return 1;
    }

    remove(".neogit/allAdds");
    rename(".neogit/tmp_allAdds", ".neogit/allAdds");

    printf("%s removed from the staging area.\n", filepath);
    return 0;
}
int undo_last_add() {
    FILE *allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL) {
        fprintf(stderr, "Error opening allAdds file for reading.\n");
        return 1;
    }

    char lastAddedFile[1000];
    char lastAddedLine[1000];

    char line[1000];
    while (fgets(line, sizeof(line), allAddsFile) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        if (sscanf(line, "%[^\n]", entry_filepath) == 1) {
            strcpy(lastAddedFile, entry_filepath);
            strcpy(lastAddedLine, line);
        }
    }

    fclose(allAddsFile);

    if (lastAddedFile[0] == '\0') {
        fprintf(stderr, "No files to undo in the allAdds file.\n");
        return 1;
    }

    printf("Undo: Removing %s from the allAdds file.\n", lastAddedFile);

    FILE *newAllAddsFile = fopen(".neogit/tmp_allAdds", "w");
    if (newAllAddsFile == NULL) {
        fprintf(stderr, "Error opening tmp_allAdds file for writing.\n");
        return 1;
    }

    allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL) {
        fclose(newAllAddsFile);
        fprintf(stderr, "Error opening allAdds file for reading.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), allAddsFile) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        if (sscanf(line, "%[^\n]", entry_filepath) == 1) {
            // Check if the entry matches the last added file
            if (strcmp(entry_filepath, lastAddedFile) != 0) {
                fprintf(newAllAddsFile, "%s\n", entry_filepath);
            }
        }
    }

    fclose(allAddsFile);
    fclose(newAllAddsFile);

    remove(".neogit/allAdds");
    rename(".neogit/tmp_allAdds", ".neogit/allAdds");

    FILE *stagingFile = fopen(".neogit/staging", "r");
    if (stagingFile == NULL) {
        fprintf(stderr, "Error opening staging file for reading.\n");
        return 1;
    }

    FILE *newStagingFile = fopen(".neogit/tmp_staging", "w");
    if (newStagingFile == NULL) {
        fclose(stagingFile);
        fprintf(stderr, "Error opening tmp_staging file for writing.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), stagingFile) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        if (strcmp(line, lastAddedLine) != 0) {
            fprintf(newStagingFile, "%s\n", line);
        }
    }

    fclose(stagingFile);
    fclose(newStagingFile);

    remove(".neogit/staging");
    rename(".neogit/tmp_staging", ".neogit/staging");

    return 0;
}
// status 

int is_file_in_staging(const char *filename) {
    FILE *staging_file = fopen(".neogit/staging", "r");
    if (staging_file == NULL) {
        perror("Error opening staging file for reading");
        return 0; // Assume file is not in staging if there is an error
    }

    char line[1000];
    while (fgets(line, sizeof(line), staging_file) != NULL) {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        if (strcmp(filename, line) == 0) {
            fclose(staging_file);
            return 1; // File is in staging
        }
    }

    fclose(staging_file);
    return 0; // File is not in staging
}
int status() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening directory");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Check if it's a regular file
            printf("%s %s\n", entry->d_name,is_file_in_staging(entry->d_name) ? "+" : "-");
        }
    }

    closedir(dir);

    return 0;
}


















int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, "Invalid command!\n");
        return 1;
    }

    if (strcmp(argv[1], "config") == 0) {
        if (strcmp(argv[2], "-global") == 0) {
            if (strcmp(argv[3], "user.name") == 0) {
                return config(argv[4], "", 1);  
            } else if (strcmp(argv[3], "user.email") == 0) {
                return config("", argv[4], 1);  
            }
        }
    }
    else if (strcmp(argv[1], "init") == 0) {
        return run_init(argc, argv);
    }
    if (!is_neogit_initialized()){
        printf("Neogit has not been initialized yet!\n");
        return 1;
    }
    else if (strcmp(argv[1], "add") == 0) {
        return run_add(argc, argv);
    }
     else if (strcmp(argv[1], "reset") == 0) {
        if (argc > 2 && strcmp(argv[2], "-undo") == 0) {
            return undo_last_add();
        } else {
            return run_reset(argc, argv);
        }
    }
    else if (strcmp(argv[1], "status") == 0) {
        return status();
    }



    fprintf(stdout, "Invalid command!\n");
    return 1;
}

