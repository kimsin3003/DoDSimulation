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
		auto NewArcheType = new ArcheType();
//		NewArcheType->Create();
		ArcheTypes.Add(new ArcheType());
	}

	return entityId;
}
