#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include <vector>
using namespace std;
struct Process {
	string name;
	int readyTime;
	int executionTime;
};
void calcFIFO(vector<Process>&);
void calcRR(vector<Process>&);
void calcSJF(vector<Process>&);
void calcPSJF(vector<Process>&);
int main() {
	string str;
	int numProcess;
	cin >> str >> numProcess;
	vector<Process> processes(numProcess);
	for(int i = 0; i < numProcess; ++i) {
		cin >> processes[i].name >> processes[i].readyTime >> processes[i].executionTime;
	}

	if(str == "FIFO") {
		calcFIFO(processes);
	}
	else if(str == "RR") {
		calcRR(processes);
	}
	else if(str == "SJF") {
		calcSJF(processes);
	}
	else if(str == "PSJF") {
		calcPSJF(processes);
	}
	else {
		cerr << "This scheduling algorithm is not supported." << endl;
		exit(1);
	}
	return 0;
}

void print(const vector<Process> &processes, const vector<pair<int, int>> &sol) {
	for(int i = 0; i < (int)processes.size(); ++i) {
		const auto &name = processes[i].name;
		const auto &startTime = sol[i].first;
		const auto &finishTime = sol[i].second;
		cout << name << ' ' << startTime << ' ' << finishTime << endl;
		//cout << name << ' ' << finishTime - startTime << endl;
	}
}

void calcFIFO(vector<Process> &processes) {
	//stable_sort(begin(processes), end(processes), [](const auto &x, const auto &y) {return x.readyTime < y.readyTime;});
	int time = 0;
	vector<pair<int, int>> sol;
	for(const auto &process : processes) {
		int startTime, endTime;
		if(time < process.readyTime)
			time = process.readyTime;
		startTime = time;
		time += process.executionTime;
		endTime = time;
		sol.emplace_back(startTime, endTime);
	}
	print(processes, sol);
}

void calcRR(vector<Process> &processes) {
	//stable_sort(begin(processes), end(processes), [](const auto &x, const auto &y) {return x.readyTime < y.readyTime;});
	queue<int> Q;
	vector<pair<int, int>> sol(processes.size());
	for(auto &x : sol)
		x.first = -1, x.second = -1;
	constexpr int timeQuantum = 500;
	int time = 0;
	int runningTime = 0;
	int finishCnt = 0;
	for(int i = 0; finishCnt < (int)processes.size(); ) {
		while(i < (int)processes.size() and time >= processes[i].readyTime)
			Q.push(i), ++i;
		if(!Q.empty()) {
			if(sol[Q.front()].first == -1)
				sol[Q.front()].first = time;
			time++;
			processes[Q.front()].executionTime--;
			runningTime++;
			if(processes[Q.front()].executionTime <= 0) {
				//auto x = Q.front();
				//cout << processes[x].name << ' ' <<
				//	time << '\t';
				//int t = Q.front();
				//int tt = t;
				//do {
				//	cout << tt;
				//	Q.push(Q.front());
				//	Q.pop();
				//	tt = Q.front();
				//}while(tt!=t);
				//cout << endl;
				runningTime = 0, sol[Q.front()].second = time, finishCnt++, Q.pop();
			}
			else if(runningTime == timeQuantum) {
				auto x = Q.front();
				//cout << processes[x].name << ' ' <<
				//	time << '\t';
				//int t = Q.front();
				//int tt = t;
				//do {
				//	cout << tt;
				//	Q.push(Q.front());
				//	Q.pop();
				//	tt = Q.front();
				//}while(tt!=t);
				//cout << endl;

				Q.pop();
				Q.push(x);
				runningTime = 0;
			}
		}
		else time++;
	}
	print(processes, sol);
}

void calcSJF(vector<Process> &processes) {
	//stable_sort(begin(processes), end(processes), [](const auto &x, const auto &y) {return x.readyTime < y.readyTime;});
	priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> PQ;
	int time = 0;
	vector<pair<int, int>> sol(processes.size());
	for(auto &x : sol)
		x.first = x.second = -1;
	int numFinish = 0;
	for(int i = 0; numFinish < int(processes.size()); ) {
		while(i < int(processes.size()) and time >= processes[i].readyTime)
			PQ.emplace(processes[i].executionTime, i), i++;
		if(!PQ.empty()) {
			auto [et, x] = PQ.top(); PQ.pop();
			sol[x].first = time;
			time += et;
			sol[x].second = time;
			numFinish++;
		}
		else time++;
	}
	print(processes, sol);
}

void calcPSJF(vector<Process> &processes) {
	//stable_sort(begin(processes), end(processes), [](const auto &x, const auto &y) {return x.readyTime < y.readyTime;});
	priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> PQ;
	int time = 0;
	vector<pair<int, int>> sol(processes.size());
	for(auto &x : sol)
		x.first = x.second = -1;
	int numFinish = 0;
	for(int i = 0; numFinish < (int)processes.size(); ) {
		while(i < (int)processes.size() and time >= processes[i].readyTime)
			PQ.emplace(processes[i].executionTime, i), i++;
		if(!PQ.empty()) {
			auto [et, x] = PQ.top(); PQ.pop();
			if(sol[x].first == -1)
				sol[x].first = time;
			time++;
			et -= 1;
			if(et <= 0) {
				sol[x].second = time;
				numFinish++;
			}
			else {
				PQ.emplace(et, x);
			}
		}
		else time++;
	}
	print(processes, sol);
}
