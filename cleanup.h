#ifndef CLEANUP_H

#define CLEANUP_H

int cleanup_files_recursive(const char*, int, int, int);
int cleanup_log_files(const char*, int, int, int);
void log_deletion_record(const char*);

#endif