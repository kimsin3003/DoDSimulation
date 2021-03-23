#pragma once
#include "ECS/SystemBase.h"
#include "ECS/Scheduler.h"
#include "ECS/Query.h"
#include "Mover.h"
#include "Door.h"

class TestSystem : SystemBase
{
public:
	virtual void Update(const Scheduler& scheduler) override;

	Query<AMover&, const ADoor&> _query;
};

