
#pragma once
#include <stdint.h>
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Containers/Set.h"
#include "UObject/Object.h"
#include "Component.h"


class UWorld;
class AActor;

struct IComponentPool
{
};

template<typename CompType>
struct ComponentPool : public IComponentPool
{
	TArray<CompType> Comps;
};

struct PoolData
{
	FString TypeId;
	IComponentPool* Pool;
};

class ArcheType
{
public:
	template<typename... CompType>
	void Create()
	{
		PoolDatas.Add(new ComponentPool<CompType>())...;
		Keys = { typeid(CompType).name()... };
	}



	const TArray<int64_t>& GetEntityList() { return EntityIds; }

	template<typename CompType>
	ComponentPool<CompType>* GetPool() const
	{
		FString TypeToFind = typeid(CompType).name();
		check(Keys.Contains(TypeToFind));
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

	TSet<FString> Keys;
private:
	TArray<PoolData*> PoolDatas;
	TArray<int64_t> EntityIds;
};

class Database
{
public:
	Database(UWorld* world);
	int AddActor(AActor* actor);

	template<typename... CompType>
	TArray<ArcheType*> GetArcheTypes() const
	{
		TArray<ArcheType*> MatchingArcheTypes;
		TSet<FString> KeyToFind = { typeid(CompType).name()... };
		for (auto ArcheType : ArcheTypes)
		{
			if (ArcheType->Keys.Includes(KeyToFind))
				MatchingArcheTypes.Add(ArcheType);
		}
		return MatchingArcheTypes;
	}

private:
	UWorld* _world = nullptr;
	int64_t _nextEntity = 0;
	TArray<ArcheType*> ArcheTypes;
};