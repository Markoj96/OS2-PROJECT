#include "KernelProcess.h"
#include "KernelSystem.h"
#include "system.h"
#include "process.h"
#include <iostream>

using namespace std;

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition)
{
	this->PID = 0;
	this->vmSpace = processVMSpace;
	this->vmSize = processVMSpaceSize;
	this->pmtSpace = pmtSpace;
	this->pmtSize = pmtSpaceSize;
	this->partition = partition;
	this->diskSize = partition->getNumOfClusters();
	this->first = nullptr;
	this->last = nullptr;
	this->firstFrame = nullptr;
	this->lastFrame = nullptr;
	this->freePMTSize = this->pmtSize;
	this->freePageSize = this->vmSize;
	this->freeClusterSize = this->diskSize;

	for (int i = 0; i*PMT_SIZE < pmtSize; i++)
	{
		freePMT.push(i);
	}

	for (int i = 0; i < vmSize; i++)
	{
		freePage.push(i);
	}

	for (int i = 0; i < diskSize; i++)
	{
		freeCluster.push(i);
	}
}

KernelSystem::~KernelSystem()
{
	while (first)
	{
		Element* temp = first;
		first = first->next;
		delete temp->process;
		delete temp;
	}

	while (firstFrame)
	{
		Frame* temp = firstFrame;
		firstFrame = firstFrame->next;
		delete temp;
	}

	first = nullptr;
	last = nullptr;
	firstFrame = nullptr;
	lastFrame = nullptr;
}

Process* KernelSystem::createProcess()
{
	if (this->freePMTSize == 0)
	{
		return nullptr;
	}

	Process* process = new Process(PID++);

	registerProcess(process);
	process->pProcess->registerSystem(this);

	PageNum pageNum = this->freePMT.front();
	process->pProcess->pmtp = (PMTEntry*)((VirtualAddress)this->pmtSpace + pageNum*PMT_SIZE*PAGE_SIZE);
	this->freePMT.pop();
	this->freePMTSize--;

	return process;
}

Time KernelSystem::periodicJob()
{
	return 0;
}

Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType access)
{
	KernelProcess* process = this->getProcess(pid);
	if (process == nullptr)
	{
		return Status::TRAP;
	}

	if ((address & 0xFFFFFF) != address)
	{
		return Status::TRAP;
	}

	PageNum page = address >> 10;
	if (process->pmtp[page].access == AccessType::READ_WRITE)
	{
		if (access == AccessType::EXECUTE)
		{
			return Status::TRAP;
		}
	}
	else if (process->pmtp[page].access != access)
	{
		return Status::TRAP;
	}

	if (process->pmtp[page].V == 0)
	{
		return Status::PAGE_FAULT;
	}

	if (access == AccessType::WRITE)
	{
		process->pmtp[page].D = 1;
	}

	return Status::OK;
}

void KernelSystem::removeProcess(KernelProcess* process)
{
	Element* temp = this->first;
	Element* last_temp = nullptr;

	while (temp && temp->process->getProcessId() != process->getProcessId())
	{
		last_temp = temp;
		temp = temp->next;
	}

	if(temp)
	{
		if(!last_temp)
		{
			this->first = this->first->next;
		}
		else
		{
			last_temp->next = temp->next;
		}
		if(temp == this->last)
		{
			this->last = last_temp;
		}

		temp->process = nullptr;
		temp->next = nullptr;
		delete temp;
	}
}

void KernelSystem::registerProcess(Process* process)
{
	Element* newProcess = new Element(process);
	if (!first)
	{
		first = newProcess;
	}
	else
	{
		last->next = newProcess;
	}
	last = newProcess;
}

KernelProcess* KernelSystem::getProcess(ProcessId pid)
{
	Element* temp = this->first;

	while (temp && temp->process->getProcessId() != pid)
	{
		temp = temp->next;
	}

	if (temp)
	{
		return temp->process->pProcess;
	}
	
	return nullptr;
}