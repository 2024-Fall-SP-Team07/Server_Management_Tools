#ifndef CLEANUP_H

#define CLEANUP_H

void log_deletion_record(const char*);
int cleanup_files_recursive(const char*, int, int);
int cleanup_log_files(const char*, int, int);

#endif