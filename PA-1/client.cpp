/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Rishab Jadhav
	UIN: 533009378
	Date: September 24th, 2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	// Create child process, duplicates current process
	pid_t pid = fork();

	if (pid < 0) {
		// ERROR STATE
		cout << "ERROR" << endl;
	} else if (pid == 0) {
		// pid == 0, running in CHILD process, to run server

		// Run server
		execlp("./server", "server", NULL);
	}

	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':	// patient argument
				p = atoi (optarg);
				break;
			case 't':	// time argument
				t = atof (optarg);
				break;
			case 'e':	// ecg number argument
				e = atoi (optarg);
				break;
			case 'f':	// file name argument
				filename = optarg;
				break;
		}
	}

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
    char buf[MAX_MESSAGE]; // 256
    datamsg x(p, t, e);
	
	memcpy(buf, &x, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg)); // question
	double reply;
	chan.cread(&reply, sizeof(double)); //answer
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
    // sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = "teslkansdlkjflasjdf.dat";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;

	delete[] buf2;
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	wait(NULL);
}
