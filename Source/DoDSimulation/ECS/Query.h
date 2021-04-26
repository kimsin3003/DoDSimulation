#pragma once
#include <array>
#include <string>
#include <functional>
#include <typeinfo>
#include "Database.h"
#include "Templates/Tuple.h"
#include "Templates/RemoveReference.h"
#include "Templates/RemoveCV.h"

template <typename...>
class Query;

template <typename... CompType>
class Query
{
public:
	template<typename PoolCompType>
	using CompPoolType = ComponentPool<PoolCompType>*;

	template <typename FuncType>
	void Each(const Database& Database, FuncType Func)
	{
		TArray<ArcheType*> ArcheTypes = Database.GetArcheTypes<CompType...>();
		for (ArcheType* ArcheType : ArcheTypes)
		{
			const TArray<int32> EntityOrder = ArcheType->GetEntityList();
			auto Pools = MakeTuple<CompPoolType<CompType> ...>( ArcheType->GetPool<CompType>()... );

			for (int32 Index = 0; Index < EntityOrder.Num(); Index++)
			{
				int32 EntityId = EntityOrder[Index];
				Func(EntityId, Pools.Get<CompPoolType<CompType>>()->Comps[Index] ...);
			}
		}
	}
};

template <typename... CompType, typename... ExcludeType>
class Query<TExclude<ExcludeType...>, CompType...>
{
public:
	template<typename PoolCompType>
	using CompPoolType = ComponentPool<PoolCompType>*;

	template <typename FuncType>
	void Each(const Database& Database, FuncType Func)
	{
		TArray<ArcheType*> ArcheTypes = Database.GetArcheTypes<CompType...>(TExclude<ExcludeType...>());
		for (ArcheType* ArcheType : ArcheTypes)
		{
			TArray<int32> EntityOrder = ArcheType->GetEntityList();

			for (int32 Index = 0; Index < EntityOrder.Num(); Index++)
			{
				int32 EntityId = EntityOrder[Index];
				if (Func(EntityId, *Database.GetComp<CompType>(EntityId) ...) == false)
					return;
			}
		}
	}
};
