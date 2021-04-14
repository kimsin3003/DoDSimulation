#pragma once
#include "ECS/SystemBase.h"
#include "ECS/Scheduler.h"
#include "ECS/Query.h"
#include "Mover.h"
#include "Door.h"

class TestSystem : public SystemBase
{
public:
	virtual void Update(float deltaTime, const Scheduler& scheduler) override;

	Query<AMover&, ADoor&> Query;
};

