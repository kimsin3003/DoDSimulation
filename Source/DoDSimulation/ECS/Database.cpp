#include "Database.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void ArcheType::MoveToSmaller(ArcheType* To, int32 EntityId)
{
	int32 IndexToMove = EnitityIdToIndex[EntityId];
	RemoveEntity(IndexToMove, EntityId);
	To->AddEntity(EntityId);

	for (auto& Pair : TypeIdToPool)
	{
		FString TypeId = Pair.Key;
		PoolData* From = Pair.Value;
		From->MoveTo(IndexToMove, To->TypeIdToPool[TypeId]);
	}
}

void ArcheType::AddEntity(int32 EntityId)
{
	int32 NewIndex = EntityIds.Add(EntityId);
	EnitityIdToIndex.Add(EntityId, NewIndex);
}

void ArcheType::RemoveEntity(int32 IndexToMove, int32 EntityId)
{
	//move last entity to removed entity's index
	int32 LastEntityId = EntityIds[EntityIds.Num() - 1];
	EntityIds[IndexToMove] = LastEntityId;
	EnitityIdToIndex[LastEntityId] = IndexToMove;
	EntityIds.RemoveAt(EntityIds.Num() - 1);

	//remove removing entity's index data
	EnitityIdToIndex.Remove(EntityId);
}

ArcheType::~ArcheType()
{
	for (auto& Pair : TypeIdToPool)
	{
		delete Pair.Value;
	}
}