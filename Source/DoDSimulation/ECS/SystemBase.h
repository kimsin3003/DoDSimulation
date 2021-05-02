#pragma once

class SystemBase
{
public:
	virtual ~SystemBase() {}
	virtual void Update(float deltaTime, class Database* DB) = 0;
};

