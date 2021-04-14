#include "TestSystem.h"
#include <functional>
#include <stdio.h>

void TestSystem::Update(float deltaTime, const Scheduler& scheduler)
{
	Query.Each(*(scheduler.DB), [deltaTime](int32 EntityId, AMover& mover, ADoor& door)
	{
		mover.Tick(deltaTime);
		door.Tick(deltaTime);
	});
}

