#pragma once
#include "Scheduler.h"

class SystemBase
{
public:
	virtual ~SystemBase() {}
	virtual void Update(float deltaTime, const Scheduler& scheduler) = 0;
};

