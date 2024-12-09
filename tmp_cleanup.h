#ifndef TMP_CLEANUP_H

#define TMP_CLEANUP_H

int is_valid_date(int, int, int);
int log_deletion_record(const char*);
int file_age_check(const char*, int);
int is_file_in_use(const char*);
int is_file_locked(const char*);
int is_valid_owner_group(const char*);
int is_directory_empty(const char*);
int should_exclude_directory(const char*);
int cleanup_files_recursive(const char*, int, int);
int cleanup_log_files(const char*, int, int);
int ask_delete_confirmation(int*);
int main();

#endif