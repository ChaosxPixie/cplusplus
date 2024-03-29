//Name: Fuentes, Malana
//Email: mvu21@lsu.edu
//Project: PA-1 (Multithreadding)
//Instructor: Feng Chen
//Class: cs4103-sp24
//Server Login: cs410327

#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <thread> 
#include <ctime>
#include <condition_variable>
#include <unistd.h>

using namespace std;

const string LOG_FILE = "service.log";

class JobPool {
private:
    vector<int> pool;
    vector<int> client_jobs;
    vector<int> server_jobs;
    mutex mtx;
    condition_variable cv;
    int num_created_jobs;
    int num_processed_jobs;
	int total_jobs;

public:
    JobPool(int num_pool_entries, int total_jobs, int num_clients, int num_servers) 
	: total_jobs(total_jobs), num_created_jobs(0), num_processed_jobs(0) {
        pool.resize(num_pool_entries, -1);
		client_jobs.resize(num_clients,0);
		server_jobs.resize(num_servers, 0);
    }

    void addJob(int clientId) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return num_created_jobs < total_jobs; });
		int jobId = num_created_jobs++;
		for (size_t i = 0; i < pool.size(); ++i) {
			if (pool[i] == -1){
				pool[i] = jobId;
				client_jobs[clientId]++;
				logEvent("Client", clientId, i, jobId);
				break;
			}
		}
		cv.notify_all();
	}

    int removeJob(int serverId) {
		unique_lock<mutex> lock(mtx);
		cv.wait(lock, [this] { return num_processed_jobs < total_jobs; });
		int jobId = -1; 
		for (size_t i = 0; i <pool.size(); ++i) {
			if (pool[i] != -1) { 
				jobId = pool[i];
				pool[i] = -1;
				num_processed_jobs++;
				server_jobs[serverId]++;
				logEvent("Server", serverId, i, jobId);
				break;
			}
		}
		cv.notify_all();
		return jobId;
	}

	void printClientJobs() {
		for (size_t i = 0; i < client_jobs.size(); ++i) {
        cout << "Client " << i << " created " << client_jobs[i] << " jobs." << endl;
		}
	}

	void printServerJobs() {
		for (size_t i = 0; i < server_jobs.size(); ++i) {
        cout << "Server " << i << " processed " << server_jobs[i] << " jobs." << endl;
		}
	}

private:
	void logEvent(const std::string& threadType, int threadId, int poolIndex, int jobId) {
		std::ofstream log(LOG_FILE, std::ios::app);
		if (log.is_open()) {
			time_t currentTime = time(nullptr); // Get the current time
			log << currentTime << " " << threadType << " " << threadId << " " << poolIndex << " " << jobId << std::endl;
			log.close();
		}
	}
};

void processJob(int jobId, int serverId, int processingTime) {
    usleep(processingTime * 1000);
}

void clientFunc(JobPool& jobPool, int id, int totalJobs, int processingTime) {
    for (int i = 0; i < totalJobs; ++i) {
        jobPool.addJob(id);
		usleep(processingTime * 1000);
    }
}

void serverFunc(JobPool& jobPool, int id, int totalJobs, int processingTime) {
    for (int i = 0; i < totalJobs; ++i) {
        int jobId = jobPool.removeJob(id);
        if (jobId != -1) {
            thread t(processJob, jobId, id, processingTime);
			t.join();
        }
		usleep(processingTime * 1000);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cout << "Usage: " << argv[0] << " <num_client_threads> <num_server_threads> <num_pool_entries> <total_jobs> <processing_time>" << endl;
        return 1;
    }

    int num_client_threads = atoi(argv[1]);
    int num_server_threads = atoi(argv[2]);
    int num_pool_entries = atoi(argv[3]);
    int total_jobs = atoi(argv[4]);
    int processing_time = atoi(argv[5]);

    if (num_client_threads <= 0 || num_server_threads <= 0 || num_pool_entries <= 0 || total_jobs <= 0 || processing_time <= 0) {
        cout << "Error: Invalid input parameters. Please provide positive integers greater than 0 for all parameters." << endl;
        return 1;
    }

    JobPool jobPool(num_pool_entries, total_jobs, num_client_threads, num_server_threads);

    vector<thread> clientThreads;
    vector<thread> serverThreads;

    for (int i = 0; i < num_client_threads; i++) {
        clientThreads.emplace_back(clientFunc, ref(jobPool), i, total_jobs/num_client_threads, processing_time);
    }

    for (int i = 0; i < num_server_threads; i++) {
        serverThreads.emplace_back(serverFunc, ref(jobPool), i, total_jobs/num_server_threads, processing_time);
    }

    for (auto& t : clientThreads) {
        t.join();
    }

    for (auto& t : serverThreads) {
        t.join();
    }

    cout << "Job processing completed!" << endl;
    jobPool.printClientJobs();
    jobPool.printServerJobs();
    cout << "Check the log file '" << LOG_FILE << "' for details." << endl;

    return 0;
}

