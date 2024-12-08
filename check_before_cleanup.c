#include "h_for_tmp_cleanup.h"
#include "check_before_cleanup.h"

int is_valid_date(int year, int month, int day)
{
    if (month < 1 || month > 12)
    {
        return 0;
    }
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        days_in_month[1] = 29;
    }
    if (day < 1 || day > days_in_month[month - 1])
    {
        return 0;
    }
    return 1;
}

int file_age_check(const char *filename, int max_age_days)
{
    struct stat st;
    if (stat(filename, &st) == -1)
    {
        // perror("stat failed");
        return -1;
    }
    time_t current_time = time(NULL);
    double diff = difftime(current_time, st.st_mtime) / (60 * 60 * 24);
    return (diff >= max_age_days);
}

int is_file_in_use(const char *filename)
{
    char command[512];
    snprintf(command, sizeof(command), "fuser %s 2>/dev/null", filename);
    refresh();
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        // perror("fuser failed");
        return 0;
    }
    char buffer[128];
    int is_in_use = 0;
    if (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        is_in_use = 1;
    }
    pclose(fp);
    return is_in_use;
}

int is_file_locked(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        // perror("open failed");
        return 0;
    }
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    if (fcntl(fd, F_GETLK, &lock) == -1)
    {
        // perror("fcntl failed");
        close(fd);
        return 0;
    }
    close(fd);
    return (lock.l_type != F_UNLCK);
}

int is_valid_owner_group(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == -1)
    {
        // perror("stat failed");
        return 0;
    }
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    if (pw == NULL || gr == NULL)
    {
        return 0;
    }
    return 1;
}

int is_directory_empty(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if (!dir)
    {
        // perror("opendir failed");
        return 0;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return 1;
}

int should_exclude_directory(const char *filepath)
{
    const char *exclude_dirs[] = {
        "/var/cache/apt/archives",
        "/var/cache/dnf",
        "/var/cache/ldconfig",
        "/var/cache/pam",
        "/var/cache/private",
        "/var/cache/snapd",
        "/var/cache/PackageKit",
        "/var/cache/fontconfig"
    };
    for (unsigned int i = 0; i < sizeof(exclude_dirs) / sizeof(exclude_dirs[0]); i++)
    {
        if (strncmp(filepath, exclude_dirs[i], strlen(exclude_dirs[i])) == 0)
        {
            return 1;
        }
    }
    return 0;
}