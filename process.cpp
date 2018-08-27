#include "process.h"
#include "KernelProcess.h"

Process::Process(ProcessId pid)
{
	this->pProcess = new KernelProcess(pid);
}

Process::~Process()
{
	delete this->pProcess;
	this->pProcess = nullptr;
}

ProcessId Process::getProcessId() const
{
	return this->pProcess->getProcessId();
}

Status Process::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags)
{
	if (segmentSize > 0)
	{
		return this->pProcess->createSegment(startAddress, segmentSize, flags);
	}

	return Status::TRAP;
}

Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content)
{
	if (segmentSize > 0)
	{
		return this->pProcess->loadSegment(startAddress, segmentSize, flags, content);
	}

	return Status::TRAP;
}

Status Process::deleteSegment(VirtualAddress startAddress)
{
	return this->pProcess->deleteSegment(startAddress);
}

Status Process::pageFault(VirtualAddress address)
{
	return this->pProcess->pageFault(address);
}

PhysicalAddress Process::getPhysicalAddress(VirtualAddress address)
{
	return this->pProcess->getPhysicalAddress(address);
}