#include "TestSystem.h"
#include <functional>
#include <stdio.h>

void TestSystem::Update(const Scheduler& scheduler)
{
	_query.Each(*scheduler.DB, [](AMover& mover, const ADoor& door)
	{
		printf("fuck");
	});
}

