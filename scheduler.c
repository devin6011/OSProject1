#define _GNU_SOURCE
#include <sched.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
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
	int isFinish;
	int originalIndex;
};

/* global variable declarations */

enum SchedulingPolicy policy;
struct Process *processes;
int numFinishProcess;

const int RRTimeQuantum = 500;
int RRUsedTime;

int reselectFlag;

int runningTime;

// a circular queue, also used for priority queue
int* queue;
int queueHead, queueTail, queueCapacity, queueSize;

/* function declarations */

int cmp(const void *a, const void *b) {
	struct Process *pA = (struct Process*)a;
	struct Process *pB = (struct Process*)b;
	if(pA->readyTime == pB->readyTime && pA->originalIndex == pB->originalIndex)
		return 0;
	if(pA->readyTime < pB->readyTime || (pA->readyTime == pB->readyTime && pA->originalIndex < pB->originalIndex))
		return -1;
	return 1;
}

void setCPU(pid_t pid, int cpu) {
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
	if(sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
		perror("Fail to set CPU affinity");
		exit(1);
	}
}

void setPriority(pid_t pid, int priority) {
	struct sched_param param;
	param.sched_priority = priority;
	if(sched_setscheduler(pid, SCHED_FIFO, &param) != 0) {
		perror("Fail to set priority");
		exit(1);
	}
}

void unitTime() {
	volatile unsigned long i;
	for(i = 0;i < 1000000UL; i++);
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
		clock_gettime(CLOCK_REALTIME, &startTime);
		for(int i = 0; i < process->executionTime; i++)
			unitTime();
		clock_gettime(CLOCK_REALTIME, &finishTime);

		syscall(451, pid, startTime.tv_sec, startTime.tv_nsec, finishTime.tv_sec, finishTime.tv_nsec);
#ifdef DEBUG
		fprintf(stderr, "[Project 1] %d %ld.%09ld %ld.%09ld\n", pid, startTime.tv_sec, startTime.tv_nsec, finishTime.tv_sec, finishTime.tv_nsec);
#endif
		exit(0);
	}
	else { // parent
		setCPU(pid, 1);
		process->pid = pid;
		setPriority(process->pid, 3);
		kill(pid, SIGCONT);
	}
}

void initProcessQueue(int numProcess) {
	queueCapacity = numProcess + 20;
	queue = malloc(sizeof(int) * queueCapacity);
	queueHead = 0;
	queueTail = 0;
	queueSize = 0;
}

void destroyProcessQueue() {
	free(queue);
}

int getQueueMinPos() {
	if(queueHead == queueTail) {
		fprintf(stderr, "Corrupt queue!");
		exit(1);
	}
	int minValue = processes[queue[queueHead]].executionTime, minIndex = queueHead, minOriIndex = processes[queue[queueHead]].originalIndex;
	int pos = (queueHead + 1) % queueCapacity;
	while(pos != queueTail) {
		int value = processes[queue[pos]].executionTime;
		int oriIndex = processes[queue[pos]].originalIndex;
		if(value < minValue || (value == minValue && oriIndex < minOriIndex))
			minValue = value, minIndex = pos, minOriIndex = oriIndex;
		pos = (pos + 1) % queueCapacity;
	}
	return minIndex;
}

void pushProcess(int processIndex) {
	queue[queueTail] = processIndex;
	queueTail = (queueTail + 1) % queueCapacity;
	queueSize++;
	if(queueTail == queueHead) {
		fprintf(stderr, "Corrupt queue!");
		exit(1);
	}
}

int popProcess() {
	if(queueHead == queueTail) {
		fprintf(stderr, "Corrupt queue!");
		exit(1);
	}
	int ret = queue[queueHead];
	queueHead = (queueHead + 1) % queueCapacity;
	queueSize--;
	return ret;
}

void selectProcessToRun() {
	if(queueHead == queueTail) return;
	static int lastSelection = -1;
	int selection, queueMinPos = -1;
	if(lastSelection != -1)
		processes[lastSelection].executionTime -= runningTime, runningTime = 0;

	if(policy == FIFO || policy == RR)
		selection = queue[queueHead];
	else {
		queueMinPos = getQueueMinPos();
		selection = queue[queueMinPos];
	}
	if(selection != lastSelection) {
		if(policy == SJF) {
			if(lastSelection != -1 && !processes[lastSelection].isFinish)
				return;
			setPriority(processes[selection].pid, 97);
			lastSelection = selection;
		}
		else {
			setPriority(processes[selection].pid, 97);
			if(lastSelection != -1 && !processes[lastSelection].isFinish)
				setPriority(processes[lastSelection].pid, 3);
			lastSelection = selection;
		}
		if(policy == SJF || policy == PSJF) {
			int temp = queue[queueHead];
			queue[queueHead] = queue[queueMinPos];
			queue[queueMinPos] = temp;
		}
	}
}

void sigchldHandler(int signum) {
	wait(NULL);
	numFinishProcess++;
	if(policy == RR)
		RRUsedTime = 0;
	int theDead = popProcess();
	processes[theDead].isFinish = 1;
	//selectProcessToRun();
	reselectFlag = 1;
}

int main() {
	char schedulingPolicy[10];
	int numProcess;
	if(scanf("%9s %d", schedulingPolicy, &numProcess) != 2) {
		fprintf(stderr, "Error occured when reading input");
		exit(1);
	}

	processes = malloc(sizeof(struct Process) * numProcess);

	for(int i = 0; i < numProcess; i++) {
		if(scanf("%127s %d %d", processes[i].name, &processes[i].readyTime, &processes[i].executionTime) != 3) {
			fprintf(stderr, "Error occured when reading input");
			exit(1);
		}
		processes[i].pid = -1;
		processes[i].isFinish = 0;
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

	// sort processes by their readyTime
	qsort(processes, numProcess, sizeof(struct Process), cmp);

	// set host process to use CPU 0
	setCPU(0, 0);

	// set host process to use a low priority
	setPriority(0, 20);

	signal(SIGCHLD, sigchldHandler);

	// run scheduling
	initProcessQueue(numProcess);
	numFinishProcess = 0;
	reselectFlag = 0;
	runningTime = 0;
	if(policy == RR)
		RRUsedTime = 0;
	for(int t = 0, i = 0; ; t++) {
		while(i < numProcess && t == processes[i].readyTime) {
			createProcess(&processes[i]);
			pushProcess(i);
			i++;
			reselectFlag = 1;
		}
		if(reselectFlag)
			selectProcessToRun(), reselectFlag = 0;
		unitTime();
		runningTime++;
		if(numFinishProcess == numProcess)
			break;
		if(policy == RR) {
			RRUsedTime++;
			if(RRUsedTime >= RRTimeQuantum && queueSize >= 2) {
				int processIndex = popProcess();
				selectProcessToRun();
				pushProcess(processIndex);
				RRUsedTime = 0;
			}
		}
	}

	free(processes);
	destroyProcessQueue();
	return 0;
}
