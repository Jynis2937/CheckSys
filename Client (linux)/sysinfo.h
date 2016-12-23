#ifndef _SYSINFO_H
#define _SYSINFO_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

typedef struct _sysinfo
{
	double cpu_usage;
	double ram_usage;
	double hdd_usage;
} sysinfo, *psysinfo;

static struct _sysinfo myinfo = {0, 0, 0};

double get_hdd_usage(const char* mnted_dir);
void get_sysinfo(void);

#endif /* _SYSINFO_H */
