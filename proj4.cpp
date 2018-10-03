#include <iostream>
#include <fctnl.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <unistd.h>

using namespace std;

//global vars
char *cli_arg1;
char *cli_arg2;
char *read_content[100];

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
	}
	else{
		//if we got the number of arguments we wanted
		const void *read_buffer = argv;
		cli_arg1 = argv[1];
		cli_arg2 = argv[2];

	}

	//open the file
	if ( (fd = open(cli_arg1, O_RDONLY)) < 0){
		//open as read only since we should not be modifying files
		cerr << "open error";
	}

	//read the files
	if ( read(fd, read_content, 100) < 0){
		cerr << "read error: "
	}
	else{
		//we are reading the files now?
	}







} //end of main