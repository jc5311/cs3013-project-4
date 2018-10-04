#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

using namespace std;

//definitions
#define BYTESTOREAD 1024

//global vars
char *filename;
char *search_string;
char read_buffer[BYTESTOREAD];

int main(int argc, char* argv[]){
	//collect file name and search string
	//open the file
	//read the file and find the number of instances of a string byte by byte of
	// the given search string
	int fd;

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

	//open the file
	errno = 0;
	if ( (fd = open(filename, O_RDONLY)) < 0){
		//open as read only since we should not be modifying files
		cout << "Error: open returned errno: " << errno << endl;
		return 1;
	}

	//find file size
	struct stat st;
	long filesize;
	stat(filename, &st);
	filesize = st.st_size;
	cout << "File size: " << filesize << " bytes." << endl;


	//read the file
	int numof_bytes_read;
	int count = 0;
	while ( (numof_bytes_read = read(fd, read_buffer, BYTESTOREAD)) > 0){
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

	cout << "num of substr found " << count << endl;;
	if (numof_bytes_read < 0){
		cerr << "read error:";
		return 1;
	}






	return 0;
} //end of main