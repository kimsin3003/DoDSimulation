#pragma once
#include "Scheduler.h"

class SystemBase
{
public:
	virtual ~SystemBase() {}
	virtual void Update(const Scheduler& scheduler) = 0;
};

