#include <stdio.h>
#include <glob.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <libgen.h>


#define MAX_MESSAGE_LENGTH 73
#define MAX_FILENAME_LENGTH 257
#define MAX_LINE_LENGTH 1025
#define ALIAS_FILE ".neogit/alias"
#define CONFIG_FILE ".neogit/config"
#define GCONFIG_FILE ".neogit/gconfig"
#define TMP_FILE ".neogit/gconfig.tmp"

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

char *master = "master";

// FUNCTION DECLARATIONS
int findHighestFileNumberbranch(const char *branchname);
int configLocal();
char *get_current_branch();
int config(char *username, char *email, int isGlobal);
int run_init(int argc, char *const argv[]);

//

int run_add(int argc, char *const argv[]);
int is_directory(char *path);
int add_to_staging(char *path);
int add_directory_contents(char *dir_path);

//

int compareFiles(const char *file1Path, const char *file2Path);

//

int run_reset(int argc, char *const argv[]);
int reset_file(char *filepath);
int undo_last_add();

//

int is_file_in_staging(const char *filename);
int status();

//

int run_commit(int argc, char *const argv[]);
int inc_last_commit_ID();
bool check_file_directory_exists(char *filepath);
int commit_staged_file(int commit_ID, char *filepath);
int track_file(char *filepath);
bool is_tracked(char *filepath);
int create_commit_file(int commit_ID, char *message);
int find_file_last_commit(char *filepath);

//

int showlog();
int showlogn(int n);
int findHighestFileNumber();
int showlogBranch(char *branchname);
int showlogAuthor(char *author);
int logBefore(char *time);
int logSince(char *targetDate);
int logsearch(char *word);

//

int branch(char *path);
void copyFolder(const char *src, const char *dest);
int showBranches();

//

int removeCommitsGreaterThan(int commitID);
int checkout(char *branchname);

//

void add_alias(const char *alias, const char *command);
void read_aliases();
void execute_alias(const char *alias);

//

void add_shortcut(const char *shortcut_name, const char *shortcut_message);
const char *get_shortcut_message(const char *shortcut_name);
int replace_shortcut(const char *shortcut_name, const char *new_message);
int remove_shortcut(const char *shortcut_name);

//

int neogit_diff(const char *file1, const char *file2, int line1_start, int line1_end, int line2_start, int line2_end);
int compareLines(const char *line1, const char *line2);

//

int create_tag(const char *tag_name, const char *message, int cgiven, int commitid, int force);
int showAllTags();
int compareTags(const void *a, const void *b);
int showTagContent(const char *tagName);

//

int neogit_grep(const char *file, const char *word, const char *commitId, int printLineNumber);

// 


//////////////////////////MAIN FUNCTION////////////////////////////

int UserNameIsGlobal()
{
    FILE *file = fopen(".neogit/isglobal", "r");

    if (file != NULL)
    {
        int value;
        if (fscanf(file, "%d", &value) == 1)
        {
            fclose(file);
            return value;
        }
        fclose(file);
    }

    return -1;
}
char *get_current_branch()
{
    FILE *file = fopen(".neogit/currbranch", "r");

    if (file == NULL)
    {
        fprintf(stderr, "Error opening file .neogit/currbranch\n");
        return NULL;
    }

    char *branch = (char *)malloc(25);

    if (fgets(branch, 25, file) != NULL)
    {
        size_t length = strlen(branch);
        if (length > 0 && branch[length - 1] == '\n')
        {
            branch[length - 1] = '\0';
        }
    }
    else
    {
        fprintf(stderr, "Error reading from file .neogit/currbranch\n");
        free(branch);
        branch = NULL;
    }

    fclose(file);

    return branch;
}
bool is_neogit_initialized()
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror(RED "Error getting current working directory" RESET);
        return false;
    }

    do
    {
        DIR *dir = opendir(".neogit");
        if (dir != NULL)
        {
            closedir(dir);
            return true;
        }

        char parent_dir[1024];
        strcpy(parent_dir, dirname(cwd));

        if (strcmp(parent_dir, cwd) == 0)
        {
            break;
        }
        strcpy(cwd, parent_dir);

    } while (true);

    return false;
}
char *extractUsername()
{
    char *author = NULL;
    char line[MAX_LINE_LENGTH];
    FILE *configFile;

    if (UserNameIsGlobal())
    {
        configFile = fopen(".neogit/gconfig", "r");
    }
    else
    {
        configFile = fopen(".neogit/config", "r");
    }

    if (configFile == NULL)
    {
        perror(RED "Error opening config file" RESET);
        return NULL;
    }

    while (fgets(line, sizeof(line), configFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (UserNameIsGlobal() && strncmp(line, "global_username", 15) == 0)
        {
            char *start = strchr(line, ':');
            if (start != NULL)
            {
                start++; // Move past the ':'
                while (*start == ' ' || *start == '\t')
                {
                    start++; // Skip spaces or tabs
                }
                author = strdup(start);
                break;
            }
        }
        else if (!UserNameIsGlobal() && strncmp(line, "username:", 9) == 0)
        {
            char *start = strchr(line, ':');
            if (start != NULL)
            {
                start++; // Move past the ':'
                while (*start == ' ' || *start == '\t')
                {
                    start++; // Skip spaces or tabs
                }
                author = strdup(start);
                break;
            }
        }
    }

    fclose(configFile);
    return author;
}
int configLocal()
{
    FILE *file = fopen(".neogit/isglobal", "w");
    fprintf(file, "0");
    fclose(file);
    printf(GREEN "User is set to local!\n" RESET);
    return 0;
}
int findHighestFileNumberbranch(const char *branchname)
{
    char branchPath[200];
    snprintf(branchPath, sizeof(branchPath), ".neogit/branches/%s", branchname);

    DIR *dir = opendir(branchPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Error opening branch directory: %s\n", branchPath);
        return -1;
    }

    int highestNumber = -1;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
        {
            int commitID = atoi(ent->d_name);

            if (commitID > highestNumber)
            {
                highestNumber = commitID;
            }
        }
    }

    closedir(dir);

    return highestNumber;
}

///////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stdout, "Invalid command!\n");
        return 1;
    }


    else if (strcmp(argv[1], "init") == 0)
    {
        return run_init(argc, argv);
    }
    if (!is_neogit_initialized())
    {
        printf("Neogit has not been initialized yet!\n");
        return 1;
    }

    execute_alias(argv[1]);

    if (strcmp(argv[1], "config") == 0)
    {
        if (strcmp(argv[2], "-global") == 0)
        {
            if (strcmp(argv[3], "user.name") == 0)
            {
                return config(argv[4], "", 1);
            }
            else if (strcmp(argv[3], "user.email") == 0)
            {
                return config("", argv[4], 1);
            }
        }
        else if (strncmp(argv[2], "alias.", 6) == 0)
        {
            if (argc >= 4)
            {
                const char *alias_with_prefix = argv[2];
                const char *alias_name = alias_with_prefix + 6;
                const char *command = argv[3];
                add_alias(alias_name, command);
                return 0;
            }
            else
            {
                fprintf(stdout, "Invalid alias configuration!\n");
                return 1;
            }
        }
        else if (strcmp(argv[2], "-local") == 0)
        {
            return configLocal();
        }
    }

    else if (strcmp(argv[1], "add") == 0)
    {
        if (strcmp(argv[2], "-n") != 0)
        {
            return run_add(argc, argv);
        }
        else if (strcmp(argv[2], "-n") == 0)
        {
            return status();
        }
    }
    else if (strcmp(argv[1], "reset") == 0)
    {
        if (argc > 2 && strcmp(argv[2], "-undo") == 0)
        {
            return undo_last_add();
        }
        else
        {
            return run_reset(argc, argv);
        }
    }
    else if ((strcmp(argv[1], "status") == 0))
    {
        return status();
    }
    else if (strcmp(argv[1], "commit") == 0)
    {
        if (argc == 4 && strcmp(argv[2], "-m") == 0)
        {
            const char *message = argv[3];

            const char *shortcut_message = get_shortcut_message(message);
            if (shortcut_message != NULL)
            {
                strcpy(argv[3], get_shortcut_message(argv[3]));
            }
            return run_commit(argc, argv);
        }
        else
        {
            fprintf(stdout, "Invalid commit command!\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "log") == 0)
    {
        if (argc >= 4 && strcmp(argv[2], "-n") == 0)
        {
            int n = atoi(argv[3]);
            return showlogn(n);
        }
        else if (argc >= 4 && strcmp(argv[2], "-branch") == 0)
        {
            return showlogBranch(argv[3]);
        }
        else if (argc >= 4 && strcmp(argv[2], "-author") == 0)
        {
            return showlogAuthor(argv[3]);
        }
        else if (argc >= 4 && strcmp(argv[2], "-before") == 0)
        {
            return logBefore(argv[3]);
        }
        else if (argc >= 4 && strcmp(argv[2], "-since") == 0)
        {
            return logSince(argv[3]);
        }
        else if (argc >= 4 && strcmp(argv[2], "-search") == 0)
        {
            return logsearch(argv[3]);
        }
        return showlog();
    }
    else if (strcmp(argv[1], "branch") == 0)
    {
        return branch(argv[2]);
    }
    else if (strcmp(argv[1], "branches") == 0)
    {
        return showBranches();
    }
    else if (strcmp(argv[1], "checkout") == 0)
    {
        return checkout(argv[2]);
    }
    else if (strcmp(argv[1], "set") == 0)
    {
        if (argc == 6 && strcmp(argv[2], "-m") == 0 && strcmp(argv[4], "-s") == 0)
        {
            const char *shortcut_message = argv[3];
            const char *shortcut_name = argv[5];
            add_shortcut(shortcut_name, shortcut_message);
            printf(GREEN "Shortcut set successfully: " RESET "%s -> %s\n", shortcut_name, shortcut_message);
            return 0;
        }
        else
        {
            fprintf(stdout, "Invalid set command!\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "replace") == 0)
    {
        if (strcmp(argv[2], "-m") == 0 && strcmp(argv[4], "-s") == 0)
        {
            return replace_shortcut(argv[5], argv[3]);
        }
    }
    else if (strcmp(argv[1], "remove") == 0 && strcmp(argv[2], "-s") == 0)
    {
        return remove_shortcut(argv[3]);
    }
    else if (strcmp(argv[1], "diff") == 0)
    {
        const char *file1 = argv[3];
        const char *file2 = argv[4];
        int line1_start = 0, line1_end = 0, line2_start = 0, line2_end = 0;

        for (int i = 5; i < argc; i++)
        {
            if (strcmp(argv[i], "-line1") == 0 && i + 1 < argc)
            {
                sscanf(argv[i + 1], "%d-%d", &line1_start, &line1_end);
                i++;
            }
            else if (strcmp(argv[i], "-line2") == 0 && i + 1 < argc)
            {
                sscanf(argv[i + 1], "%d-%d", &line2_start, &line2_end);
                i++;
            }
        }

        return neogit_diff(file1, file2, line1_start, line1_end, line2_start, line2_end);
    }
    else if (strcmp(argv[1], "tag") == 0 && strcmp(argv[2], "show") != 0)
    {
        if (argc < 3)
        {
            fprintf(stdout, "Invalid tag command!\n");
            return 1;
        }

        const char *tag_name = argv[2];
        const char *message = NULL;
        int custom_commit_id = -1;
        int force = 0;

        for (int i = 3; i < argc; i++)
        {
            if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            {
                message = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            {
                custom_commit_id = atoi(argv[i + 1]);
                i++;
            }
            else if (strcmp(argv[i], "-f") == 0)
            {
                force = 1;
            }
        }

        return create_tag(tag_name, message, custom_commit_id != -1, custom_commit_id, force);
    }
    else if (strcmp(argv[1], "tags") == 0)
    {
        return showAllTags();
    }
    else if (strcmp(argv[1], "tag") == 0 && strcmp(argv[2], "show") == 0)
    {
        return showTagContent(argv[3]);
    }
    else if (argc >= 6 && strcmp(argv[1], "grep") == 0 && strcmp(argv[2], "-f") == 0 && strcmp(argv[4], "-p") == 0)
    {
        const char *file = argv[3];
        const char *word = argv[5];
        const char *commitId = NULL;
        int printLineNumber = 0;

        for (int i = 6; i < argc; i++)
        {
            if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            {
                commitId = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-n") == 0)
            {
                printLineNumber = 1;
            }
        }

        return neogit_grep(file, word, commitId, printLineNumber);
    }

    fprintf(stdout, "Invalid command!\n");
    return 1;
}

///////////////////////////////////////////////////

// config & init
int config(char *username, char *email, int isGlobal)
{
    FILE *file = fopen(GCONFIG_FILE, "r");
    FILE *tempFile = fopen(TMP_FILE, "w");

    if (file == NULL || tempFile == NULL)
    {
        perror(RED "Error opening config file" RESET);
        return 1;
    }

    char line[256];
    char newLine[256];
    char prefix[20];
    int found = 0;

    sprintf(prefix, "global_%s", strcmp(username, "") == 0 ? "email" : "username");

    while (fgets(line, sizeof(line), file))
    {
        if (strstr(line, prefix) != NULL)
        {
            found = 1;
            break;
        }
    }

    fseek(file, 0, SEEK_SET);

    if (isGlobal)
    {
        while (fgets(line, sizeof(line), file))
        {
            if (strstr(line, prefix) != NULL)
            {
                sprintf(newLine, "%s : %s\n", prefix, strcmp(username, "") == 0 ? email : username);
                fprintf(tempFile, "%s", newLine);
                found = 1;
            }
            else
            {
                fprintf(tempFile, "%s", line);
            }
        }

        if (!found)
        {
            fprintf(tempFile, "%s : %s\n", prefix, strcmp(username, "") == 0 ? email : username);
        }
    }

    fclose(file);
    fclose(tempFile);

    if (rename(TMP_FILE, GCONFIG_FILE) != 0)
    {
        perror(RED "Error replacing config file" RESET);
        return 1;
    }

    if (found)
    {
        printf(GREEN "Global %s is updated!\n", strcmp(username, "") == 0 ? "email" : "username" RESET);
    }
    else if (strcmp(username, "") == 0)
    {
        printf(GREEN "Global email is set!\n" RESET);
    }
    else
    {
        printf(GREEN "Global username is set!\n" RESET);
        FILE *file = fopen(".neogit/isglobal", "w");
        fprintf(file, "1");
        fclose(file);
    }

    return 0;
}
int create_configs(char *username, char *email)
{
    FILE *file = fopen(".neogit/config", "w");
    if (file == NULL)
        return 1;

    fprintf(file, "username: %s\n", username);
    fprintf(file, "email: %s\n", email);
    fprintf(file, "last_commit_ID: %d\n", 0);
    fprintf(file, "current_commit_ID: %d\n", 0);
    fprintf(file, "branch: %s", "master");
    fclose(file);

    if (mkdir(".neogit/files", 0755) != 0)
        return 1;

    if (mkdir(".neogit/commits", 0755) != 0)
        return 1;

    if (mkdir(".neogit/branches", 0755) != 0)
        return 1;

    if (mkdir(".neogit/tags", 0755) != 0)
        return 1;

    file = fopen(".neogit/staging", "w");
    fclose(file);

    file = fopen(".neogit/tracks", "w");
    fclose(file);

    file = fopen(".neogit/allAdds", "w");
    fclose(file);

    file = fopen(".neogit/shortcuts", "w");
    fclose(file);

    file = fopen(".neogit/alias", "w");
    fclose(file);

    file = fopen(".neogit/gconfig", "w");
    fclose(file);

    file = fopen(".neogit/isglobal", "w");
    fprintf(file, "0");
    fclose(file);

    file = fopen(".neogit/currbranch", "w");
    fprintf(file, "master");
    fclose(file);

    branch(master);
    return 0;
}
int get_user_config(char *username, char *email, int isGlobal)
{
    if (isGlobal)
    {
        printf("Enter global username: ");
        scanf("%s", username);
        printf("Enter global email: ");
        scanf("%s", email);
    }
    else
    {
        printf("Enter local username: ");
        scanf("%s", username);
        printf("Enter local email: ");
        scanf("%s", email);
    }

    return 0;
}
int run_init(int argc, char *const argv[])
{
    char cwd[1024];
    char tmp_cwd[1024];
    int exists = 0;
    struct dirent *entry;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 1;

    do
    {
        DIR *dir = opendir(".");
        if (dir == NULL)
        {
            perror(RED "We had an error trying to open the current directory. Please try again !\n" RESET);
            return 1;
        }
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".neogit") == 0)
                exists = 1;
        }
        closedir(dir);

        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL)
            return 1;

        if (strcmp(tmp_cwd, "/") != 0)
        {
            if (chdir("..") != 0)
                return 1;
        }

    } while (strcmp(tmp_cwd, "/") != 0);

    if (chdir(cwd) != 0)
        return 1;

    if (!exists)
    {
        if (mkdir(".neogit", 0755) != 0)
            return 1;

        char username[1000];
        char email[1000];
        int isGlobal = 0;

        if (argc > 2 && strcmp(argv[2], "-global") == 0)
        {
            isGlobal = 1;
        }

        if (get_user_config(username, email, isGlobal) != 0)
        {
            perror(RED "Error getting user configuration" RESET);
            return 1;
        }
        printf(GREEN "Neogit initialized successfully!\n" RESET);

        return create_configs(username, email);
    }
    else
    {
        printf(RED "Neogit repository has already been initialized" RESET);
    }
    return 0;
}

// add

int run_add(int argc, char *const argv[])
{
    if (argc < 3)
    {
        perror(RED "Please specify at least one file or directory" RESET);
        return 1;
    }

    int forceFlag = 0;
    if (argc > 3 && strcmp(argv[2], "-f") == 0)
    {
        forceFlag = 1;
    }
    if (argc > 3 && strcmp(argv[2], "-f") != 0)
    {
        printf(RED "You need to use neogit add -f in order to add multiple files!\n" RESET);
        return 0;
    }

    for (int i = forceFlag ? 3 : 2; i < argc; ++i)
    {
        glob_t glob_result;
        if (glob(argv[i], GLOB_ERR | GLOB_MARK, NULL, &glob_result) == 0)
        {
            for (size_t j = 0; j < glob_result.gl_pathc; ++j)
            {
                if (add_to_staging(glob_result.gl_pathv[j]) != 0 && !forceFlag)
                {
                    fprintf(stderr, RED "%s doesn't exist! \n" RESET, glob_result.gl_pathv[j]);
                }
            }
            globfree(&glob_result);
        }
        else
        {
            if (!forceFlag)
            {
                fprintf(stderr, RED "Error expanding wildcard pattern for %s\n" RESET, argv[i]);
            }
        }
    }

    return 0;
}
int is_directory(char *path)
{
    DIR *dir = opendir(path);
    if (dir != NULL)
    {
        closedir(dir);
        return 1;
    }
    return 0;
}
int add_to_staging(char *path)
{
    if (access(path, F_OK) == -1)
    {
        return 1;
    }

    if (is_directory(path))
    {
        if (add_directory_contents(path) != 0)
        {
            return 1;
        }
    }
    else
    {
        FILE *file = fopen(".neogit/staging", "r");
        if (file == NULL)
            return 1;

        char command[1000];
        while (fgets(command, sizeof(command), file) != NULL)
        {
            int length = strlen(command);

            if (length > 0 && command[length - 1] == '\n')
            {
                command[length - 1] = '\0';
            }

            if (strcmp(path, command) == 0)
            {
                fclose(file);
                return 0;
            }
        }
        fclose(file);

        file = fopen(".neogit/staging", "a");
        if (file == NULL)
            return 1;

        mkdir(".neogit/StagingAreaFiles", 0755);
        FILE *source, *destination;
        char ch;

        source = fopen(path, "rb");
        char staged_filename[MAX_FILENAME_LENGTH];
        sprintf(staged_filename, ".neogit/StagingAreaFiles/%s", basename(path));
        destination = fopen(staged_filename, "wb");

        while ((ch = fgetc(source)) != EOF)
        {
            fputc(ch, destination);
        }

        fclose(source);
        fclose(destination);

        fprintf(file, "%s\n", path);
        printf(GREEN "%s added to staging area!\n" RESET, path);
        fclose(file);
        // add to allAdds
        file = fopen(".neogit/allAdds", "a");
        if (file == NULL)
            return 1;

        fprintf(file, "%s\n", path);
        fclose(file);
        // add to tracks
        file = fopen(".neogit/tracks", "a");
        if (file == NULL)
            return 1;

        fprintf(file, "%s\n", path);
        fclose(file);
    }

    return 0;
}
int add_directory_contents(char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror(RED "Error opening directory" RESET);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char entry_path[1000];
            snprintf(entry_path, 1000, "%s%s", dir_path, entry->d_name);
            if (add_to_staging(entry_path) != 0)
            {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// function to compare

int compareFiles(const char *file1Path, const char *file2Path)
{
    FILE *file1 = fopen(file1Path, "rb");
    FILE *file2 = fopen(file2Path, "rb");

    if (file1 == NULL || file2 == NULL)
    {
        perror(RED "Error opening files" RESET);
        return -1;
    }

    fseek(file1, 0, SEEK_END);
    long fileSize1 = ftell(file1);
    fseek(file1, 0, SEEK_SET);

    fseek(file2, 0, SEEK_END);
    long fileSize2 = ftell(file2);
    fseek(file2, 0, SEEK_SET);

    if (fileSize1 != fileSize2)
    {
        fclose(file1);
        fclose(file2);
        return 1;
    }

    char *buffer1 = malloc(fileSize1);
    char *buffer2 = malloc(fileSize2);

    if (buffer1 == NULL || buffer2 == NULL)
    {
        perror(RED "Error allocating memory" RESET);
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

int run_reset(int argc, char *const argv[])
{
    if (argc < 3)
    {
        perror(RED "Please specify at least one file or directory" RESET);
        return 1;
    }
    int forceFlag = 0;
    if (argc > 3 && strcmp(argv[2], "-f") == 0)
    {
        forceFlag = 1;
    }
    for (int i = forceFlag ? 3 : 2; i < argc; ++i)
    {
        glob_t glob_result;
        if (glob(argv[i], GLOB_ERR | GLOB_MARK, NULL, &glob_result) == 0)
        {
            for (size_t j = 0; j < glob_result.gl_pathc; ++j)
            {
                if (reset_file(glob_result.gl_pathv[j]) != 0 && !forceFlag)
                {
                    fprintf(stderr, RED "%s doesn't exist in the staging area!\n" RESET, glob_result.gl_pathv[j]);
                }
            }
            globfree(&glob_result);
        }
        else
        {
            if (!forceFlag)
            {
                fprintf(stderr, RED "Error expanding wildcard pattern for %s\n" RESET, argv[i]);
            }
        }
    }

    return 0;
}
int reset_file(char *filepath)
{
    FILE *staging_file = fopen(".neogit/staging", "r");
    if (staging_file == NULL)
        return 1;

    FILE *tmp_file = fopen(".neogit/tmp_staging", "w");
    if (tmp_file == NULL)
    {
        fclose(staging_file);
        return 1;
    }

    FILE *allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL)
    {
        fclose(staging_file);
        fclose(tmp_file);
        return 1;
    }

    int found = 0;
    char line[1000];

    // Iterate through the staging file
    while (fgets(line, sizeof(line), staging_file) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (strcmp(filepath, line) == 0)
        {
            found = 1;
        }
        else
        {
            fprintf(tmp_file, "%s\n", line);
        }
    }

    char staged_filename[MAX_FILENAME_LENGTH];
    sprintf(staged_filename, ".neogit/StagingAreaFiles/%s", basename(filepath));
    remove(staged_filename);

    fclose(staging_file);
    fclose(tmp_file);

    if (!found)
    {
        // If the file was not found in the staging area, clean up and return an error
        remove(".neogit/tmp_staging");
        fclose(allAddsFile);
        fprintf(stderr, RED "%s is not in the staging area!\n" RESET, filepath);
        return 1;
    }

    remove(".neogit/staging");
    rename(".neogit/tmp_staging", ".neogit/staging");

    // Open the allAdds file for writing
    FILE *newAllAddsFile = fopen(".neogit/tmp_allAdds", "w");
    if (newAllAddsFile == NULL)
    {
        fclose(allAddsFile);
        fprintf(stderr, RED "Error opening tmp_allAdds file for writing.\n" RESET);
        return 1;
    }

    while (fgets(line, sizeof(line), allAddsFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        int entry_number;
        if (sscanf(line, "%d . %[^\n]", &entry_number, entry_filepath) != 2)
        {
            continue; // Skip invalid lines
        }

        if (strcmp(filepath, entry_filepath) == 0)
        {
            found = 1;
        }
        else
        {
            fprintf(newAllAddsFile, "%d . %s\n", entry_number, entry_filepath);
        }
    }

    fclose(allAddsFile);
    fclose(newAllAddsFile);

    if (!found)
    {
        remove(".neogit/tmp_allAdds");
        fprintf(stderr, RED "%s is not in the allAdds file!\n" RESET, filepath);
        return 1;
    }

    remove(".neogit/allAdds");
    rename(".neogit/tmp_allAdds", ".neogit/allAdds");

    printf(GREEN "%s removed from the staging area.\n" RESET, filepath);
    return 0;
}
int undo_last_add()
{
    FILE *allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL)
    {
        fprintf(stderr, RED "Error opening allAdds file for reading.\n" RESET);
        return 1;
    }

    char lastAddedFile[1000];
    char lastAddedLine[1000];

    char line[1000];
    while (fgets(line, sizeof(line), allAddsFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        if (sscanf(line, "%[^\n]", entry_filepath) == 1)
        {
            strcpy(lastAddedFile, entry_filepath);
            strcpy(lastAddedLine, line);
        }
    }

    fclose(allAddsFile);

    if (lastAddedFile[0] == '\0')
    {
        fprintf(stderr, RED "No files to undo in the allAdds file.\n" RESET);
        return 1;
    }

    char paath[200];
    sprintf(paath, ".neogit/StagingAreaFiles/%s", lastAddedFile);
    remove(paath);

    printf(GREEN "Undo: Removing %s from the staging area.\n" RESET, lastAddedFile);

    FILE *newAllAddsFile = fopen(".neogit/tmp_allAdds", "w");
    if (newAllAddsFile == NULL)
    {
        fprintf(stderr, RED "Error opening tmp_allAdds file for writing.\n" RESET);
        return 1;
    }

    allAddsFile = fopen(".neogit/allAdds", "r");
    if (allAddsFile == NULL)
    {
        fclose(newAllAddsFile);
        fprintf(stderr, RED "Error opening allAdds file for reading.\n" RESET);
        return 1;
    }

    while (fgets(line, sizeof(line), allAddsFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        char entry_filepath[1000];
        if (sscanf(line, "%[^\n]", entry_filepath) == 1)
        {
            // Check if the entry matches the last added file
            if (strcmp(entry_filepath, lastAddedFile) != 0)
            {
                fprintf(newAllAddsFile, "%s\n", entry_filepath);
            }
        }
    }

    fclose(allAddsFile);
    fclose(newAllAddsFile);

    remove(".neogit/allAdds");
    rename(".neogit/tmp_allAdds", ".neogit/allAdds");

    FILE *stagingFile = fopen(".neogit/staging", "r");
    if (stagingFile == NULL)
    {
        fprintf(stderr, RED "Error opening staging file for reading.\n" RESET);
        return 1;
    }

    FILE *newStagingFile = fopen(".neogit/tmp_staging", "w");
    if (newStagingFile == NULL)
    {
        fclose(stagingFile);
        fprintf(stderr, RED "Error opening tmp_staging file for writing.\n" RESET);
        return 1;
    }

    while (fgets(line, sizeof(line), stagingFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (strcmp(line, lastAddedLine) != 0)
        {
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

int is_file_in_staging(const char *filename)
{
    FILE *staging_file = fopen(".neogit/staging", "r");
    if (staging_file == NULL)
    {
        perror(RED "Error opening staging file for reading" RESET);
        return 0;
    }

    char line[1000];
    while (fgets(line, sizeof(line), staging_file) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (strcmp(filename, line) == 0)
        {
            fclose(staging_file);
            return 1; // File is in staging
        }
    }

    fclose(staging_file);
    return 0; // File is not in staging
}
int status()
{
    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        perror(RED "Error opening directory" RESET);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        { // Check if it's a regular file
            printf("%s %s\n", entry->d_name, is_file_in_staging(entry->d_name) ? "+" : "-");
        }
    }

    closedir(dir);

    return 0;
}

// commit

int run_commit(int argc, char *const argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: neogit commit -m \"message\"\n");
        return 1;
    }

    if (strlen(argv[3]) > 73)
    {
        fprintf(stderr, "Your commit message is too long! (Your message can be 72 characters long)\n");
        return 1;
    }

    char message[MAX_MESSAGE_LENGTH];
    strncpy(message, argv[3], MAX_MESSAGE_LENGTH);

    int commit_ID = inc_last_commit_ID();
    if (commit_ID == -1)
    {
        fprintf(stderr, "Error incrementing commit ID\n");
        return 1;
    }

    FILE *stagingFile = fopen(".neogit/staging", "r");
    if (stagingFile == NULL)
    {
        fprintf(stderr, "Error opening staging file\n");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), stagingFile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        char stagedFilePath[MAX_FILENAME_LENGTH];
        sprintf(stagedFilePath, ".neogit/StagingAreaFiles/%s", basename(line));

        if (compareFiles(line, stagedFilePath) != 0)
        {
            fprintf(stdout, "Changes detected in file: %s\n", line);
            commit_staged_file(commit_ID, stagedFilePath);
            track_file(line);
        }
    }

    fclose(stagingFile);

    FILE *file = fopen(".neogit/staging", "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening staging file\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (!check_file_directory_exists(line))
        {
            char dir_path[MAX_FILENAME_LENGTH];
            strcpy(dir_path, ".neogit/files/");
            strcat(dir_path, line);
            if (mkdir(dir_path, 0755) != 0)
            {
                fprintf(stderr, "Error creating directory: %s\n", dir_path);
                return 1;
            }
        }
        commit_staged_file(commit_ID, line);
        track_file(line);
    }
    fclose(file);

    file = fopen(".neogit/staging", "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening staging file for writing\n");
        return 1;
    }
    fclose(file);

    file = fopen(".neogit/allAdds", "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening allAdds file for writing\n");
        return 1;
    }
    fclose(file);

    create_commit_file(commit_ID, message);
    fprintf(stdout, GREEN "Commit successfully with commit ID %d\n" RESET, commit_ID);

    return 0;
}
int inc_last_commit_ID()
{
    FILE *file = fopen(".neogit/config", "r");
    if (file == NULL)
        return -1;

    FILE *tmp_file = fopen(".neogit/tmp_config", "w");
    if (tmp_file == NULL)
        return -1;

    int last_commit_ID;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(line, "last_commit_ID", 14) == 0)
        {
            sscanf(line, "last_commit_ID: %d\n", &last_commit_ID);
            last_commit_ID++;
            fprintf(tmp_file, "last_commit_ID: %d\n", last_commit_ID);
        }
        else
            fprintf(tmp_file, "%s", line);
    }
    fclose(file);
    fclose(tmp_file);

    remove(".neogit/config");
    rename(".neogit/tmp_config", ".neogit/config");
    return last_commit_ID;
}
bool check_file_directory_exists(char *filepath)
{
    DIR *dir = opendir(".neogit/files");
    struct dirent *entry;
    if (dir == NULL)
    {
        perror(RED "Error opening current directory" RESET);
        return 1;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, filepath) == 0)
            return true;
    }
    closedir(dir);

    return false;
}
int commit_staged_file(int commit_ID, char *filepath)
{
    FILE *read_file, *write_file;
    char read_path[MAX_FILENAME_LENGTH];
    strcpy(read_path, filepath);
    char write_path[MAX_FILENAME_LENGTH];
    strcpy(write_path, ".neogit/files/");
    strcat(write_path, filepath);
    strcat(write_path, "/");
    char tmp[10];
    sprintf(tmp, "%d", commit_ID);
    strcat(write_path, tmp);

    read_file = fopen(read_path, "r");
    if (read_file == NULL)
        return 1;

    write_file = fopen(write_path, "w");
    if (write_file == NULL)
        return 1;

    char buffer;
    buffer = fgetc(read_file);
    while (buffer != EOF)
    {
        fputc(buffer, write_file);
        buffer = fgetc(read_file);
    }
    fclose(read_file);
    fclose(write_file);

    char staged_filename[MAX_FILENAME_LENGTH];
    sprintf(staged_filename, ".neogit/StagingAreaFiles/%s", basename(filepath));
    remove(staged_filename);

    return 0;
}
int track_file(char *filepath)
{
    if (is_tracked(filepath))
        return 0;

    FILE *file = fopen(".neogit/tracks", "a");
    if (file == NULL)
        return 1;
    fprintf(file, "%s\n", filepath);
    return 0;
}
bool is_tracked(char *filepath)
{
    FILE *file = fopen(".neogit/tracks", "r");
    if (file == NULL)
        return false;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (strcmp(line, filepath) == 0)
            return true;
    }
    fclose(file);

    return false;
}
int create_commit_file(int commit_ID, char *message)
{
    // Create commit file in .neogit/commits
    char commit_filepath[MAX_FILENAME_LENGTH];
    snprintf(commit_filepath, sizeof(commit_filepath), ".neogit/commits/%d", commit_ID);

    FILE *file = fopen(commit_filepath, "w");
    if (file == NULL)
    {
        perror(RED "Error creating commit file in .neogit/commits" RESET);
        return 1;
    }

    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    char datetime_str[20];
    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(file, "datetime: %s\n", datetime_str);
    fprintf(file, "message: %s\n", message);
    fprintf(file, "branch: %s\n", get_current_branch());
    fprintf(file, "author: %s\n", extractUsername());
    fprintf(file, "files:\n");

    DIR *dir = opendir(".");
    struct dirent *entry;
    if (dir == NULL)
    {
        perror(RED "Error opening current directory" RESET);
        fclose(file);
        return 1;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && is_tracked(entry->d_name))
        {
            int file_last_commit_ID = find_file_last_commit(entry->d_name);
            fprintf(file, "%s %d\n", entry->d_name, file_last_commit_ID);
        }
    }
    closedir(dir);

    fclose(file);

    char branchPath[MAX_FILENAME_LENGTH];
    char *branchName = get_current_branch();
    snprintf(branchPath, sizeof(branchPath), ".neogit/branches/%s/%d", branchName, commit_ID);

    file = fopen(branchPath, "w");
    if (file == NULL)
    {
        perror(RED "Error creating commit file in current branch" RESET);
        return 1;
    }

    fprintf(file, "datetime: %s\n", datetime_str);
    fprintf(file, "message: %s\n", message);
    fprintf(file, "branch: %s\n", get_current_branch());
    fprintf(file, "author: %s\n", extractUsername());
    fprintf(file, "files:\n");

    fclose(file);

    return 0;
}
int find_file_last_commit(char *filepath)
{
    char filepath_dir[MAX_FILENAME_LENGTH];
    strcpy(filepath_dir, ".neogit/files/");
    strcat(filepath_dir, filepath);

    int max = -1;

    DIR *dir = opendir(filepath_dir);
    struct dirent *entry;
    if (dir == NULL)
        return 1;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            int tmp = atoi(entry->d_name);
            max = max > tmp ? max : tmp;
        }
    }
    closedir(dir);

    return max;
}

// log

int findHighestFileNumber()
{
    DIR *dir;
    struct dirent *ent;
    int highestNumber = 0;

    if ((dir = opendir(".neogit/commits")) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG)
            {
                int currentNumber = atoi(ent->d_name);

                if (currentNumber > highestNumber)
                {
                    highestNumber = currentNumber;
                }
            }
        }

        closedir(dir);
    }
    else
    {
        perror(RED "Error opening directory" RESET);
        return -1; // Error opening directory
    }

    return highestNumber;
}
int showlog()
{

    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    for (int i = commitnumb; i >= 1; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char message[200];
        char personThatCommitted[200];
        char CommitId[200];
        char branch[200];
        int numbOfCommittedFiles = 0;

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, RED "Error opening file: %s\n" RESET, dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            int length = strlen(line);

            if (length > 0 && line[length - 1] == '\n')
            {
                line[length - 1] = '\0';
            }

            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                printf(BLUE "Date and Time of commit:" RESET " %s %s\n", dateOfCommit, hour);
            }
            else if (strncmp(line, "branch:", 7) == 0)
            {
                sscanf(line, "branch: %s", branch);
                printf(BLUE "Branch :" RESET " %s\n", branch);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
                printf(BLUE "Commit message:" RESET " %s\n", message);
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
                printf(BLUE "Author:" RESET " %s\n", personThatCommitted);
            }
            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }

                    int fileCommitID;
                    char fileName[MAX_FILENAME_LENGTH];
                    sscanf(line, "%s %d", fileName, &fileCommitID);

                    if (fileCommitID == i)
                    {
                        numbOfCommittedFiles++;
                    }
                }
                printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            }
        }

        printf(BLUE "Commit ID:" RESET " %d\n", i);
        printf("\n");
        fclose(file);
    }

    return 0;
}
int showlogn(int n)
{

    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    for (int i = commitnumb; i > commitnumb - n; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char message[200];
        char personThatCommitted[200];
        char CommitId[200];
        char branch[200];
        int numbOfCommittedFiles = 0;

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            int length = strlen(line);

            if (length > 0 && line[length - 1] == '\n')
            {
                line[length - 1] = '\0';
            }

            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                printf(BLUE "Date and Time of commit:" RESET " %s %s\n", dateOfCommit, hour);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
                printf(BLUE "Commit message:" RESET " %s\n", message);
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
            }
            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
                printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            }
        }

        printf(BLUE "Author of commit:" RESET " %s\n", personThatCommitted);
        printf(BLUE "Commit ID:" RESET " %d\n", i);
        printf("\n");
        fclose(file);
    }

    return 0;
}
int showlogBranch(char *branchname)
{
    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    int branchFound = 0;

    for (int i = commitnumb; i >= 1; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char message[200];
        char personThatCommitted[200];
        char CommitId[200];
        char branch[200];
        int numbOfCommittedFiles = 0;

        FILE *files;
        char dests[200];
        snprintf(dests, sizeof(dests), ".neogit/commits/%d", i);

        files = fopen(dests, "r");

        if (files == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dests);
            return 1;
        }

        char linec[MAX_LINE_LENGTH];

        while (fgets(linec, sizeof(linec), files) != NULL)
        {
            int length = strlen(linec);

            if (length > 0 && linec[length - 1] == '\n')
            {
                linec[length - 1] = '\0';
            }

            if (strncmp(linec, "branch:", 7) == 0)
            {
                sscanf(linec, "branch: %s", branch);
            }
        }

        fclose(files);

        if (strcmp(branch, branchname) != 0)
        {
            continue;
        }
        else
        {
            branchFound = 1;
        }

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            int length = strlen(line);

            if (length > 0 && line[length - 1] == '\n')
            {
                line[length - 1] = '\0';
            }

            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                printf(BLUE "Date and Time of commit:" RESET " %s %s\n", dateOfCommit, hour);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
                printf(BLUE "Commit message:" RESET " %s\n", message);
                printf(BLUE "branch:" RESET " %s\n", branch);
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
            }

            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
                printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            }
        }

        printf(BLUE "Author of commit:" RESET " %s\n", personThatCommitted);
        printf(BLUE "Commit ID:" RESET " %d\n", i);
        printf("\n");
        fclose(file);
    }

    if (!branchFound)
    {
        fprintf(stderr, "Error: Branch '%s' not found!\n", branchname);
        return 1;
    }

    return 0;
}
int showlogAuthor(char *author)
{
    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }
    for (int i = commitnumb; i >= 1; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char message[200];
        char personThatCommitted[200];
        char CommitId[200];
        char branch[200];
        int numbOfCommittedFiles = 0;

        FILE *files;
        char dests[200];
        snprintf(dests, sizeof(dests), ".neogit/commits/%d", i);

        files = fopen(dests, "r");

        if (files == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dests);
            return 1;
        }

        char linec[MAX_LINE_LENGTH];

        while (fgets(linec, sizeof(linec), files) != NULL)
        {
            int length = strlen(linec);

            if (length > 0 && linec[length - 1] == '\n')
            {
                linec[length - 1] = '\0';
            }

            if (strncmp(linec, "author:", 7) == 0)
            {
                sscanf(linec, "author: %s", personThatCommitted);
            }
        }

        if (strcmp(author, personThatCommitted) != 0)
        {
            continue;
        }

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            int length = strlen(line);

            if (length > 0 && line[length - 1] == '\n')
            {
                line[length - 1] = '\0';
            }

            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                printf(BLUE "Date and Time of commit:" RESET " %s %s\n", dateOfCommit, hour);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
                printf(BLUE "Commit message:" RESET " %s\n", message);
            }

            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
                printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            }
        }

        printf(BLUE "Author of commit:" RESET " %s\n", author);
        printf(BLUE "Commit ID:" RESET " %d\n", i);
        printf("\n");
        fclose(file);
    }

    return 0;
}
int logBefore(char *targetDate)
{
    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    struct tm targetTime;
    if (strptime(targetDate, "%Y-%m-%d", &targetTime) == NULL)
    {
        fprintf(stderr, "Invalid date format. Use YYYY-MM-DD.\n");
        return 1;
    }

    for (int i = commitnumb; i >= 1; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char personThatCommitted[200];
        char branch[200];
        char message[200];
        int numbOfCommittedFiles = 0;

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];
        struct tm commitTime = {0};

        while (fgets(line, sizeof(line), file) != NULL)
        {
            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                strptime(dateOfCommit, "%Y-%m-%d", &commitTime);
            }
            else if (strncmp(line, "branch:", 7) == 0)
            {
                sscanf(line, "branch: %s", branch);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
            }
            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
            }
        }

        fclose(file);

        if (mktime(&commitTime) < mktime(&targetTime))
        {
            printf(BLUE "Commit ID:" RESET " %d\n", i);
            printf(BLUE "Branch:" RESET " %s\n", branch);
            printf(BLUE "Author:" RESET " %s\n", personThatCommitted);
            printf(BLUE "Commit message:" RESET " %s\n", message);
            printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            printf(BLUE "Date and Time:" RESET " %s %s\n", dateOfCommit, hour);
        }
        printf("\n");
    }

    return 0;
}
int logSince(char *targetDate)
{
    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    struct tm targetTime;
    if (strptime(targetDate, "%Y-%m-%d", &targetTime) == NULL)
    {
        fprintf(stderr, "Invalid date format. Use YYYY-MM-DD.\n");
        return 1;
    }

    for (int i = commitnumb; i >= 1; i--)
    {
        printf("\n");
        char dateOfCommit[200];
        char hour[200];
        char personThatCommitted[200];
        char branch[200];
        char message[200];
        int numbOfCommittedFiles = 0;

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, "Error opening file: %s\n", dest);
            return 1;
        }

        char line[MAX_LINE_LENGTH];
        struct tm commitTime = {0};

        while (fgets(line, sizeof(line), file) != NULL)
        {
            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
                strptime(dateOfCommit, "%Y-%m-%d", &commitTime);
            }
            else if (strncmp(line, "branch:", 7) == 0)
            {
                sscanf(line, "branch: %s", branch);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
            }
            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
            }
        }

        fclose(file);

        if (mktime(&commitTime) >= mktime(&targetTime))
        {
            printf(BLUE "Commit ID:" RESET " %d\n", i);
            printf(BLUE "Branch:" RESET " %s\n", branch);
            printf(BLUE "Author:" RESET " %s\n", personThatCommitted);
            printf(BLUE "Commit message:" RESET " %s\n", message);
            printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            printf(BLUE "Date and Time:" RESET " %s %s\n", dateOfCommit, hour);
        }
        printf("\n");
    }

    return 0;
}
int logsearch(char *word)
{
    int commitnumb = findHighestFileNumber();

    if (!commitnumb)
    {
        printf("No commits found!\n");
        return 1;
    }

    for (int i = commitnumb; i >= 1; i--)
    {
        char dateOfCommit[200];
        char hour[200];
        char message[200];
        char personThatCommitted[200];
        char branch[200];
        int numbOfCommittedFiles = 0;

        FILE *file;
        char dest[200];
        snprintf(dest, sizeof(dest), ".neogit/commits/%d", i);

        file = fopen(dest, "r");

        if (file == NULL)
        {
            fprintf(stderr, RED "Error opening file: %s\n" RESET, dest);
            return 1;
        }

        bool commitContainsWord = false;

        char line[MAX_LINE_LENGTH];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            int length = strlen(line);

            if (length > 0 && line[length - 1] == '\n')
            {
                line[length - 1] = '\0';
            }

            if (strncmp(line, "datetime:", 9) == 0)
            {
                sscanf(line, "datetime: %s %s", dateOfCommit, hour);
            }
            else if (strncmp(line, "branch:", 7) == 0)
            {
                sscanf(line, "branch: %s", branch);
            }
            else if (strncmp(line, "message:", 8) == 0)
            {
                sscanf(line, "message: %[^\n]", message);

                char *wordPosition = strstr(message, word);
                if (wordPosition != NULL)
                {
                    commitContainsWord = true;
                    printf("\n");
                    printf(BLUE "Commit message:" RESET " ");
                    printf("%.*s", (int)(wordPosition - message), message);
                    printf(YELLOW "%s" RESET, word);
                    printf("%s\n", wordPosition + strlen(word));
                }
            }
            else if (strncmp(line, "author:", 7) == 0)
            {
                sscanf(line, "author: %s", personThatCommitted);
            }
            else if (strncmp(line, "files:", 6) == 0)
            {
                while (fgets(line, sizeof(line), file) != NULL)
                {
                    if (line[0] == '\n' || line[0] == '\0')
                    {
                        break;
                    }
                    numbOfCommittedFiles++;
                }
            }
        }

        if (commitContainsWord)
        {
            printf(BLUE "Date and Time of commit:" RESET " %s %s\n", dateOfCommit, hour);
            printf(BLUE "Branch :" RESET " %s\n", branch);
            printf(BLUE "Author:" RESET " %s\n", personThatCommitted);
            printf(BLUE "Number of files in this commit:" RESET " %d\n", numbOfCommittedFiles);
            printf(BLUE "Commit ID:" RESET " %d\n", i);
            printf("\n");
        }

        fclose(file);
    }

    return 0;
}

// branch

void copyFolder(const char *src, const char *dest)
{
    char command[200];
    snprintf(command, sizeof(command), "cp -r %s %s", src, dest);
    system(command);
}
int branch(char *branchName)
{
    char branchPath[200];
    snprintf(branchPath, sizeof(branchPath), ".neogit/branches/%s", branchName);

    if (access(branchPath, F_OK) == 0)
    {
        fprintf(stderr, "Error: Branch '%s' already exists.\n", branchName);
        return 1;
    }

    if (mkdir(branchPath, 0755) != 0)
    {
        perror(RED "Error creating branch directory" RESET);
        return 1;
    }

    FILE *file = fopen(".neogit/currbranch", "w");
    if (file == NULL)
    {
        perror(RED "Error opening .neogit/currbranch for writing" RESET);
        return 1;
    }

    if (fprintf(file, "%s", branchName) < 0)
    {
        perror(RED "Error writing to .neogit/currbranch" RESET);
        fclose(file);
        return 1;
    }

    fclose(file);

    printf("Branch " YELLOW "%s" RESET " created successfully.\n", branchName);

    int highestFileNumber = findHighestFileNumber();

    if (highestFileNumber > 0)
    {
        char commitPath[200];
        snprintf(commitPath, sizeof(commitPath), ".neogit/commits/%d", highestFileNumber);

        copyFolder(commitPath, branchPath);

        printf(GREEN "Last commit added to branch '%s'.\n" RESET, branchPath);
    }
    else
    {
        printf("No commits found.\n");
    }
    printf("current branch ->" YELLOW " %s\n" RESET, get_current_branch());
    return 0;
}
int showBranches()
{
    DIR *dir;
    struct dirent *ent;
    const char *branchesDir = ".neogit/branches";
    if ((dir = opendir(branchesDir)) != NULL)
    {
        printf("Branches:\n");
        printf("- Master\n");
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_DIR && ent->d_name[0] != '.')
            {
                if (ent->d_name == get_current_branch())
                {
                    printf("- %s -> current branch\n", ent->d_name);
                }
                else
                {
                    printf("- %s\n", ent->d_name);
                }
                printf("%s", get_current_branch());
            }
        }
        closedir(dir);
    }
    else
    {
        perror(RED "Error opening branches directory" RESET);
    }
    return 0;
}

// checkout

int removeCommitsGreaterThan(int commitID) {
    DIR *dir;
    struct dirent *ent;

    dir = opendir(".neogit/branches");
    if (dir == NULL) {
        perror(RED "Error opening branches directory" RESET);
        return 1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            char branchPath[MAX_FILENAME_LENGTH];
            snprintf(branchPath, sizeof(branchPath), ".neogit/branches/%s", ent->d_name);

            DIR *branchDir = opendir(branchPath);
            if (branchDir == NULL) {
                perror(RED "Error opening branch directory" RESET);
                closedir(dir);
                return 1;
            }

            struct dirent *branchEnt;
            while ((branchEnt = readdir(branchDir)) != NULL) {
                if (branchEnt->d_type == DT_REG) {
                    int fileCommitID = atoi(branchEnt->d_name);
                    if (fileCommitID > commitID) {
                        char commitPath[MAX_FILENAME_LENGTH];
                        snprintf(commitPath, sizeof(commitPath), "%s/%s", branchPath, branchEnt->d_name);
                        if (remove(commitPath) != 0) {
                            perror(RED "Error removing commit file" RESET);
                            closedir(branchDir);
                            closedir(dir);
                            return 1;
                        }
                    }
                }
            }
            closedir(branchDir);
        }
    }

    closedir(dir);

    DIR *commitsDir = opendir(".neogit/commits");
    if (commitsDir == NULL) {
        perror(RED "Error opening commits directory" RESET);
        return 1;
    }

    struct dirent *commitEnt;
    while ((commitEnt = readdir(commitsDir)) != NULL) {
        if (commitEnt->d_type == DT_REG) {
            int fileCommitID = atoi(commitEnt->d_name);
            if (fileCommitID > commitID) {
                char commitPath[MAX_FILENAME_LENGTH];
                snprintf(commitPath, sizeof(commitPath), ".neogit/commits/%s", commitEnt->d_name);
                if (remove(commitPath) != 0) {
                    perror(RED "Error removing commit file" RESET);
                    closedir(commitsDir);
                    return 1;
                }
            }
        }
    }

    closedir(commitsDir);
    return 0;
}
int checkout(char *branchname) {
    int commitID = atoi(branchname);
    if (commitID > 0) {
        printf("Checking out to commit ID: %d\n", commitID);
        if (removeCommitsGreaterThan(commitID) != 0) {
            fprintf(stderr, RED "Error removing commits with IDs greater than %d\n" RESET, commitID);
            return 1;
        }
        printf("Working directory updated based on commit %d\n", commitID);
        return 0;
    }

    DIR *dir;
    struct dirent *ent;
    dir = opendir(".neogit/branches");
    if (dir == NULL) {
        perror(RED "Error opening branches directory" RESET);
        return 1;
    }
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            if (strcmp(ent->d_name, branchname) == 0) {
                FILE *file = fopen(".neogit/currbranch", "w");
                if (file == NULL) {
                    perror(RED "Error opening .neogit/currbranch for writing" RESET);
                    return 1;
                }
                if (fprintf(file, "%s", branchname) < 0) {
                    perror(RED "Error writing to .neogit/currbranch" RESET);
                    fclose(file);
                    return 1;
                }
                fclose(file);
                printf(GREEN "Switched to branch '%s'\n" RESET, branchname);
                printf("Working directory updated based on " YELLOW "commit %d\n" RESET, findHighestFileNumberbranch(branchname));
                closedir(dir);
                return 0;
            }
        }
    }
    fprintf(stderr, RED "Error: Branch '%s' not found.\n" RESET, branchname);
    closedir(dir);
    return 1;
}
// alias

void add_alias(const char *alias, const char *command)
{
    FILE *file = fopen(ALIAS_FILE, "a");
    if (file == NULL)
    {
        perror(RED "Error opening alias file for writing" RESET);
        exit(EXIT_FAILURE);
    }
    if (strncmp(command, "./neogit", 8) != 0)
    {
        printf("You cannot add an invalid command as an alias!\n");
        return;
    }
    fprintf(file, "%s=%s\n", alias, command);
    printf(GREEN "Alias added successfully: " RESET "%s -> %s\n", alias, command);
    fclose(file);
}
void read_aliases()
{
    FILE *file = fopen(ALIAS_FILE, "r");
    if (file == NULL)
    {
        perror(RED "Error opening alias file for reading" RESET);
        exit(EXIT_FAILURE);
    }

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *stored_alias = strtok(line, "=");
        char *stored_command = strtok(NULL, "\n");

        if (stored_alias != NULL && stored_command != NULL)
        {
            printf("Alias: %s, Command: %s\n", stored_alias, stored_command);
        }
    }

    fclose(file);
}
void execute_alias(const char *alias)
{
    FILE *file = fopen(ALIAS_FILE, "r");
    if (file == NULL)
    {
        perror(RED "Error opening alias file for reading" RESET);
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *stored_alias = strtok(line, "=");
        char *stored_command = strtok(NULL, "\n");

        if (stored_alias != NULL && stored_command != NULL && strcmp(alias, stored_alias) == 0)
        {

            char trimmed_command[100];
            sscanf(stored_command, " %99[^\t\n]", trimmed_command);

            char full_command[200];
            snprintf(full_command, sizeof(full_command), "%s", trimmed_command);
            system(full_command);
            break;
        }
    }

    fclose(file);
}

// shortcuts

void add_shortcut(const char *shortcut_name, const char *shortcut_message)
{
    FILE *file = fopen(".neogit/shortcuts", "a");
    if (file == NULL)
    {
        perror(RED "Error opening shortcuts file for writing" RESET);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s=%s\n", shortcut_name, shortcut_message);
    fclose(file);
}
const char *get_shortcut_message(const char *shortcut_name)
{
    FILE *file = fopen(".neogit/shortcuts", "r");
    if (file == NULL)
    {
        perror(RED "Error opening shortcuts file for reading" RESET);
        exit(EXIT_FAILURE);
    }

    char line[MAX_MESSAGE_LENGTH + MAX_MESSAGE_LENGTH + 2];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *stored_shortcut = strtok(line, "=");
        char *stored_message = strtok(NULL, "\n");

        if (stored_shortcut != NULL && stored_message != NULL && strcmp(shortcut_name, stored_shortcut) == 0)
        {
            return stored_message;
        }
    }

    fclose(file);
    return NULL; // Shortcut not found
}
int replace_shortcut(const char *shortcut_name, const char *new_message)
{
    const char *shortcuts_file_path = ".neogit/shortcuts";
    FILE *file = fopen(shortcuts_file_path, "r");
    if (file == NULL)
    {
        perror(RED "Error opening shortcuts file for reading" RESET);
        exit(EXIT_FAILURE);
    }
    FILE *temp_file = fopen(".neogit/temp_shortcuts", "w");
    if (temp_file == NULL)
    {
        perror(RED "Error opening temp file for writing" RESET);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[MAX_MESSAGE_LENGTH + 22];
    int found = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *stored_shortcut = strtok(line, "=");
        char *stored_message = strtok(NULL, "\n");

        if (stored_shortcut != NULL && stored_message != NULL && strcmp(shortcut_name, stored_shortcut) == 0)
        {
            fprintf(temp_file, "%s=%s\n", shortcut_name, new_message);
            found = 1;
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }
    fclose(file);
    fclose(temp_file);
    if (!found)
    {
        fprintf(stdout, "Shortcut not found: %s\n", shortcut_name);
        remove(".neogit/temp_shortcuts");
        return 0;
    }

    remove(shortcuts_file_path);
    rename(".neogit/temp_shortcuts", shortcuts_file_path);
    printf(GREEN "Shortcut replaced successfully: %s -> %s\n" RESET, shortcut_name, new_message);
    return 0;
}
int remove_shortcut(const char *shortcut_name)
{
    const char *shortcuts_file_path = ".neogit/shortcuts";

    FILE *file = fopen(shortcuts_file_path, "r");
    if (file == NULL)
    {
        perror(RED "Error opening shortcuts file for reading" RESET);
        return 1;
    }

    FILE *temp_file = fopen(".neogit/temp_shortcuts", "w");
    if (temp_file == NULL)
    {
        perror(RED "Error opening temp file for writing" RESET);
        fclose(file);
        return 1;
    }
    char line[MAX_MESSAGE_LENGTH + 52];
    int found = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *stored_shortcut = strtok(line, "=");
        char *stored_message = strtok(NULL, "\n");

        if (stored_shortcut != NULL && stored_message != NULL && strcmp(shortcut_name, stored_shortcut) == 0)
        {
            found = 1;
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }

    fclose(file);
    fclose(temp_file);
    if (!found)
    {
        fprintf(stdout, RED "Shortcut not found: %s\n" RESET, shortcut_name);
        remove(".neogit/temp_shortcuts");
        return 1;
    }
    remove(shortcuts_file_path);
    rename(".neogit/temp_shortcuts", shortcuts_file_path);
    printf(GREEN "Shortcut removed successfully: %s\n" RESET, shortcut_name);
    return 0;
}

// diff

int compareLines(const char *line1, const char *line2)
{
    while (*line1 != '\0' && *line2 != '\0')
    {
        if (*line1 == ' ' || *line1 == '\t' || *line1 == '\n')
        {
            line1++;
            continue;
        }
        if (*line2 == ' ' || *line2 == '\t' || *line2 == '\n')
        {
            line2++;
            continue;
        }

        if (*line1 != *line2)
        {
            return 0;
        }

        line1++;
        line2++;
    }

    return (*line1 == '\0' && *line2 == '\0');
}
int neogit_diff(const char *file1, const char *file2, int line1_start, int line1_end, int line2_start, int line2_end)
{
    FILE *fp1, *fp2;
    char buffer1[512], buffer2[512];

    if ((fp1 = fopen(file1, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file1);
        return 1;
    }

    if ((fp2 = fopen(file2, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file2);
        fclose(fp1);
        return 1;
    }

    for (int i = 1; i < line1_start; ++i)
        if (fgets(buffer1, sizeof(buffer1), fp1) == NULL)
            break;

    for (int i = 1; i < line2_start; ++i)
        if (fgets(buffer2, sizeof(buffer2), fp2) == NULL)
            break;

    int lineNumber1 = line1_start;
    int lineNumber2 = line2_start;

    int foundDifference = 0;

    while (!feof(fp1) || !feof(fp2))
    {

        if (feof(fp1) && feof(fp2))
        {
            break;
        }
        if (fgets(buffer1, sizeof(buffer1), fp1) != NULL)
        {
            lineNumber1++;
        }

        if (fgets(buffer2, sizeof(buffer2), fp2) != NULL)
        {
            lineNumber2++;
        }
        if ((line1_end > 0 && lineNumber1 > line1_end) || (line2_end > 0 && lineNumber2 > line2_end))
        {
            break;
        }
        if (strcmp(buffer1, buffer2) != 0)
        {
            if (!foundDifference)
            {
                printf("«««««\n");
                foundDifference = 1;
            }
            printf("«««««\n");
            printf("%s - %d \n", file1, lineNumber1);
            printf(RED "%s" RESET, buffer1);
            printf("%s - %d \n", file2, lineNumber2);
            printf(YELLOW "%s" RESET, buffer2);
            printf("»»»»»\n");
            printf("\n");
        }
    }
    fclose(fp1);
    fclose(fp2);
    return 0;
}

// tag

int create_tag(const char *tag_name, const char *message, int cgiven, int commitid, int force)
{
    char tag_path[MAX_FILENAME_LENGTH];
    char *author = extractUsername();
    int commit_id;
    if (cgiven)
    {
        commit_id = commitid;
    }
    else
    {
        commit_id = findHighestFileNumber();
    }

    snprintf(tag_path, sizeof(tag_path), ".neogit/tags/%s", tag_name);

    if (!force && access(tag_path, F_OK) == 0)
    {
        fprintf(stderr, RED "Error: Tag '%s' already exists. Use -f to force.\n" RESET, tag_name);
        return 1;
    }
    FILE *tag_file;
    if ((tag_file = fopen(tag_path, "w")) == NULL)
    {
        fprintf(stderr, RED "Error creating tag file: %s\n" RESET, tag_path);
        return 1;
    }
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(tag_file, "Tag Name: %s\n", tag_name);
    fprintf(tag_file, "Commit ID: %d\n", commit_id);
    fprintf(tag_file, "Tagger: %s\n", author);
    fprintf(tag_file, "Date and Time: %s\n", time_str);

    if (message != NULL)
    {
        fprintf(tag_file, "Message: %s\n", message);
    }

    fclose(tag_file);

    printf(GREEN "Tag '%s' created successfully.\n" RESET, tag_name);

    return 0;
}
int compareTags(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}
int showAllTags()
{
    DIR *dir = opendir(".neogit/tags");
    if (dir == NULL)
    {
        perror(RED "Error opening tags directory" RESET);
        return 1;
    }

    struct dirent *entry;
    char **tags = NULL;
    int tagCount = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char *tagName = malloc(strlen(entry->d_name) + 1);
            strcpy(tagName, entry->d_name);
            tags = realloc(tags, (tagCount + 1) * sizeof(char *));
            tags[tagCount] = tagName;
            tagCount++;
        }
    }
    closedir(dir);
    qsort(tags, tagCount, sizeof(char *), compareTags);

    for (int i = 0; i < tagCount; i++)
    {
        printf("%s\n", tags[i]);
        free(tags[i]);
    }
    free(tags);
    return 0;
}
int showTagContent(const char *tagName)
{

    char filePath[100];
    snprintf(filePath, sizeof(filePath), ".neogit/tags/%s", tagName);

    FILE *tagFile = fopen(filePath, "r");
    if (tagFile == NULL)
    {
        perror(RED "Error opening tag file" RESET);
        return 1;
    }
    char line[256];
    while (fgets(line, sizeof(line), tagFile) != NULL)
    {
        printf("%s", line);
    }

    fclose(tagFile);
    return 0;
}

// grep

int neogit_grep(const char *file, const char *word, const char *commitId, int printLineNumber)
{
    FILE *fp;
    char line[512];
    int lineNumber = 0;
    int found = 0;

    if ((fp = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, RED "Error opening file: %s\n" RESET, file);
        return 1;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        lineNumber++;

        char *position = strstr(line, word);

        if (position != NULL)
        {
            found = 1;

            if (printLineNumber)
            {
                printf(YELLOW "%d: " RESET, lineNumber);
            }

            fwrite(line, 1, position - line, stdout);

            printf(RED "%.*s" RESET, (int)strlen(word), position);

            printf("%s", position + strlen(word));
        }
    }

    fclose(fp);

    if (!found)
    {
        printf("No occurrences of '%s' found in '%s'.\n", word, file);
    }

    return 0;
}
