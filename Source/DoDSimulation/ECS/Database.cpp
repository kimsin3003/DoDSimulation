#include "Database.h"
#include "EngineUtils.h"
#include "Engine/World.h"

Database::Database(UWorld* world) : _world(world)
{
}

int Database::AddActor(AActor* actor)
{
	const TSet<UActorComponent*>& compSet = actor->GetComponents();

	int entityId = _nextEntity++;

	for (auto comp : compSet)
	{
// 		FName compName = comp->StaticClass()->GetFName();
// 		DataUnit* data = new DataUnit();
// 		data->EntityId = entityId;
// 		data->Comp = comp;
// 		_data[compName.ToString()].Add(data);
	}

	return entityId;
}
