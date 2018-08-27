#pragma once

#include "vm_declarations.h"
#include "part.h"

using namespace std;
class KernelSystem;

struct Segment
{
	VirtualAddress startAddress;
	PageNum size;
	AccessType flags;
	void* content;
	Segment *next;
	Segment *prev;
	Segment(VirtualAddress startAddress, PageNum size, AccessType flags)
	{
		this->startAddress = startAddress;
		this->size = size;
		this->flags = flags;
		this->content = nullptr;
		this->next = nullptr;
		this->prev = nullptr;
	}
};

struct PMTEntry
{
	unsigned char V;
	unsigned char D;
	AccessType access;
	unsigned char P;
	PageNum frame;
	ClusterNo cluster;
};

class KernelProcess
{
private:
	ProcessId PID;
	KernelSystem* system;
	PMTEntry* pmtp;
	Segment* firstSegment;
	Segment* lastSegment;
public:
	KernelProcess(ProcessId pid);
	~KernelProcess();
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	void registerSystem(KernelSystem* system);

	friend class System;
	friend class Process;
	friend class KernelSystem;
};