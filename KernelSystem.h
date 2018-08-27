#pragma once

#include "vm_declarations.h"
#include "part.h"
#include "queue"

#define PMT_SIZE 0x100

using namespace std;


class Process;
class KernelProcess;

struct Element
{
	Process* process;
	Element* next;
	Element(Process* p)
	{
		process = p;
		next = nullptr;
	}
};

struct Frame
{
	ProcessId pid;
	PageNum frame;
	PageNum page;
	Frame* next;
	Frame(ProcessId pid, PageNum frame, PageNum page)
	{
		this->pid = pid;
		this->frame = frame;
		this->page = page;
		next = nullptr;
	}
};

class KernelSystem
{
private:
	ProcessId PID;

	PhysicalAddress vmSpace;
	PhysicalAddress pmtSpace;

	PageNum vmSize;
	PageNum pmtSize;

	Partition* partition;
	ClusterNo diskSize;

	Element* first;
	Element* last;

	PageNum freePMTSize;
	PageNum freePageSize;
	ClusterNo freeClusterSize;

	queue<int> freePMT;
	queue<int> freePage;
	queue<int> freeCluster;

	Frame* firstFrame;
	Frame* lastFrame;
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition);
	~KernelSystem();
	Process* createProcess();
	void registerProcess(Process* process);
	void removeProcess(KernelProcess* process);
	KernelProcess* getProcess(ProcessId pid);
	Status access(ProcessId pid, VirtualAddress address, AccessType access);
	Time periodicJob();

	friend class System;
	friend class KernelProcess;
};