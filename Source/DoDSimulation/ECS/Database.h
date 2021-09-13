
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
		check(Comps.Num()-1 >= Index);
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
		auto Pool = static_cast<ComponentPool<CompType>*>(TypeIdToPool[TypeToFind]->Pool);
		if (EntityIds.Num() != Pool->Comps.Num())
			printf("asdf");
		return Pool;
	}

	template<typename CompType>
	CompType* GetComp(int32 EntityId) const
	{
		int32 Index = EnitityIdToIndex[EntityId];
		ComponentPool<CompType>* Pool = GetPool<CompType>();
		return &(Pool->Comps[Index]);
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
			New->TypeIdToPool.Add(TypeId, NewData);
		}
		FString NewCompTypeId = TypeToTypeId<NewCompType>();
		New->TypeIdToPool.Add(NewCompTypeId, PoolData::Create<NewCompType>());
		TSet<FString> NewKey = { NewCompTypeId };
		NewKey.Append(Original->TypeIds);
		New->TypeIds = NewKey;
		return New;
	}

	template<typename NewCompType>
	void AddComp(int32 EntityId, NewCompType&& Comp)
	{
		if (!EntityIds.Contains(EntityId))
			AddEntity(EntityId);
		auto& Comps = GetPool<NewCompType>()->Comps;
		check(Comps.Num() == EnitityIdToIndex[EntityId]);
		Comps.Push(MoveTemp(Comp));
	}
	void MoveToSmaller(ArcheType* To, int32 EntityId);

	template<typename CompType>
	void MoveToBigger(ArcheType* To, int32 EntityId, CompType&& Comp)
	{
		MoveToSmaller(To, EntityId);
		To->AddComp(EntityId, MoveTemp(Comp));
		if (To->GetPool<CompType>()->Comps.Num() != To->EntityIds.Num())
			printf("asdf");
	}

	~ArcheType();
private:

	void AddEntity(int32 EntityId);
	void RemoveEntity(int32 IndexToMove, int32 EntityId);

public:
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
		if (EntityToArcheType.Contains(EntityId))
		{
			ArcheType* Container = EntityToArcheType[EntityId];
			TSet<FString> BiggerKey = { TypeId };
			BiggerKey.Append(Container->TypeIds);
			ArcheType* BiggerContainer = GetExactArcheType(BiggerKey);
			if (BiggerContainer == nullptr)
			{
				BiggerContainer = ArcheType::CreateExtension<T>(Container);
				ArcheTypes.Add(BiggerContainer);
			}
			Container->MoveToBigger<T>(BiggerContainer, EntityId, MoveTemp(Comp));
			EntityToArcheType[EntityId] = BiggerContainer;
		}
		else
		{
			ArcheType* Container = GetExactArcheType(TSet<FString>{ TypeId });
			if (Container == nullptr)
			{
				Container = ArcheType::Create<T>();
				ArcheTypes.Add(Container);
			}
			Container->AddComp(EntityId, MoveTemp(Comp));
			EntityToArcheType.Add(EntityId, Container);
		}
	}

	template<typename CompType>
	CompType* GetComp(int32 EntityId) const
	{
		check(EntityToArcheType.Contains(EntityId));
		return EntityToArcheType[EntityId]->GetComp<CompType>(EntityId);
	}
	

	ArcheType* GetExactArcheType(TSet<FString> Key)
	{
		for (auto ArcheType : ArcheTypes)
		{
			if (ArcheType->TypeIds.Difference(Key).Num() == 0
				&& Key.Difference(ArcheType->TypeIds).Num() == 0)
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
			if (ArcheType->TypeIds.Includes(KeySetToInclude) && ArcheType->TypeIds.Intersect(KeySetToExclude).Num() == 0)
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