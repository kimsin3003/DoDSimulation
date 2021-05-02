#pragma once
#include "ECS/SystemBase.h"
#include "ECS/Database.h"
#include "ECS/Query.h"
#include "Mover.h"
#include "Door.h"
#include "ECSBaseComponents.h"

class TestSystem : public SystemBase
{
public:
	virtual void Update(float deltaTime, Database* DB) override;
};

