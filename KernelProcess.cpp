#include "KernelProcess.h"
#include "KernelSystem.h"
#include <iostream>

using namespace std;

KernelProcess::KernelProcess(ProcessId pid)
{
	this->PID = pid;
	this->firstSegment = nullptr;
	this->lastSegment = nullptr;
}

KernelProcess::~KernelProcess()
{
	this->system->removeProcess(this);
	while (this->firstSegment)
	{
		VirtualAddress startAddress = this->firstSegment->startAddress;
		PageNum page = startAddress >> 10;
		for (int i = 0; i < this->firstSegment->size; i++)
		{
			if (this->pmtp[i + page].V == 1)
			{
				PageNum frame = this->pmtp[i + page].frame;
				Frame* temp = this->system->firstFrame;
				Frame* last_temp = nullptr;

				this->system->freePage.push(frame);
				this->system->freePageSize++;

				while (temp && temp->frame != frame)
				{
					last_temp = temp;
					temp = temp->next;
				}

				if (!last_temp)
				{
					this->system->firstFrame = this->system->firstFrame->next;
				}
				else
				{
					last_temp->next = temp->next;
				}

				if (temp == this->system->lastFrame)
				{
					this->system->lastFrame = last_temp;
				}

				delete temp;

				this->pmtp[i + page].V = 0;
			}

			if (this->pmtp[i + page].P == 1)
			{
				ClusterNo cluster = this->pmtp[i + page].cluster;

				this->system->freeCluster.push(cluster);
				this->system->freeClusterSize++;

				this->pmtp[i + page].P = 0;
			}
		}

		Segment* temp = this->firstSegment;
		this->firstSegment = this->firstSegment->next;
		delete temp;
	}

	int idPMT = ((VirtualAddress)this->pmtp - (VirtualAddress)this->system->pmtSpace) / (PMT_SIZE*PAGE_SIZE);
	this->pmtp = nullptr;
	this->system->freePMT.push(idPMT);

}

ProcessId KernelProcess::getProcessId() const
{
	return this->PID;
}

void KernelProcess::registerSystem(KernelSystem* system)
{
	this->system = system;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags)
{
	if (startAddress & 0x3FF)
	{
		return Status::TRAP;
	}

	if ((startAddress & 0xFFFFFF) != startAddress)
	{
		return Status::TRAP;
	}

	if (((startAddress + segmentSize*PAGE_SIZE) & 0xFFFFFF) != (startAddress + segmentSize*PAGE_SIZE))
	{
		return Status::TRAP;
	}

	Segment* temp = this->firstSegment;
	while (temp)
	{
		if ((startAddress >= temp->startAddress) && (startAddress < (temp->startAddress + temp->size*PAGE_SIZE)))
		{
			return Status::TRAP;
		}
		if ((temp->startAddress >= startAddress) && (temp->startAddress < (startAddress + segmentSize*PAGE_SIZE)))
		{
			return Status::TRAP;
		}
		temp = temp->next;
	}

	if (this->system->freeClusterSize >= segmentSize)
	{
		if(firstSegment == nullptr)
		{
			Segment* newSegment = new Segment(startAddress, segmentSize, flags);
			firstSegment = newSegment;
			lastSegment = newSegment;

			VirtualAddress firstPage = startAddress >> 10;
			for (PageNum i = 0; i < segmentSize; i++)
			{
				pmtp[i + firstPage].access = flags;
				pmtp[i + firstPage].V = 0;
				pmtp[i + firstPage].D = 0;

				ClusterNo clusterNo = this->system->freeCluster.front();
				this->system->freeCluster.pop();
				this->system->freeClusterSize--;

				pmtp[i + firstPage].cluster = clusterNo;
				pmtp[i + firstPage].P = 1;
			}

			return Status::OK;
		}
		else
		{
			Segment* newSegment = new Segment(startAddress, segmentSize, flags);
			lastSegment->next = newSegment;
			newSegment->prev = lastSegment;
			lastSegment = lastSegment->next;

			VirtualAddress firstPage = startAddress >> 10;
			for (PageNum i = 0; i < segmentSize; i++)
			{
				pmtp[i + firstPage].access = flags;
				pmtp[i + firstPage].V = 0;
				pmtp[i + firstPage].D = 0;

				ClusterNo clusterNo = this->system->freeCluster.front();
				this->system->freeCluster.pop();
				this->system->freeClusterSize--;

				pmtp[i + firstPage].cluster = clusterNo;
				pmtp[i + firstPage].P = 1;
			}

			return Status::OK;
		}
	}

	return Status::TRAP;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content)
{
	if (startAddress & 0x3FF)
	{
		return Status::TRAP;
	}

	if ((startAddress & 0xFFFFFF) != startAddress)
	{
		return Status::TRAP;
	}

	if (((startAddress + segmentSize*PAGE_SIZE) & 0xFFFFFF) != (startAddress + segmentSize*PAGE_SIZE))
	{
		return Status::TRAP;
	}

	Segment* temp = this->firstSegment;
	while (temp)
	{
		if ((startAddress >= temp->startAddress) && (startAddress < (temp->startAddress + temp->size*PAGE_SIZE)))
		{
			return Status::TRAP;
		}
		if ((temp->startAddress >= startAddress) && (temp->startAddress < (startAddress + segmentSize*PAGE_SIZE)))
		{
			return Status::TRAP;
		}
		temp = temp->next;
	}

	if (this->system->freeClusterSize >= segmentSize)
	{
		if (firstSegment == nullptr)
		{
			Segment* newSegment = new Segment(startAddress, segmentSize, flags);
			this->firstSegment = newSegment;
			this->lastSegment = newSegment;

			VirtualAddress firstPage = startAddress >> 10;
			for (PageNum i = 0; i < segmentSize; i++)
			{
				pmtp[i + firstPage].access = flags;
				pmtp[i + firstPage].V = 0;
				pmtp[i + firstPage].P = 0;
				pmtp[i + firstPage].D = 0;
			}

			if (content != nullptr)
			{
				for (PageNum i = 0; i < segmentSize; i++)
				{
					ClusterNo clusterNo = this->system->freeCluster.front();
					char* currentContent = (char*)content + PAGE_SIZE*i;
					this->system->partition->writeCluster(clusterNo, currentContent);
					this->system->freeCluster.pop();
					this->system->freeClusterSize--;

					pmtp[i + firstPage].cluster = clusterNo;
					pmtp[i + firstPage].P = 1;
				}
			}

			return Status::OK;
		}
		else
		{
			Segment* newSegment = new Segment(startAddress, segmentSize, flags);
			newSegment->content = content;
			this->lastSegment->next = newSegment;
			newSegment->prev = lastSegment;
			this->lastSegment = lastSegment->next;

			VirtualAddress firstPage = startAddress >> 10;
			for (PageNum i = 0; i < segmentSize; i++)
			{
				pmtp[i + firstPage].access = flags;
				pmtp[i + firstPage].V = 0;
				pmtp[i + firstPage].P = 0;
				pmtp[i + firstPage].D = 0;
			}

			if (content != nullptr)
			{
				for (PageNum i = 0; i < segmentSize; i++)
				{
					ClusterNo clusterNo = this->system->freeCluster.front();
					char* currentContent = (char*)content + PAGE_SIZE*i;
					this->system->partition->writeCluster(clusterNo, currentContent);
					this->system->freeCluster.pop();
					this->system->freeClusterSize--;

					pmtp[i + firstPage].cluster = clusterNo;
					pmtp[i + firstPage].P = 1;
				}
			}

			return Status::OK;
		}
	}

	return Status::TRAP;
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{
	if (startAddress & 0x3FF)
	{
		return Status::TRAP;
	}

	if ((startAddress & 0xFFFFFF) != startAddress)
	{
		return Status::TRAP;
	}

	Segment* temp = this->firstSegment;
	Segment* temp_last = nullptr;
	while (temp && temp->startAddress != startAddress)
	{
		temp_last = temp;
		temp = temp->next;
	}

	if (temp)
	{
		VirtualAddress firstPage = startAddress >> 10;
		for (int i = 0; i < temp->size; i++)
		{
			if (this->pmtp[i + firstPage].V == 1)
			{
				this->system->freePage.push(i + firstPage);
				this->system->freePageSize++;
				this->pmtp[i + firstPage].V = 0;
			}

			if (this->pmtp[i + firstPage].P == 1)
			{
				this->system->freeCluster.push(this->pmtp[i + firstPage].cluster);
				this->system->freeClusterSize++;
				this->pmtp[i + firstPage].P = 0;
			}
		}

		if (temp->prev)
		{
			if (temp == this->lastSegment)
			{
				temp->prev->next = nullptr;
				this->lastSegment = this->lastSegment->prev;
			}
			else
			{
				temp->prev->next = temp->next;
				temp->next->prev = temp->prev;
			}
		}
		else
		{
			if (temp == this->lastSegment)
			{
				this->firstSegment = nullptr;
				this->lastSegment = nullptr;
			}
			else
			{
				temp->next->prev = nullptr;
				this->firstSegment = this->firstSegment->next;
			}
		}

		delete temp;
		return Status::OK;
	}

	return Status::TRAP;
}

Status KernelProcess::pageFault(VirtualAddress address)
{
	if ((address & 0xFFFFFF) != address)
	{
		return Status::TRAP;
	}

	PageNum pageNum = address >> 10;
	if (this->pmtp[pageNum].V == 0)
	{
		if (this->system->freePageSize > 0)
		{
			PageNum freePage = this->system->freePage.front();
			this->system->freePage.pop();
			this->system->freePageSize--;

			char* destination = (char*)(this->system->vmSpace) + freePage*PAGE_SIZE;
			ClusterNo clusterNo = this->pmtp[pageNum].cluster;
			this->system->partition->readCluster(clusterNo, destination);

			if (this->system->firstFrame)
			{
				this->system->lastFrame->next = new Frame(this->PID, freePage, pageNum);
				this->system->lastFrame = this->system->lastFrame->next;
			}
			else
			{
				this->system->firstFrame = new Frame(this->PID, freePage, pageNum);
				this->system->lastFrame = this->system->firstFrame;
			}

			this->pmtp[pageNum].V = 1;
			this->pmtp[pageNum].frame = freePage;

			return Status::OK;
		}
		else
		{
			PageNum victim = this->system->firstFrame->frame;
			PageNum victimPage = this->system->firstFrame->page;
			KernelProcess* process = this->system->getProcess(this->system->firstFrame->pid);

			if (process->pmtp[victimPage].D == 1)
			{
				ClusterNo clusterNo = process->pmtp[victimPage].cluster;
				char* content = (char*)this->system->vmSpace + victim*PAGE_SIZE;
				this->system->partition->writeCluster(clusterNo, content);
				process->pmtp[victimPage].D = 0;
			}

			process->pmtp[victimPage].V = 0;

			char* destination = (char*)this->system->vmSpace + victim*PAGE_SIZE;
			ClusterNo clusterNo = this->pmtp[pageNum].cluster;
			this->system->partition->readCluster(clusterNo, destination);

			Frame* newFrame = new Frame(this->PID, victim, pageNum);
			this->system->lastFrame->next = newFrame;
			this->system->lastFrame = this->system->lastFrame->next;

			Frame* temp = this->system->firstFrame;
			this->system->firstFrame = this->system->firstFrame->next;

			delete temp;

			return Status::OK;
		}
	}

	return Status::TRAP;
}

PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{
	if ((address & 0xFFFFFF) != address)
	{
		return nullptr;
	}

	PageNum offset = address & 0x3FF;
	PageNum pageNum = address >> 10;

	if (this->pmtp[pageNum].V == 0)
	{
		return nullptr;
	}

	return (PhysicalAddress)((VirtualAddress)this->system->vmSpace + this->pmtp[pageNum].frame*PAGE_SIZE + offset);
}