
#pragma once
#include <stdint.h>
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Templates/Tuple.h"
#include "UObject/Object.h"
#include "Component.h"


class UWorld;
class AActor;



class EntityPool
{
	TArray<int64_t> EntityId;
};

template<typename CompType>
class ComponentPool : public EntityPool
{
	TArray<CompType> Comps;
};

class PoolData
{
	FString TypeId;
	EntityPool* Pool;
};

struct ArcheType
{
	TSet<FString> Key;

	template<typename... CompType>
	void Create()
	{
		PoolDatas.Add(new ComponentPool<CompType>())...;
		Key = { typeid(CompType).name()... };
	}

	template<typename CompType>
	const ComponentPool<CompType>* GetPool() const
	{
		FString TypeToFind = typeid(CompType).name();
		assert(Key.Contains(TypeToFind));
		for (auto Data : PoolDatas)
		{
			if (Data->TypeId == TypeToFind)
				return static_cast<ComponentPool<CompType>*>(Data->Pool);
		}
		return nullptr;
	}

	~ArcheType()
	{
		for (auto Data : PoolDatas)
		{
			delete Data;
		}
	}


	TArray<PoolData*> PoolDatas;
};

class Database
{
public:
	Database(UWorld* world);
	int AddActor(AActor* actor);

	template<typename... CompType>
	TArray<ArcheType*> GetPool()
	{
		TArray<ArcheType*> MatchingArcheTypes;
		TSet KeyToFind = { typeid(CompType).name()... };
		for (auto& ArcheType : ArcheTypes)
		{
			if (ArcheType.Key.Contains(KeyToFind))
				MatchingArcheTypes.Add(&ArcheType);
		}
		return MatchingArcheTypes;
	}

private:
	UWorld* _world = nullptr;
	int64_t _nextEntity = 0;
	TArray<ArcheType*> ArcheTypes;
};