#ifndef CLEANUP_H

#define CLEANUP_H

int file_age_check(const char*, int);
int is_file_in_use(const char*);
int is_file_locked(const char*);
int is_valid_owner_group(const char*);
int is_directory_empty(const char*);
int should_exclude_directory(const char*);
int is_valid_date(int, int, int);

#endif