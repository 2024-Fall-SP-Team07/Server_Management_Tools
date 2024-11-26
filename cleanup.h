#ifndef CLEANUP.H

#define CLEANUP.H

int cleanup_files_recursive(const char*, int, int);
int cleanup_log_files(const char*, int, int);
void log_deletion_record(const char*);

#endif