#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
	int value1;
	int value2;
}

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
int num_threads = 0;
int count = 0;
void *mmapped_data = 0;

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

//thread routines
void *wthreadRoutine(void* arg){
	int m_id = *(int*)arg;
	msg message;

	RecvMsg(m_id, &message);
	//message.value1 will be holding start addr
	//message.value2 will be holding end addr

	//read through buffer

	//count substrings

	message.iSender = m_id;
	message.type = ALLDONE;
	message.value1 = 0;
	message.value2 = count;
	SendMsg(0, &message);

	return 0;
}

void *pthreadroutine(void *arg){
	int start = mmapped_data, end = 0, mod = 0;
	for (int i = 1; i <= num_threads; i++){

		if (i > 1){
			start = end + 1;
			end = (start - 1) + (filesize/num_threads);
			mod = filesize % num_threads;
		}

		if (i <= mod){
			end++;
		}

		msg message;
		message.iSender = 0;
		message.type = RANGE;
		message.value1 = start; //start address to search at
		message.value2 = end; //end address to stop searching at
		SendMsg(i, &message);
	}

	//wait for messages from all workers
	for (int = 1; i <= num_threads; i++){
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
	int fd;
	int do_mmap = 0;
	int do_multi_threaded = 0;
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
	}

	/*if (argc > 3){
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
	}*/
	if (argc > 3){
		//adjust this to be an additional optional argument
		//later. Whenever we get here using pn we should always set and use mmap
		do_multi_threaded = 1;
		entry = atoi(argv[3]);
		if ( (entry >= 0) || (entry <= MAXTHREADS)){
			num_threads = entry;
		}
		else {
			num_threads = MAXTHREADS;
		}
	}

	//end of input arg checking ************************************************

	char read_buffer[bytes_to_read];
	//open the file
	errno = 0;
	if ( (fd = open(filename, O_RDONLY)) < 0){
		//open as read only since we should not be modifying files
		cout << "Error: open returned errno: " << errno << endl;
		return 1;
	}

	//find file size
	struct stat st;
	stat(filename, &st);
	filesize = st.st_size;
	cout << "File size: " << filesize << " bytes." << endl;


	//read the file
	int numof_bytes_read;

	if (do_mmap){
		errno = 0;
		mmapped_data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		if (mmapped_data < 0){
			cout << "mmap error: errno set to " << errno << endl;
			return 1;
		}
		//read through the returned area for the string
		char *ptr = strstr( (char*)mmapped_data, search_string);
		while ( ptr != NULL){
			count ++;
			ptr = strstr(ptr + 1, search_string);
		}
	}
	else if (do_multi_threaded){
		//based on the num threads to do work on we need to split work amongst
		//them. The work should be the process of reading through the file and
		//looking for the search string. We can use message passing like before
		//where we do work, return a message to our parent process, and in that
		//message return the count of words we found. The professor mentions
		//looking into accounting for the fact that search strings can be split
		//across files but for now let's not bother.

		//problem: evenly splitting a buffer across threads
		//What we can do, divide the number of bytes to read across each thread.
		//Now, similar to what we've done previously, give extra bytes to
		//threads when things are not evenly divisible.

		//create mailboxes
		//create semaphores
		//create threads

		//wait on message for threads
		//check message for the number of search strings found and add to count
		//close threads
		//destroy semaphores
		//prepare to exit

		errno = 0;
		void *mmapped_data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		if (mmapped_data < 0){
			cout << "mmap error: errno set to " << errno << endl;
			return 1;
		}

		//determine how to organize work amongst threads

	}
	else{
		while ( (numof_bytes_read = read(fd, read_buffer, bytes_to_read)) > 0){
			//read returns the number of bytes read and increments some counter
			//that marks where it currently is. So loop the read function and check
			//what it stores in read_buffer for words to count. Incremement a count
			//for the number of times a search string is found


			//go through read_buffer and count num of search_string gound
			char *ptr = strstr(read_buffer, search_string);
			while (ptr != NULL){
				count++;
				ptr = strstr(ptr + 1, search_string);
			}
		}
	}

	if (numof_bytes_read < 0){
		cerr << "read error:";
		return 1;
	}
	cout << "num of substr found " << count << endl;;
	
	return 0;
} //end of main