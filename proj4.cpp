#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

using namespace std;

//definitions
#define DEFAULTBYTESTOREAD 1024
#define MAXCHUNK 8192
#define MAXTHREADS 16
#define RANGE 1
#define ALLDONE 2

//structs
typedef struct{
	int iSender; //sender of message
	int type; //message type
	long value1;
	long value2;
} msg;

//global vars
char *filename;
long filesize;
char *search_string;
int bytes_to_read = DEFAULTBYTESTOREAD;
int num_threads = MAXTHREADS;

pthread_t tids[MAXTHREADS+1];
sem_t rcvsem[MAXTHREADS+1];
sem_t sendsem[MAXTHREADS+1];

msg mailboxes[MAXTHREADS+1];
int mailbox_ids[MAXTHREADS+1] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
int count = 0;
char *mmapped_data = 0;

int fd;

//functions
int RecvMsg(int iFrom, msg *pMsg){
	sem_wait(&rcvsem[iFrom]);
	*pMsg = mailboxes[iFrom];
	sem_post(&sendsem[iFrom]);
	return 0;
}

int SendMsg(int iTo, msg *pMsg){
	sem_wait(&sendsem[iTo]);
	mailboxes[iTo] = *pMsg;
	sem_post(&rcvsem[iTo]);
	return 0;
}

int ReadBuff(int start_addr, int end_addr){
	int count = 0;
	int ptr = 0;

	for (int i = start_addr; i < end_addr; i++){
		if (mmapped_data[i] == search_string[ptr]){
			//if char matches, increment pointer for string search
			ptr++;
			if (ptr == strlen(search_string)){
				count++;
				ptr = 0;
			}
			else if ( (i == end_addr - 1) && ptr < strlen(search_string)){
				end_addr++;
				if (end_addr >= filesize){
					//we reached the end of the area to read so break here
					//because there is no point in continuing
					break;
				}
			}
		}
		else{
			//if char mismatch
			ptr = 0;
		}

	}
	return count;
}

//thread routines
void *wthreadRoutine(void* arg){
	//cout << "bloop worker thread." << endl;
	int m_id = *(int*)arg;
	msg message;
	int wcount = 0;

	RecvMsg(m_id, &message);
	//message.value1 will be holding start addr
	//message.value2 will be holding end addr
	wcount = ReadBuff(message.value1, message.value2);

	message.iSender = m_id;
	message.type = ALLDONE;
	message.value1 = 0;
	message.value2 = wcount;
	SendMsg(0, &message);

	return 0;
}

void *pthreadRoutine(void *arg){
	//cout << "bloop parent thread." << endl;
	int start = 0;
	int end = filesize/num_threads;
	int mod = 0;
	
	for (int i = 1; i <= num_threads; i++){
		if (i > 1){
			start = end;
			end = start + (filesize/num_threads);
			mod = filesize % num_threads;
		}

		if (i <= mod){
			end++;
		}

		msg message;
		message.iSender = 0;
		message.type = RANGE;
		message.value1 = (long) start; //start address to search at
		message.value2 = (long) end; //end address to stop searching at
		SendMsg(i, &message);
	}

	//wait for messages from all workers
	for (int i = 1; i <= num_threads; i++){
		msg message;
		RecvMsg(0, &message);
		if (message.type == ALLDONE){
			count += message.value2; //value 2 will be holding the count found
		}
	}

	return 0;
}

int main(int argc, char* argv[]){
	
	//collect input args *******************************************************
	int do_mmap = 0;
	int do_multi_threaded = 0;
	struct stat st;

	//check the input
	if (argc < 3){
		//if the minimum number of arguments was not met
		cout << "Error: Please provide at least 2 arguments." << endl;
		return 1;
	}
	else{
		//if we got the number of arguments we wanted
		filename = argv[1];
		search_string = argv[2];

		stat(filename, &st);
		filesize = st.st_size;
	}

	if (argc > 3){
		//third argument given
		if ( strcmp("mmap", argv[3]) == 0){
			//mmap entered, acknowledge this
			do_mmap = 1;
		}
		else{
			//mmap not entered, assume it is a number
			int entry = atoi(argv[3]);
			if (entry >= MAXCHUNK){
				bytes_to_read = MAXCHUNK;
			}
			else if ( (entry > 0) && (entry < MAXCHUNK)){
				bytes_to_read = entry;
			}
		}
	}

	if (argc > 4){
		//fourth argument given for number of threads to use
		do_multi_threaded = 1;
		int entry = atoi(argv[4]);
		if ( (entry > 0) && (entry <= MAXTHREADS)){
			num_threads = entry;

			if (num_threads > filesize){
				num_threads = filesize;
			}
		}
		else if ( (entry < 0) || (entry > MAXTHREADS) ){
			cout << "Error: Please enter at least 1 thread or at most 16 threads." << endl;
			exit(1);
		}
	}
	//end of input arg checking ************************************************

	//print file size
	cout << "File size: " << filesize << " bytes." << endl;

	char read_buffer[bytes_to_read];
	//open the file
	errno = 0;
	if ( (fd = open(filename, O_RDONLY)) < 0){
		//open as read only since we should not be modifying files
		perror("Could not open file.");
		exit(1);
	}

	if (do_mmap && !do_multi_threaded){
		errno = 0;
		mmapped_data = (char*)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		if (mmapped_data < 0){
			perror("Could not mmap file.");
			exit(1);
		}
		
		//read through the returned area for the string
		int ptr = 0;
		for (int i = 0; i < filesize; i++){
			if (mmapped_data[i] == search_string[ptr]){
				//if char matches, increment pointer for string search
				ptr++;
				if (ptr == strlen(search_string)){
					count++;
					ptr = 0;
				}
			}
			else{
				//if char mismatch
				ptr = 0;
			}
		}
		
		if (munmap(mmapped_data, filesize) < 0){
			perror("Could not unmap memory.");
			exit(1);
		}
	}
	else if (do_multi_threaded){
		errno = 0;
		mmapped_data = (char*)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		if (mmapped_data < 0){
			perror("Could not mmap file.");
			exit(1);
		}

		//create sems
		for (int i = 0; i <= num_threads; i++){
			if ( sem_init(&sendsem[i], 0, 1) == 0){
				//created semaphore successfully...nothing more to do?
			}
			else{
				perror("Could not create semaphore.");
				exit(1);
			}
		}

		for (int i = 0; i <= num_threads; i++){
			if ( sem_init(&rcvsem[i], 0, 0) == 0){
				//created semaphore successfully...nothing more to do?
			}
			else{
				perror("Could not create semaphore.");
				exit(1);
			}
		}

		
		//create parent thread
		if (pthread_create(&tids[0], NULL, pthreadRoutine, &mailbox_ids[0]) == 0){
			//thread created successfully
		}
		else{
			perror("Could not create pthread.");
			exit(1);
		}

		//create worker threads
		for (int i = 1; i <= num_threads; i++){
			if ( pthread_create(&tids[i], NULL, wthreadRoutine, &mailbox_ids[i]) == 0){
				//thread created successfully...do nothing for now?
			}
			else{
				perror("Could not create pthread.");
				exit(1);
			}
		}

		//join threads
		for (int i = 0; i <= num_threads; i++){
			int retval;
			void* res;
			retval = pthread_join(tids[i], &res);
			if (retval != 0){
				perror("could not join pthread.");
				exit(1);
			}
		}

		//destroy semaphores
		for (int i = 0; i <= num_threads; i++){
			sem_destroy(&sendsem[i]);
			sem_destroy(&rcvsem[i]);
		}

	} //end of else if(do_multi_threaded)

	else{
		//do read
		//read the file
		int numof_bytes_read;
		int ptr = 0;
		while ( (numof_bytes_read = read(fd, read_buffer, bytes_to_read)) > 0){
			//read returns the number of bytes read and increments some counter
			//that marks where it currently is. So loop the read function and check
			//what it stores in read_buffer for words to count. Incremement a count
			//for the number of times a search string is found

			for (int i = 0; i < numof_bytes_read; i++){
				if (read_buffer[i] == search_string[ptr]){
					//if char matches, increment pointer for string search
					ptr++;
					if (ptr == strlen(search_string)){
						count++;
						ptr = 0;
					}
				}
				else{
					//if char mismatch
					ptr = 0;
				}

			}
			//return count;

		} //end of while loop
	}

	close(fd);
	cout << "num of substr found " << count << endl;;
	
	return 0;
} //end of main