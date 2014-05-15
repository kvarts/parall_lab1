#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#define MAX_NAME 256
#define TYPE_FOR_BLOCK long long
#define MAX_MEM 500000

#define pi 3.1415926535897932384
#define abs(x) x<0?-x:x
#define max_cnt10 1000000000000000000
#define OPT_ERROR 1
#define FILE_NOT_OPEN 3
#define HASH_ERROR 4
#define PTHREAD_ERROR 5
#define MEM_ERROR 6
#define FSTAT 7

pthread_mutex_t mutex_hash;
TYPE_FOR_BLOCK hash_result;
TYPE_FOR_BLOCK *data_file;
int limit_data;

long long int fact(int i)
{
	return  i==0?1:i*fact(i-1);
}

double my_pow(double x, int n)
{
	double result = 1;
	int i;
	for (i = 0; i < n; i++)
		result *= x;
	return result;
}

double format_pi(double x)
{
	double result = x;
	if (result > pi)
		result -= abs(result)/pi * pi;
		if (result < -pi)
			result += abs(result)/pi * pi;
		//printf("[debug]: abs(x) = %f\n", abs(result));
	
	return result;
}

double cos_teilor(double x)
{	
	x = format_pi(x);
	int i = 0;
	double result = 1;
	double temp1, temp4;
	TYPE_FOR_BLOCK temp2;
	short temp3 = -1;
	for(i = 1; i < 10; i++){
		temp1 = my_pow(x, 2*i);
		temp2 = fact(2*i);
		temp4 = (double) temp1/temp2;
		result += temp4*temp3;
		temp3 *= -1;
	}
	return result;
}

TYPE_FOR_BLOCK func_for_hash(double x)
{
	//printf("[debug]: Вход в функцию для хэша. x=%lld\n",(long long) x);
	double cos = cos_teilor(x);
	//printf("[debug]: cos(%f)=%f\n", x, cos);
	TYPE_FOR_BLOCK result = cos * max_cnt10;
}

void *pthread_func(void *arg)
{
	int start = *(int *) arg;
	int num_data = start;
	TYPE_FOR_BLOCK func;
	//printf("[debug]: Номер ячейки старта - %d\n", start);
	while(num_data <= start + limit_data -1) {
		func = func_for_hash(*(data_file+num_data));
		pthread_mutex_lock(&mutex_hash);
		//printf("[debug]: Хэш равен: %#llX\n", hash_result);
		hash_result ^= func;
		pthread_mutex_unlock(&mutex_hash);
		num_data++;
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	char name_file[MAX_NAME];
	int handle_file; 
	int cnt_cores;
	long cnt_mem;
	int i;

	if (argc != 2)
		error_exit(OPT_ERROR);
	
	strcpy(name_file, argv[1]);
	handle_file = open(name_file, O_RDONLY);
	if (!handle_file) {
		return error_exit(FILE_NOT_OPEN);
	}
	
	cnt_cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Количество ядер процессора:\t%d\n", cnt_cores);

	cnt_mem = (0.2 * sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGE_SIZE) /
					sizeof(TYPE_FOR_BLOCK)) * sizeof(TYPE_FOR_BLOCK);
	cnt_mem = cnt_mem < MAX_MEM ? cnt_mem : MAX_MEM;
	
	printf("Объем используемой памяти:\t%ld Mb\n", (long) cnt_mem/1024);

	if (hash(handle_file, cnt_cores, cnt_mem) == -1)
		return error_exit(HASH_ERROR);
	
	close(handle_file);
	free(data_file);
	printf("Хэш = %#llX\n",(long long) hash_result);
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

	size =	mem/sizeof(TYPE_FOR_BLOCK);

	data_file = calloc (size, sizeof(TYPE_FOR_BLOCK));
	if (!data_file)
		return error_exit(MEM_ERROR);
	//else
		//printf("[debug]: Выделена память\n");
	while(1) {
		memset((TYPE_FOR_BLOCK *)data_file, '\0', size);
		cnt_read = read(file, data_file, mem);
		if (!cnt_read)
			break;
		limit_data = cnt_read / cores;
		
		//printf("[debug]: Кол-во байт на поток - %d\n", limit_data);
		
		for (i = 0; i < cores; i++) {
			start_points[i] = i * limit_data;
			if(pthread_create(&id[i], NULL, pthread_func, &start_points[i])) {
				return error_exit(PTHREAD_ERROR);
			}
			//printf("[debug]: Создан поток с id = %d\n", (int) id[i]);
		}

		for (i=0; i<cores; i++) {
			//printf("[debug]: Ожидается поток с id = %d\n", (int) id[i]);
			pthread_join(id[i], NULL);
			//printf("[debug]: Завершился поток с id = %d\n", (int) id[i]);
		}
	}
	return 1;
}

int error_exit(int id_error)
{
	char text_error[80];
	int i;
	for (i=0; i<80; i++)
		text_error[i]='\0';
	switch (id_error) {
		case OPT_ERROR:
			strcpy(text_error, "Error! Need a value of option!\n");
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
	if (errno)
		printf("Комментарий: %s\n", strerror(errno));
	_exit(EXIT_FAILURE);
}
