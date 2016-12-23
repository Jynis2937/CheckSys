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
}
void* recv_alarmn(void* param)
{
	FILE* pipe;
	static char info[1024];
	int s = (*(int*)param);
	
	while(1)
	{
		if(recv(s, info, sizeof(info), 0) > 0)
		{
			if(!strcmp(info, "Alarm1"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}		
			else if(!strcmp(info, "Alarm2"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm3"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm4"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}
			else if(!strcmp(info, "Alarm5"))
			{
				pipe = popen("mplayer ./Alarm1.wav > /dev/null", "r");
			}
			pclose(pipe);
		} 		
		
		sleep(1);
	}
}


#define UPDATE_TIME 5

int main(int argc, char* argv[])
{
	char info[1024];
	char df_buf[32], sys_buf[64];
	int sock, n;
	char* haddr;
	struct sockaddr_in server_addr;
	FILE* df_target;
	FILE* filesys;
	pthread_t thr;
	int thr_id;

	if(2 != argc)
	{
		printf("usage : ./%s <IP>\n", argv[0]);
		exit(1);	
	}

	system("df --output=target > dfop");
	system("df > df_sys");

	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Socket error.\n");
		exit(0);
	}
	bzero((char*)&server_addr, sizeof(server_addr));

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(argv[1]);
        server_addr.sin_port = htons(2937);
 
	while(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("Can't connect to server.\n");
		sleep(UPDATE_TIME);
	}
	
	thr_id = pthread_create(&thr, NULL, &recv_alarmn, (void*)&sock);
       	do
	{
		get_sysinfo();
		sprintf(info, "%s\r\n%d\r\n%d\r\n%d\r\n", getenv("USER"), (int)ceilf(myinfo.cpu_usage), (int)ceilf(myinfo.ram_usage), (int)ceilf(myinfo.hdd_usage));
		
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
		if(send(sock, info, strlen(info), 0) < 0)
		{
			printf("Can't connect to server.\n");
		}
		sleep(UPDATE_TIME);
	}while(1);
}

