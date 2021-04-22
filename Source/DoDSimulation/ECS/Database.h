
#pragma once
#include <stdint.h>
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Containers/Set.h"
#include "UObject/Object.h"
#include "Component.h"
#include "Templates/RemoveReference.h"


class UWorld;
class AActor;

template<typename T>
FString TypeToTypeId() { return typeid(T).name(); }

template<typename...>
class TTypeList {};

template<typename... Type>
class TExclude : public TTypeList<Type...> {};

struct IComponentPool
{
	virtual IComponentPool* CreateEmpty() = 0;
	virtual void MoveTo(int32 Index, IComponentPool* To) = 0;
	virtual ~IComponentPool() {}
};

template<typename CompType>
struct ComponentPool : public IComponentPool
{
	TArray<CompType> Comps;
	IComponentPool* CreateEmpty() final
	{
		return new ComponentPool<CompType>();
	}

	void MoveTo(int32 Index, IComponentPool* To) final
	{
		ComponentPool<CompType>* RealTo = static_cast<ComponentPool<CompType>*>(To);
		RealTo->Comps.Add(MoveTemp(Comps[Index]));
		Comps[Index] = Comps[Comps.Num() - 1];
		Comps.RemoveAt(Comps.Num() - 1);
	}
	~ComponentPool() final {}
};

struct PoolData
{
	FString TypeId;
	IComponentPool* Pool;

	template<typename CompType>
	static PoolData* Create()
	{
		PoolData* New = new PoolData();
		New->TypeId = TypeToTypeId<CompType>();
		New->Pool = new ComponentPool<CompType>();
		return New;
	}

	PoolData* CreateEmptyClone()
	{
		return new PoolData
		{
			TypeId,
			Pool->CreateEmpty()
		};
	};

	void MoveTo(int32 Index, PoolData* To)
	{
		Pool->MoveTo(Index, To->Pool);
	}
};

class ArcheType
{
public:

	const TArray<int32>& GetEntityList() { return EntityIds; }

	const bool IsEmpty() { return EntityIds.Num() == 0; }

	template<typename CompType>
	ComponentPool<CompType>* GetPool() const
	{
		FString TypeToFind = TypeToTypeId<CompType>();
		check(TypeIdToPool.Contains(TypeToFind));
		return static_cast<ComponentPool<CompType>*>(TypeIdToPool[TypeToFind]->Pool);
	}

	template<typename CompType>
	CompType& GetComp(int32 EntityId)
	{
		int32 Index = EnitityIdToIndex[EntityId];
		return GetPool<CompType>()->Comps[Index];
	}

	template<typename... CompType>
	static ArcheType* Create()
	{
		ArcheType* New = new ArcheType();
		New->TypeIdToPool = { {TypeToTypeId<CompType>(), (PoolData::Create<CompType>())}... };
		New->TypeIds = { TypeToTypeId<CompType>()... };
		return New;
	}

	template<typename NewCompType>
	static ArcheType* CreateExtension(ArcheType* Original)
	{
		ArcheType* New = new ArcheType();
		for (const auto& Pair : Original->TypeIdToPool)
		{
			FString TypeId = Pair.Key;
			PoolData* Data = Original->TypeIdToPool[TypeId];
			PoolData* NewData = Data->CreateEmptyClone();
			New->TypeIdToPool[TypeId] = NewData;
		}
		FString NewCompTypeId = TypeToTypeId<NewCompType>();
		New->TypeIdToPool[NewCompTypeId] = PoolData::Create<NewCompType>();
		TSet<FString> NewKey = { NewCompTypeId };
		NewKey.Append(Original->TypeIds);
		New->TypeIds = NewKey;
		return New;
	}

	template<typename NewCompType>
	void Add(int32 EntityId, NewCompType&& Comp)
	{
		int32 Index = EntityIds.Add(EntityId);
		EnitityIdToIndex.Add(EntityId, Index);
		GetPool<NewCompType>()->Comps.Push(MoveTemp(Comp));
	}

	void MoveToSmaller(ArcheType* To, int32 EntityId)
	{
		int32 IndexToMove = EnitityIdToIndex[EntityId];
		EntityIds[IndexToMove] = EntityIds[EntityIds.Num() - 1];
		EntityIds.RemoveAt(EntityIds.Num() - 1);
		EnitityIdToIndex.Remove(EntityId);

		To->EntityIds.Add(EntityId);
		for (auto& Pair : TypeIdToPool)
		{
			FString TypeId = Pair.Key;
			PoolData* From = Pair.Value;
			From->MoveTo(IndexToMove, To->TypeIdToPool[TypeId]);
		}
	}

	template<typename CompType>
	void MoveToBigger(ArcheType* To, int32 EntityId, CompType&& Comp)
	{
		MoveToSmaller(To, EntityId);
		To->GetPool<CompType>()->Comps.Push(MoveTemp(Comp));
	}

	~ArcheType()
	{
		for (auto& Pair : TypeIdToPool)
		{
			delete Pair.Value;
		}
	}
	TSet<FString> TypeIds;
private:
	TArray<int32> EntityIds;
	TMap<int32, int32> EnitityIdToIndex;
	TMap<FString, PoolData*> TypeIdToPool;
};

class Database
{
public:
	int32 CreateEntity() { return NextEntity++; }

	template<typename CompType>
	void AddComp(int32 EntityId, CompType&& Comp)
	{
		typedef typename TRemoveReference<CompType>::Type T;
		FString TypeId = TypeToTypeId<T>();
		ArcheType* Container = nullptr;
		if (EntityToArcheType.Contains(EntityId))
		{
			Container = EntityToArcheType[EntityId];
			TSet<FString> BiggerKey = { TypeId };
			BiggerKey.Append(Container->TypeIds);
			ArcheType* BiggerContainer = GetExactArcheType(BiggerKey);
			if (BiggerContainer == nullptr)
				BiggerContainer = ArcheType::CreateExtension<T>(Container);
			Container->MoveToBigger<T>(BiggerContainer, EntityId, MoveTemp(Comp));
			EntityToArcheType[EntityId] = BiggerContainer;

			ArcheTypes.Add(BiggerContainer);
			if (Container->IsEmpty())
			{
				ArcheTypes.Remove(Container);
				delete Container;
			}
		}
		else
		{
			Container = ArcheType::Create<T>();
			Container->Add(EntityId, MoveTemp(Comp));
			EntityToArcheType.Add(EntityId, Container);
		}
	}

	template<typename CompType>
	CompType& GetComp(int32 EnitityId)
	{
		check(EntityToArcheType.Contains(EntityId));
		EntityToArcheType[EntityId]->GetComp(EntityId);
	}
	

	ArcheType* GetExactArcheType(TSet<FString> Key)
	{
		for (auto ArcheType : ArcheTypes)
		{
			if (ArcheType->TypeIds.Difference(Key).Num() == 0)
				return ArcheType;
		}
		return nullptr;
	}
	
	template <typename... CompType, typename... ExcludeType>
	TArray<ArcheType*> GetArcheTypes(const TExclude<ExcludeType...>& Exclude = {}) const
	{
		TArray<ArcheType*> MatchingArcheTypes;
		TSet<FString> KeySetToInclude = { TypeToTypeId<CompType>()... };
		TSet<FString> KeySetToExclude = { TypeToTypeId<ExcludeType>()... };
		for (auto ArcheType : ArcheTypes)
		{
			if (ArcheType->TypeIds.Includes(KeySetToInclude) && ArcheType->TypeIds.Intersect(KeySetToInclude).Num() == 0)
				MatchingArcheTypes.Add(ArcheType);
		}
		return MatchingArcheTypes;
	}

	template <typename... CompType>
	TArray<ArcheType*> GetArcheTypes() const
	{
		TArray<ArcheType*> MatchingArcheTypes;
		TSet<FString> KeySetToInclude = { TypeToTypeId<CompType>()... };
		for (auto ArcheType : ArcheTypes)
		{
			if (ArcheType->TypeIds.Includes(KeySetToInclude))
				MatchingArcheTypes.Add(ArcheType);
		}
		return MatchingArcheTypes;
	}

	~Database()
	{
		for (auto ArcheType : ArcheTypes)
		{
			delete ArcheType;
		}
	}

private:
	int32 NextEntity = 0;
	TArray<ArcheType*> ArcheTypes;
	TMap<int32, ArcheType*> EntityToArcheType;
};