#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>



#define OPT_NEED 1
#define OPT_UNKNW 2
#define FILE_NOT_OPEN 3
#define HASH_ERROR 4
#define PTHREAD_ERROR 5
#define MEM_ERROR 6
#define FSTAT 7
#define N 5
#define MY_TYPE unsigned short int

pthread_mutex_t mutex_hash;

unsigned long long int hash_result;
int limit_data;
MY_TYPE *data_file;
double temp_result=0;

unsigned long long int fact(int x)
{
	return  x==0?1:x*fact(x-1);
}

double my_pow(MY_TYPE x, int i)
{
	return i==0?1:x*my_pow(x,i-1);
}

unsigned long long int my_func(MY_TYPE x)
{
	int i;
	double sum = 1;
	unsigned long long int result;
	if (x == 0 || x == 1) return 1;
	for (i = 1; i < N; i++)
	{
		sum += fact(i)/my_pow(x,i);
	}
	
	sum*=100000;
	result = sum;
	temp_result+=sum;
	printf("%d %lld\n",x, result);
	return result;
}

void *pthread_exp(void *arg)
{
	int start = *(int *) arg;
	int num_data = start;
	unsigned long long int func;

	while(num_data <= start + limit_data -1) {
		func = my_func(*(data_file+num_data));
		
		pthread_mutex_lock(&mutex_hash);
		hash_result ^= func;
		pthread_mutex_unlock(&mutex_hash);
		num_data++;
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	int opt;
	char *file_name;
	int handle_file; 
	int cnt_cores, cnt_mem_free;
	int i;

	while((opt=getopt(argc, argv, ":f:")) != -1) {
		switch (opt) {
		case 'f':
			file_name = optarg;
			break;
		case ':':
			return error_exit(OPT_NEED);
		case '?':
			return error_exit(OPT_UNKNW);
		}
	}

	cnt_cores = 1;//sysconf(_SC_NPROCESSORS_ONLN);
	printf("Cores:  \t%d\n", cnt_cores);

	cnt_mem_free = (0.3 * sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGE_SIZE) /
					sizeof(MY_TYPE)) * sizeof(MY_TYPE);
	printf("Mem:\t%d Mb\n", cnt_mem_free);

	handle_file = open(file_name,O_RDONLY);
	if(!handle_file) {
		return error_exit(FILE_NOT_OPEN);
	}

	if (hash(handle_file, cnt_cores, cnt_mem_free) == -1)
		return error_exit(HASH_ERROR);
	close(handle_file);
	free(data_file);
	printf("hash = %#10llx\ntemp=%.15f\n", hash_result, temp_result);
	return 1;
}


int hash(int file, int cores, int mem)
{
	int cnt_read,i,size;
	int start_points[cores];
	struct stat stat_buf;
	pthread_t id[cores];

	pthread_mutex_init(&mutex_hash, NULL);
	hash_result = 0;
	
	if(fstat(file, &stat_buf) < 0)
		return error_exit(FSTAT);
	
	if ((stat_buf.st_size) < mem) 	
		size =	stat_buf.st_size/sizeof(MY_TYPE);
	else 
		size =	mem/sizeof(MY_TYPE);

	data_file =  calloc(size,sizeof(MY_TYPE));
	if (!data_file)
		return error_exit(MEM_ERROR);
		
	while(1) {
		memset((MY_TYPE *)data_file, '\0', size);
		cnt_read = read(file, data_file, mem);
		if(!cnt_read)
			break;
		limit_data = cnt_read / cores;
		
		for(i = 0; i < cores; i++) {
			start_points[i] = i * limit_data;
			printf("limit=%d start=%d\n",limit_data, start_points[i]);
			if(pthread_create(	&id[i],	NULL, pthread_exp, &start_points[i])) {
				return error_exit(PTHREAD_ERROR);
			}
		}

		for(i=0; i<cores; i++) 
			pthread_join(id[i], NULL);
	}
	return 1;
}




int error_exit(int id_error)
{
	char text_error[80];
	int i;
	for(i=0; i<80; i++)
		text_error[i]='\0';
	switch (id_error) {
		case OPT_NEED:
			strcpy(text_error, "Error! Need a value of option!\n");
			break;
		case OPT_UNKNW:
			strcpy(text_error, "Error! Unknown an option!\n");
			break;
		case FILE_NOT_OPEN:
			strcpy(text_error, "Error! File doesn`t opened!\n");
			break;
		case HASH_ERROR:
			strcpy(text_error, "Error! Hash doesn`t calculate!\n");
			break;
		case PTHREAD_ERROR:
			strcpy(text_error, "Error! Pthread doesn`t calculate!\n");
			break;
		case MEM_ERROR:
			strcpy(text_error, "Error! Something with memory... hm...\n");
			break;
		case FSTAT:
			strcpy(text_error, "Error! fstat() returned negative value!\n");
			break;
	}
	write(0, text_error, 80);
	return -1;
}
