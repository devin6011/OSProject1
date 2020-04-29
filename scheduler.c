#define _GNU_SOURCE
#include <sched.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* type declarations */

enum SchedulingPolicy {
	FIFO, RR, SJF, PSJF
};

struct Process {
	char name[128];
	int readyTime;
	int executionTime;
	int pid;
	int originalIndex;
};

/* global variable declarations */

enum SchedulingPolicy policy;
struct Process processes[256];
int numFinishProcess;

const int RRTimeQuantum = 500;
int RRUsedTime;

int currentRunningProcess;
int nextProcessToRun;

// a circular queue, also used for priority queue
int queue[1<<9];
int queueHead, queueTail, queueCapacity = 1<<9, queueSize;
int queueMask = (1<<9) - 1;

/* function declarations */

void setCPU(pid_t pid, int cpu) {
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
#ifdef DEBUG
	if(sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
		perror("Fail to set CPU affinity");
		exit(1);
	}
#else
	sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set);
#endif
}

void setPriority(pid_t pid, int priority) {
	struct sched_param param;
	param.sched_priority = priority;
#ifdef DEBUG
	if(sched_setscheduler(pid, SCHED_FIFO, &param) != 0) {
		perror("Fail to set priority");
		exit(1);
	}
#else
	sched_setscheduler(pid, SCHED_FIFO, &param);
#endif
}

void createProcess(struct Process *process) {
	pid_t pid = fork();
	if(pid < 0) {
		perror("Fork error");
		exit(1);
	}
	else if(pid == 0) { // child
		pid = getpid();

		printf("%s %d\n", process->name, pid);
		fflush(stdout);

		struct timespec startTime, finishTime;
		syscall(452, &startTime.tv_sec, &startTime.tv_nsec);
		for(int i = 0; i < process->executionTime; i++) {
			volatile unsigned long k;
			for(k = 0; k < 1000000UL; k++);
		}
		syscall(452, &finishTime.tv_sec, &finishTime.tv_nsec);

		syscall(451, pid, startTime.tv_sec, startTime.tv_nsec, finishTime.tv_sec, finishTime.tv_nsec);
#ifdef DEBUG
		fprintf(stderr, "[Project 1] %d %ld.%09ld %ld.%09ld\n", pid, startTime.tv_sec, startTime.tv_nsec, finishTime.tv_sec, finishTime.tv_nsec);
#endif
		exit(0);
	}
	else { // parent
		setCPU(pid, 1);
		process->pid = pid;
	}
}

void initProcessQueue() {
	queueHead = queueTail = queueSize = 0;
}

void get2QueueMinPos(int *firstMin, int *secondMin) {
	if(queueHead == queueTail) {
		*firstMin = -1, *secondMin = -1;
		return;
	}
	int minValue = processes[queue[queueHead]].executionTime, minIndex = queueHead, minOriIndex = processes[queue[queueHead]].originalIndex;
	int secondMinValue = 2147483647, secondMinIndex = -1, secondMinOriIndex = 2147483647;
	int pos = (queueHead + 1) & queueMask;
	while(pos != queueTail) {
		int value = processes[queue[pos]].executionTime;
		int oriIndex = processes[queue[pos]].originalIndex;
		if(value < minValue || (value == minValue && oriIndex < minOriIndex)) {
			secondMinValue = minValue, secondMinIndex = minIndex, secondMinOriIndex = minOriIndex;
			minValue = value, minIndex = pos, minOriIndex = oriIndex;
		}
		else if(value < secondMinValue || (value == secondMinValue && oriIndex < secondMinOriIndex)) {
			secondMinValue = value, secondMinIndex = pos, secondMinOriIndex = oriIndex;
		}
		pos = (pos + 1) & queueMask;
	}
	*firstMin = minIndex, *secondMin = secondMinIndex;
}

void pushProcess(int processIndex) {
	queue[queueTail] = processIndex;
	queueTail = (queueTail + 1) & queueMask;
	queueSize++;
#ifdef DEBUG
	if(queueTail == queueHead) {
		fprintf(stderr, "Corrupt queue!");
		exit(1);
	}
#endif
}

int popProcess() {
#ifdef DEBUG
	if(queueHead == queueTail) {
		fprintf(stderr, "Corrupt queue!");
		exit(1);
	}
#endif
	int ret = queue[queueHead];
	queueHead = (queueHead + 1) & queueMask;
	queueSize--;
	return ret;
}

void selectProcessToRun() {
	if(queueHead == queueTail) {
		currentRunningProcess = -1;
		nextProcessToRun = -1;
		return;
	}
	if(queueSize == 1)
		nextProcessToRun = -1;
	int firstChoice = -1, secondChoice = -1;
	int queueFirstMinPos, queueSecondMinPos;

	if(policy == FIFO || policy == RR) {
		firstChoice = queue[queueHead];
		if(((queueHead + 1) & queueMask) != queueTail)
			secondChoice = queue[(queueHead + 1) & queueMask];
	}
	else {
		get2QueueMinPos(&queueFirstMinPos, &queueSecondMinPos);
		firstChoice = queue[queueFirstMinPos];
		if(queueSecondMinPos != -1)
			secondChoice = queue[queueSecondMinPos];
	}

	if(policy == FIFO || policy == SJF) { //non-preemptive
		if(currentRunningProcess != firstChoice) {
			if(currentRunningProcess == -1) {
				setPriority(processes[firstChoice].pid, 97);
				currentRunningProcess = firstChoice;
				if(policy == SJF) {
					int temp = queue[queueHead];
					queue[queueHead] = queue[queueFirstMinPos];
					queue[queueFirstMinPos] = temp;
				}
			}
			else {
				if(nextProcessToRun != firstChoice) {
					setPriority(processes[firstChoice].pid, 50);
					if(nextProcessToRun != -1 && processes[nextProcessToRun].executionTime > 0)
						setPriority(processes[nextProcessToRun].pid, 3);
					nextProcessToRun = firstChoice;
				}
				secondChoice = -1;
			}
		}
		if(secondChoice != -1 && nextProcessToRun != secondChoice) {
			setPriority(processes[secondChoice].pid, 50);
			if(nextProcessToRun != -1 && nextProcessToRun != currentRunningProcess && processes[nextProcessToRun].executionTime > 0)
				setPriority(processes[nextProcessToRun].pid, 3);
			nextProcessToRun = secondChoice;
		}
	}
	else { // preemptive
		if(currentRunningProcess != firstChoice) {
			setPriority(processes[firstChoice].pid, 97);
			if(currentRunningProcess != -1)
				setPriority(processes[currentRunningProcess].pid, 3);
			currentRunningProcess = firstChoice;
			if(policy == PSJF) {
				int temp = queue[queueHead];
				queue[queueHead] = queue[queueFirstMinPos];
				queue[queueFirstMinPos] = temp;
			}
		}
		if(secondChoice != -1 && nextProcessToRun != secondChoice) {
			setPriority(processes[secondChoice].pid, 50);
			if(nextProcessToRun != -1 && nextProcessToRun != currentRunningProcess && processes[nextProcessToRun].executionTime > 0)
				setPriority(processes[nextProcessToRun].pid, 3);
			nextProcessToRun = secondChoice;
		}
	}
	// for(int i = 0; i < 8; ++i) {
	// 	if(processes[i].executionTime <= 0 || processes[i].pid == -1) continue;
	// 	struct sched_param param;
	// 	sched_getparam(processes[i].pid, &param);
	// 	fprintf(stderr, "%d ", param.sched_priority);
	// }
	// fprintf(stderr, "\n");
}

int main() {
	char schedulingPolicy[10];
	int numProcess;
	if(scanf("%9s %d", schedulingPolicy, &numProcess) != 2) {
		fprintf(stderr, "Error occured when reading input");
		exit(1);
	}

	for(int i = 0; i < numProcess; i++) {
		if(scanf("%127s %d %d", processes[i].name, &processes[i].readyTime, &processes[i].executionTime) != 3) {
			fprintf(stderr, "Error occured when reading input");
			exit(1);
		}
		processes[i].pid = -1;
		processes[i].originalIndex = i;
	}
	
	if(strcmp(schedulingPolicy, "FIFO") == 0) {
		policy = FIFO;
	}
	else if(strcmp(schedulingPolicy, "RR") == 0) {
		policy = RR;
	}
	else if(strcmp(schedulingPolicy, "SJF") == 0) {
		policy = SJF;
	}
	else if(strcmp(schedulingPolicy, "PSJF") == 0) {
		policy = PSJF;
	}
	else {
		fprintf(stderr, "No such scheduling policy\n");
		exit(1);
	}

	setCPU(0, 0);

	setPriority(0, 3);

	initProcessQueue();
	numFinishProcess = 0;
	currentRunningProcess = -1;
	nextProcessToRun = -1;
	int reselectFlag = 0;
	if(policy == RR)
		RRUsedTime = 0;
	for(int t = 0, i = 0; numFinishProcess < numProcess; t++) {
		while(i < numProcess && t == processes[i].readyTime) {
			createProcess(&processes[i]);
			pushProcess(i);
			reselectFlag = 1;
			i++;
		}
		if(reselectFlag)
			selectProcessToRun(), reselectFlag = 0;

		{
			volatile unsigned long k;
			for(k = 0; k < 1000000UL; k++);
		}

		if(currentRunningProcess == -1)
			continue;

		if(policy == RR)
			RRUsedTime++;
		processes[currentRunningProcess].executionTime--;

		if(processes[currentRunningProcess].executionTime <= 0) {
			popProcess();
			numFinishProcess++;
			waitpid(processes[currentRunningProcess].pid, NULL, 0);
			currentRunningProcess = -1;
			if(policy == RR)
				RRUsedTime = 0;
			reselectFlag = 1;
			continue;
		}

		if(policy == RR && RRUsedTime >= RRTimeQuantum && queueSize >= 2) {
			pushProcess(popProcess());
			selectProcessToRun();
			RRUsedTime = 0;
		}
	}
	return 0;
}
