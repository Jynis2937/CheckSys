#include "sysinfo.h"

double get_hdd_usage(const char* mnted_dir)
{
	static struct statvfs vfs_filesys_info;
	if((statvfs(mnted_dir, &vfs_filesys_info)) < 0 ) 
	{
        	return -1;
        } 
	else 
	{
		if(0 == vfs_filesys_info.f_blocks)
		{
			return -1;		
		}
		return 100 * ((double)(vfs_filesys_info.f_blocks - vfs_filesys_info.f_bfree) / vfs_filesys_info.f_blocks);
        }
}
void get_sysinfo(void)
{
	FILE* temp_file_stream;	
	char buf[32];

	system("cat /proc/stat | grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage \"%\"}' > rsrcinfo");
	system("free | awk 'FNR == 3 {print $3/($3+$4)*100}' >> rsrcinfo");
	temp_file_stream = fopen("rsrcinfo", "r+");
	fgets(buf, sizeof(buf), temp_file_stream);
	myinfo.cpu_usage = atof(buf);
	fgets(buf, sizeof(buf), temp_file_stream);
	myinfo.ram_usage = atof(buf);
	myinfo.hdd_usage = get_hdd_usage("/");

	fclose(temp_file_stream);
	remove("rsrcinfo");
}

int exit_code = 0;
int global_sock = 0;
int connection_ok = 0;
void* recv_alarmn(void* param)
{
	FILE* pipe;
	static char info[1024];
	int readn;

	do
	{
		printf("waiting...\n");
		if((readn = recv(global_sock, info, sizeof(info), MSG_NOSIGNAL)) > 0)
		{
			printf("Received alarm : %s\n", info);
			if(!strcmp(info, "Alarm1"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}		
			else if(!strcmp(info, "Alarm2"))
			{
				pipe = popen("mplayer ./Alarm2.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm3"))
			{
				pipe = popen("mplayer ./Alarm3.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm4"))
			{
				pipe = popen("mplayer ./Alarm4.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm5"))
			{
				pipe = popen("mplayer ./Alarm5.wav > /dev/null", "r");
			}
		} 
		memset(info, 0, sizeof(info));
		sleep(1);	
	}while(0 == exit_code);
}
#define UPDATE_TIME 5
void* terminate_process_pending(void* param)
{
	char t;
	int sock = (*(int*)param);

	printf("To terminate this process, enter key 'q' once.\n");
reget:
	t = getchar();
	if(t == 'q')
	{
		exit_code = 1;
	}
	else
	{
		goto reget;
	}
}


int main(int argc, char* argv[])
{
	char info[1024];
	char df_buf[32], sys_buf[64];
	int sock, n;
	char* haddr;
	struct sockaddr_in server_addr;
	FILE* df_target;
	FILE* filesys;
	pthread_t thr[2];
	int thr_id[2];
	int is_thread_start = 0;
	int is_thread_start_terminator = 0;

	if(2 != argc)
	{
		printf("usage : ./%s <IP>\n", argv[0]);
		exit(1);	
	}

reconnection:
	connection_ok = 0;

	system("df --output=target > dfop");
	system("df > df_sys");

	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Socket error.\n");
		exit(0);
	}
	global_sock = sock;
	bzero((char*)&server_addr, sizeof(server_addr));

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(argv[1]);
        server_addr.sin_port = htons(2937); 
	
	while(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("Can't connect to server.\n");
		if(exit_code == 1)
		{
			goto clean;
		}
		sleep(UPDATE_TIME);
		connection_ok = 1;
	}

	if(0 == is_thread_start)
	{
		thr_id[0] = pthread_create(&thr[0], NULL, &recv_alarmn, (void*)&sock);
		is_thread_start = 1;	
	}
	if(0 == is_thread_start_terminator)
	{
		thr_id[1] = pthread_create(&thr[1], NULL, &terminate_process_pending, (void*)&sock);
		is_thread_start_terminator = 1;
	}

       	do
	{
		get_sysinfo();
		sprintf(info, "%s\r\n%d\r\n%d\r\n%d\r\n%s\r\n", getenv("USER"), (int)ceilf(myinfo.cpu_usage), (int)ceilf(myinfo.ram_usage), (int)ceilf(myinfo.hdd_usage), "TRUE");
		
		df_target = fopen("dfop", "r+");
		filesys = fopen("df_sys", "r+");
		fgets(df_buf, sizeof(df_buf), df_target);
		fgets(sys_buf, sizeof(sys_buf), filesys);
		while(!feof(df_target))
		{
			fgets(df_buf, sizeof(df_buf), df_target);
			fgets(sys_buf, sizeof(sys_buf), filesys);			
			if(df_buf[strlen(df_buf) - 1] == '\n')
			{
				df_buf[strlen(df_buf) - 1] = NULL;
			}
			if(NULL != strstr(sys_buf, " "))
			{
				sys_buf[strstr(sys_buf, " ") - sys_buf] = 0;
			}

			// printf("%s\n", sys_buf);
			sprintf(df_buf, "%s:%d\r\n", sys_buf, (int)ceilf(get_hdd_usage(df_buf)));
			strcat(info, df_buf);		
		}
		fclose(df_target);
		fclose(filesys);

		printf("%s\n", info);
		if(send(sock, info, strlen(info), MSG_NOSIGNAL) == -1)
		{			
			printf("Can't connect to server.\n");
			if(1 == exit_code)
			{
				goto clean;			
			}
			sleep(UPDATE_TIME);
			goto reconnection;
		}
		sleep(UPDATE_TIME);
	}while(0 == exit_code);

	get_sysinfo();
	sprintf(info, "%s\r\n%d\r\n%d\r\n%d\r\n%s\r\n", getenv("USER"), (int)ceilf(myinfo.cpu_usage), (int)ceilf(myinfo.ram_usage), (int)ceilf(myinfo.hdd_usage), "FALSE");
		
	df_target = fopen("dfop", "r+");
	filesys = fopen("df_sys", "r+");
	fgets(df_buf, sizeof(df_buf), df_target);
	fgets(sys_buf, sizeof(sys_buf), filesys);
	while(!feof(df_target))
	{
		fgets(df_buf, sizeof(df_buf), df_target);
		fgets(sys_buf, sizeof(sys_buf), filesys);			
		if(df_buf[strlen(df_buf) - 1] == '\n')
		{
			df_buf[strlen(df_buf) - 1] = NULL;
		}
		if(NULL != strstr(sys_buf, " "))
		{
			sys_buf[strstr(sys_buf, " ") - sys_buf] = 0;
		}
			// printf("%s\n", sys_buf);
		sprintf(df_buf, "%s:%d\r\n", sys_buf, (int)ceilf(get_hdd_usage(df_buf)));
		strcat(info, df_buf);		
	}
	fclose(df_target);
	fclose(filesys);

	printf("%s\n", info);
	send(sock, info, strlen(info), 0);

clean:
	close(sock);
	return 0;
}

