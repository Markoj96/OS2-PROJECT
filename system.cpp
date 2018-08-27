#include "system.h"
#include "KernelSystem.h"

System::System(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition)
{
	this->pSystem = new KernelSystem(processVMSpace, processVMSpaceSize, pmtSpace, pmtSpaceSize, partition);
}

System::~System()
{
	delete this->pSystem;
	this->pSystem = nullptr;
}

Process* System::createProcess()
{
	return this->pSystem->createProcess();
}

Time System::periodicJob()
{
	return this->pSystem->periodicJob();
}

Status System::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	return this->pSystem->access(pid, address, type);
}