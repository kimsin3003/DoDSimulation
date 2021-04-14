#pragma once
#include <array>
#include <string>
#include <functional>
#include <typeinfo>
#include "Database.h"
#include "Templates/Tuple.h"
#include "Templates/RemoveReference.h"
#include "Templates/RemoveCV.h"

template<typename... CompType>
class Query
{
public:
	template<typename PoolCompType>
	using CompPoolType = ComponentPool<typename TRemoveReference<PoolCompType>::Type>*;

	template <typename FuncType>
	void Each(const Database& Database, FuncType Func)
	{
		TArray<ArcheType*> ArcheTypes = Database.GetArcheTypes();
		for (ArcheType* ArcheType : ArcheTypes)
		{
			const TArray<int64_t> EntityOrder = ArcheType->GetEntityList();
			auto Pools = MakeTuple<CompPoolType<CompType> ...>( ArcheType->GetPool<typename TRemoveReference<CompType>::Type>()... );

			for (int64_t Index = 0; Index < EntityOrder.Num(); Index++)
			{
				int32 EntityId = EntityOrder[Index];
				Func(EntityId, Pools.Get<CompPoolType<CompType>>()->Comps[Index] ...);
			}
		}
	}
};
