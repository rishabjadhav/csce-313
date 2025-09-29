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
#include <sys/stat.h> // added to pass gradescope
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {

	// default values for CMD line arguments
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m = MAX_MESSAGE;
	bool channel_arg = 0;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
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
			case 'm':	// message
				m = atoi(optarg);
				break;
			case 'c': 	// new channel argument
				channel_arg = true;
				break;
		}
	}

	string msg = to_string(m);

	char* serv_args[] = {
		(char*)"./server",
		(char*)"-m",
		(char*) msg.c_str(), NULL
	};

	// Create child process, duplicates current process
	pid_t pid = fork();

	if (pid < 0) {
		// ERROR STATE
		cout << "ERROR" << endl;
	} else if (pid == 0) {
		// pid == 0, running in CHILD process, to run server

		// Run server
		execvp("./server", serv_args);
		EXITONERROR("Exec failed");
	}

	// ensure the serv has enough time to get made first
	sleep(2);

    FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	
	// new requested channel is initially duplicated from chan
	FIFORequestChannel* r_chan = chan;
	
	if (channel_arg) {
		MESSAGE_TYPE newChanCreated = NEWCHANNEL_MSG;
		chan->cwrite(&newChanCreated, sizeof(MESSAGE_TYPE));

		// holds new channel name
		char newChanName[100];
		// int of end of channel
		int eoc = chan->cread(newChanName, sizeof(newChanName));
		newChanName[eoc] = '\0'; 

		r_chan = new FIFORequestChannel(newChanName, FIFORequestChannel::CLIENT_SIDE);
	}

	if (filename == "") {
		// Single datapoint call, only to be ran if p, t, e all specified
		if (p > 0 && t > 0 && e > 0) {
			char buf[MAX_MESSAGE]; // 256
			datamsg x(p, t, e);
			memcpy(buf, &x, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan->cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}

		// Either t or e aren't specified, return 1000 datapoints
		if (p > 0) {
			if (t < 0 || e < 0) {
				char buf[MAX_MESSAGE]; // 256
				ofstream writeout("received/x1.csv");

				for (int i = 0; i < 1000; i++) {
					double r_e1, r_e2;

					double time = i * 0.004;
					datamsg e1(p, time, 1);
					memcpy(buf, &e1, sizeof(datamsg));
					chan->cwrite(buf, sizeof(datamsg));
					chan->cread(&r_e1, sizeof(double));

					datamsg e2(p, time, 2);
					memcpy(buf, &e2, sizeof(datamsg));
					chan->cwrite(buf, sizeof(datamsg));
					chan->cread(&r_e2, sizeof(double));

					writeout << time << "," << r_e1 << "," << r_e2 << endl;
				}
			}
	} 
	} else {
		// file requests
		
		filemsg fm(0, 0);
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());

		// write the file name into our new request channel
		r_chan->cwrite(buf2, len);
		__int64_t f_size;
		r_chan->cread(&f_size, sizeof(__int64_t));

		// open output, needing path to open file in path, and detail where to write
		std::string write_path = "received/" + filename;
		FILE* writeout = fopen(write_path.c_str(), "wb");

		__int64_t count_transfer_bytes = 0;
		char* f_buffer = new char[m];
		
		while (count_transfer_bytes < f_size) {
			
			// number of bytes remaining in to chunk
			__int64_t bytes_left = f_size - count_transfer_bytes;
			int cap_bytechunk = (bytes_left < m) ? bytes_left : m;

			// create a file message with the size of the byte chunk we want to allow
			// we can only read in 100 bytes at a time
			filemsg fmsg(count_transfer_bytes, cap_bytechunk);
			len = sizeof(filemsg) + filename.length() + 1;
			delete[] buf2;
			buf2 = new char[len];
			memcpy(buf2, &fmsg, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());

			// send request and wait for server reply
			r_chan->cwrite(buf2, len);
			int received_bytes = r_chan->cread(f_buffer, cap_bytechunk);

			// write received chunk to disk
			fwrite(f_buffer, 1, received_bytes, writeout);
			count_transfer_bytes += received_bytes;
		}

		fclose(writeout);
		delete[] f_buffer;
		delete[] buf2;
	}

	// closing the channel
    MESSAGE_TYPE q = QUIT_MSG;

	if (channel_arg && r_chan != chan) {
		r_chan->cwrite(&q, sizeof(MESSAGE_TYPE));
		delete r_chan;
	}
	
    chan->cwrite(&q, sizeof(MESSAGE_TYPE));
	delete chan;
	wait(NULL);

	return 0;
}
