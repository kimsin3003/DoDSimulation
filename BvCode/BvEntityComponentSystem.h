// Copyright Krafton. Beaver Lab. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "BvTechArtGlobal.h"

DECLARE_LOG_CATEGORY_EXTERN(BvEntityComponentSystem, All, All)

template <typename EntityIdType>
class FBvTechArtBasicRegistry;

class FBvTechArtRegistryUtils
{
public:
	using IntegralType = uint32;

	template <typename EntityIdType>
	static IntegralType ToIntegral(EntityIdType InEntityId)
	{
		return static_cast<IntegralType>(InEntityId);
	}
};

/**
 * @brief	helper method to manipulate template operation 
 * @todo	need to make separate header file to use commonly
 */

template<typename...>
class TBvTechArtTypeList {};

template<typename... Type>
class TBvTechArtExclude : public TBvTechArtTypeList<Type...> {};

/**
 * make static array by tuple elements
 */
template <typename Type, uint32 Num, typename... TupleTypes, uint32... Indices>
void BvTechArtTupleToStaticArray(TStaticArray<Type, Num>& OutStaticArray, const TTuple<TupleTypes...>& InTuple, TIntegerSequence<uint32, Indices...>)
{
	Type TempArray[Num] = { static_cast<Type>(InTuple.Get<Indices>())... };
	for (int32 Index = 0; Index < Num; ++Index)
	{
		OutStaticArray[Index] = TempArray[Index];
	}
}

template <typename Type, typename... TupleTypes>
decltype(auto) BvTechArtToStaticArray(const TTuple<TupleTypes...>& InTuple)
{
	TStaticArray<Type, sizeof...(TupleTypes)> Result;
	BvTechArtTupleToStaticArray(Result, InTuple, TMakeIntegerSequence<uint32, sizeof...(TupleTypes)>());
	return Result;
}


/**
 * @brief	
 * using declaration to be used to _repeat_ the same type a number of times equal to the size of a given parameter pack
 */
template <typename Type, typename>
using TBvTechArtUnpackAsType = Type;

/**
 * @author
 * Hong SangHyeok
 *
 * @brief
 * prototyping ECS for GridOptimizerVolumeProcessor
 * motivated from ENTT lib (ECS github library)
 */
template <typename EntityIdType>
struct FBvTechArtEntityIdPool
{
	enum
	{
		INVALID_INDEX = EntityIdType(-1),
		PAGE_SIZE = 256, // 256 bytes
		ENTITY_PER_PAGE = PAGE_SIZE / sizeof(EntityIdType),
	};

	struct FPage
	{
		EntityIdType* GetData()
		{
			return EntityIds;
		}

		const EntityIdType* GetData() const
		{
			return EntityIds;
		}

		// default constructor
		FPage()
		{
			FPlatformMemory::Memset(&EntityIds[0], 0xFF, sizeof(EntityIds));
		}

		// linear array of entity ids
		EntityIdType EntityIds[ENTITY_PER_PAGE];
	};

	using PageType = TSharedPtr<FPage>;

	auto Page(const EntityIdType InEntityId) const
	{
		return FBvTechArtRegistryUtils::ToIntegral(InEntityId) / ENTITY_PER_PAGE;
	}

	auto Offset(const EntityIdType InEntityId) const
	{
		return FBvTechArtRegistryUtils::ToIntegral(InEntityId) & (ENTITY_PER_PAGE - 1);
	}

	auto Index(const EntityIdType InEntityId) const
	{
		int32 PackedIndex = Sparse[Page(InEntityId)]->GetData()[Offset(InEntityId)];
		return PackedIndex;
	}

	EntityIdType* Assure(const FBvTechArtRegistryUtils::IntegralType InPosition)
	{
		if (!(InPosition < FBvTechArtRegistryUtils::ToIntegral(Sparse.Num())))
		{
			// we need to enlarge the Sparse array
			int32 NumToAdd = (InPosition + 1) - Sparse.Num();
			Sparse.AddZeroed(NumToAdd);
		}
		if (!Sparse[InPosition].IsValid())
		{
			// we are going to use PageData, so allocate it!
			Sparse[InPosition] = MakeShared<FPage>();
		}

		return Sparse[InPosition]->GetData();
	}

	void Emplace(const EntityIdType InEntityId)
	{
		Assure(Page(InEntityId))[Offset(InEntityId)] = FBvTechArtRegistryUtils::ToIntegral(Packed.Num());
		Packed.Add(InEntityId);
	}

	void Erase(const EntityIdType InEntityId)
	{
		const auto PageIndex = Page(InEntityId);
		const auto OffsetIndex = Offset(InEntityId);
		
		checkSlow(Sparse[PageIndex].IsValid());
		EntityIdType* PageElements = Sparse[PageIndex]->GetData();

		Packed[FBvTechArtRegistryUtils::ToIntegral(Sparse[PageIndex]->GetData()[OffsetIndex])] = Packed.Last();
		PageElements[Offset(Packed.Last())] = PageElements[OffsetIndex];
		PageElements[OffsetIndex] = INVALID_INDEX;
		Packed.Pop();
	}

	bool Contains(const EntityIdType InEntityId) const
	{
		const auto Curr = Page(InEntityId);
		return Curr < FBvTechArtRegistryUtils::ToIntegral(Sparse.Num()) 
			&& Sparse[Curr].IsValid()
			&& Sparse[Curr]->GetData()[Offset(InEntityId)] != INVALID_INDEX;
	}

	/**
	 * Packed access manipulator
	 * @brief	looping Packed (EntityIds)
	 */

	int32 Num() const
	{
		return Packed.Num();
	}

	EntityIdType Get(int32 Index)
	{
		return Packed[Index];
	}

	const EntityIdType Get(int32 Index) const
	{
		return Packed[Index];
	}

	/**
	 * @note
	 * * Sparse[Page(Index)][Offset(Index)] has index to access Packed array
	 * * Packed has index to access component in component pool by EntityId
	 */

	TArray<PageType> Sparse;
	TArray<EntityIdType> Packed;
};

template <typename EntityIdType, typename ComponentType>
struct FBvTechArtComponentPool : public FBvTechArtEntityIdPool<EntityIdType>
{
	using Super = FBvTechArtEntityIdPool<EntityIdType>;

	template <typename... Args>
	void Emplace(const EntityIdType InEntityId, Args&&... InArgs)
	{
		Components.Add(ComponentType{ Forward<Args>(InArgs)... });
		Super::Emplace(InEntityId);
	}

	void Erase(const EntityIdType InEntityId)
	{
		auto Other = MoveTemp(Components.Last());
		Components[Super::Index(InEntityId)] = MoveTemp(Other);
		Components.Pop();
		Super::Erase(InEntityId);
	}

	ComponentType& Get(const EntityIdType InEntityId)
	{
		return Components[Super::Index(InEntityId)];
	}

	const ComponentType& Get(const EntityIdType InEntityId) const
	{
		return Components[Super::Index(InEntityId)];
	}

	bool Contains(const EntityIdType InEntityId) const
	{
		return Super::Contains(InEntityId);
	}

	TArray<ComponentType> Components;
};

template <typename EntityIdType, typename ComponentType>
struct FBvTechArtDefaultPool final : FBvTechArtComponentPool<EntityIdType, ComponentType>
{
	using Super = FBvTechArtComponentPool<EntityIdType, ComponentType>;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FDefaultPoolDelegate, FBvTechArtBasicRegistry<EntityIdType>&, EntityIdType);

	template <typename... Args>
	decltype(auto) Emplace(FBvTechArtBasicRegistry<EntityIdType>& Owner, const EntityIdType InEntityId, Args&&... InArgs)
	{
		Super::Emplace(InEntityId, Forward<Args>(InArgs)...);
		return Super::Get(InEntityId);
	}

	void Erase(FBvTechArtBasicRegistry<EntityIdType>& Owner, const EntityIdType InEntityId)
	{
		Super::Erase(InEntityId);
	}

	/**
	 * @brief
	 * update component in the pool
	 *
	 * @tparam Func
	 * signature : void(ComponentType&);
	 */
	template<typename Func>
	decltype(auto) Patch(FBvTechArtBasicRegistry<EntityIdType>& Owner, const EntityIdType InEntityId, Func&& InFunc)
	{
		Forward<Func>(InFunc)(Super::Get(InEntityId));
		return Super::Get(InEntityId);
	}
};

/**
 * @brief
 * component type id generator
 */
struct FBvComponentTypeIdGenerator
{
	static uint32 GenerateTypeId(TCHAR* InTypeName)
	{
		return FCrc::StrCrc32<TCHAR>(InTypeName);
	}
};

#define DECLARE_COMPONENT_TYPE_ID(TypeName) \
	static uint32 GetComponentTypeId() { return FBvComponentTypeIdGenerator::GenerateTypeId(TEXT(#TypeName)); } \

template <typename...>
class FBvTechArtBasicView;

template <typename...>
class FBvTechArtReadOnlyBasicView;

template <typename EntityIdType>
class FBvTechArtBasicRegistry
{
public:
	template <typename ComponentType>
	using FComponentPoolType = FBvTechArtDefaultPool<EntityIdType, ComponentType>;

	struct FComponentPoolData
	{
		uint32 TypeId;
		TUniquePtr<FBvTechArtEntityIdPool<EntityIdType>> Pool{};
	};	

	EntityIdType Create()
	{
		// @todo - need to recycle entity-id
		return GenerateEntityId();
	}

	void Delete(const EntityIdType InEntityId)
	{
		for (auto& PoolData : ComponentPools)
		{
			TUniquePtr<FBvTechArtEntityIdPool<EntityIdType>>& PoolDataRef = PoolData.Pool;
			if (PoolDataRef.IsValid())
			{
				PoolDataRef->Erase(InEntityId);
			}
		}
	}

	template <typename ComponentType>
	FComponentPoolType<ComponentType>& Assure()
	{
		uint32 ComponentTypeId = ComponentType::GetComponentTypeId();

		FComponentPoolData* ComponentPoolData = ComponentPools.FindByPredicate([ComponentTypeId](const FComponentPoolData& ComponentPoolData) {
			return ComponentPoolData.TypeId == ComponentTypeId;
			});

		if (ComponentPoolData == nullptr)
		{
			ComponentPools.Add(FComponentPoolData{
				ComponentType::GetComponentTypeId(),
				MakeUnique<FComponentPoolType<ComponentType>>()
				});

			ComponentPoolData = &ComponentPools.Last();
		}

		return *(static_cast<FComponentPoolType<ComponentType>*>(ComponentPoolData->Pool.Get()));
	}

	/**
	 * return FComponentPoolType<ComponentType>, but when it is not available, it returns nullptr
	 * @note	
	 * * different from Assure()!
	 * * made it to support MT environment
	 */
	template <typename ComponentType>
	const FComponentPoolType<ComponentType>* GetPool() const
	{
		uint32 ComponentTypeId = ComponentType::GetComponentTypeId();

		const FComponentPoolData* ComponentPoolData = ComponentPools.FindByPredicate([ComponentTypeId](const FComponentPoolData& ComponentPoolData) {
			return ComponentPoolData.TypeId == ComponentTypeId;
			});

		// when there is no component pool generated, return nullptr
		if (!ComponentPoolData)
		{
			return nullptr;
		}

		return static_cast<const FComponentPoolType<ComponentType>*>(ComponentPoolData->Pool.Get());
	}

	template <typename ComponentType, typename... Args>
	decltype(auto) Emplace(const EntityIdType InEntityId, Args&&... InArgs)
	{
		return Assure<ComponentType>().Emplace(*this, InEntityId, Forward<Args>(InArgs)...);
	}

	template <typename... ComponentType>
	bool Has(const EntityIdType InEntityId) const
	{
		return (Assure<ComponentType>().Contains(InEntityId) && ...);
	}

	template <typename... ComponentType>
	bool Any(const EntityIdType InEntityId) const
	{
		return (Assure<ComponentType>().Contains(InEntityId) || ...);
	}

	template <typename... ComponentType>
	decltype(auto) Get(const EntityIdType InEntityId) const
	{
		if constexpr (sizeof...(ComponentType) == 1)
		{
			return (Assure<ComponentType>().Get(InEntityId)...);
		}
		else
		{
			return MakeTuple(Get<ComponentType>(InEntityId)...);
		}
	}

	template <typename ComponentType, typename Func>
	decltype(auto) Patch(const EntityIdType InEntityId, Func&& InFunc)
	{
		return Assure<ComponentType>().Patch(*this, InEntityId, Forward<Func>(InFunc));
	}

	bool Orphan(const EntityIdType InEntityId)
	{
		for (auto& ComponentPool : ComponentPools)
		{
			if (ComponentPool.Pool && ComponentPool.Pool.Contains(InEntityId))
			{
				return true;
			}
		}
		return false;
	}

	/** returns a view for the given components */
	template <typename... Components, typename... Excludes>
	FBvTechArtBasicView<EntityIdType, TBvTechArtExclude<Excludes...>, Components...> View(TBvTechArtExclude<Excludes...> = {})
	{
		return { Assure<Components>()..., Assure<Excludes>()... };
	}

	/** returns a read-only view for the given components */
	template <typename... Components, typename... Excludes>
	FBvTechArtReadOnlyBasicView<EntityIdType, TBvTechArtExclude<Excludes...>, Components...> ReadOnlyView(FBvMemoryControlBlock& SyncObject, TBvTechArtExclude<Excludes...> = {}) const
	{
		return { SyncObject, GetPool<Components>()..., GetPool<Excludes>()... };
	}

	TArray<EntityIdType> EntityIds;
	TArray<FComponentPoolData> ComponentPools;

protected:
	EntityIdType GenerateEntityId()
	{
		return EntityIds.Add(EntityIdType{});
	}
};

template <typename EntityIdType, typename IncludeComponent>
class FBvTechArtReadOnlyBasicView<EntityIdType, TBvTechArtExclude<>, IncludeComponent>
{
public:
	friend class FBvTechArtBasicRegistry<EntityIdType>;

	template <typename ComponentType>
	using FComponentPoolType = const FBvTechArtDefaultPool<EntityIdType, ComponentType>;

	FBvTechArtReadOnlyBasicView(FBvMemoryControlBlock& InMemoryControlBlock)
		: SinglePool{ nullptr }
		, View{ nullptr }
		, MemoryLockObject(InMemoryControlBlock)
		, FrameNumber(GFrameNumber)
	{
		MemoryLockObject.EnterMemoryLock(EMemoryFlags::ReadLock);
	}

	FBvTechArtReadOnlyBasicView(FBvMemoryControlBlock& InMemoryControlBlock, FComponentPoolType<IncludeComponent>* IncludeComponentPool)
		: SinglePool{ IncludeComponentPool }
		, View{ static_cast<const FBvTechArtEntityIdPool<EntityIdType>*>(SinglePool) }
		, MemoryLockObject(InMemoryControlBlock)
		, FrameNumber(GFrameNumber)
	{
		MemoryLockObject.EnterMemoryLock(EMemoryFlags::ReadLock);
	}

	~FBvTechArtReadOnlyBasicView()
	{
		checkf(FrameNumber <= GFrameNumber, TEXT("[INVALID BEHAVIOR] please check whether you try to store ReadOnlyView as member variable!"));
		MemoryLockObject.LeaveMemoryLock();
	}

	/** FuncType signature is [void(EntityIdType, const IncludeComponent&)] */
	template<typename FuncType>
	void Each(FuncType InFunc) const
	{
		//*** call lambda by entity id
		if (IsValidView())
		{
			for (EntityIdType EntityId : View->Packed)
			{
				InFunc(EntityId, (SinglePool->Get(EntityId)));
			}
		}
	}

	/** get component by EntityId */
	const IncludeComponent* GetComponent(const EntityIdType& InEntityId) const
	{
		if (IsValidView() && View->Contains(InEntityId))
		{
			return &SinglePool->Get(InEntityId);
		}
		return nullptr;
	}

	/** should check is valid view */
	bool IsValidView() const
	{
		return View != nullptr;
	}

	/** get components (all) */
	const TArray<IncludeComponent>& GetComponents() const
	{
		checkf(IsValidView(), TEXT("you should check IsValidView() first!"));
		return SinglePool->Components;
	}

	/** get entity pool */
	const TArray<EntityIdType>& GetEntities() const
	{
		checkf(IsValidView(), TEXT("you should check IsValidView() first!"));
		return View->Packed;
	}

protected:
	FComponentPoolType<IncludeComponent>* SinglePool;
	const FBvTechArtEntityIdPool<EntityIdType>* View;

	/** for TechArtDB MT support(required FBvMemoryControlBlock) */
	FBvMemoryLockObject MemoryLockObject;
	/** detecting whether ReadOnlyView stores as member variable */
	uint32 FrameNumber;
};

/**
 * FBvTechArtBasicView for one IncludeComponent
 * @brief	THE MOST EFFICIENT View
 */
template <typename EntityIdType, typename IncludeComponent>
class FBvTechArtBasicView<EntityIdType, TBvTechArtExclude<>, IncludeComponent>
{
public:
	friend class FBvTechArtBasicRegistry<EntityIdType>;

	template <typename ComponentType>
	using FComponentPoolType = FBvTechArtDefaultPool<EntityIdType, ComponentType>;

	FBvTechArtBasicView(FComponentPoolType<IncludeComponent>& IncludeComponentPool)
		: SinglePool{ &IncludeComponentPool }
		, View{ static_cast<const FBvTechArtEntityIdPool<EntityIdType>*>(SinglePool) }
	{}

	/**
	 * FuncType signature is [void(EntityIdType, IncludeComponent&)]
	 */
	template<typename FuncType>
	void Each(FuncType InFunc)
	{
		//*** call lambda by entity id
		{
			for (EntityIdType EntityId : View->Packed)
			{
				InFunc(EntityId, (SinglePool->Get(EntityId)));
			}
		}
	}

protected:
	FComponentPoolType<IncludeComponent>* SinglePool;
	const FBvTechArtEntityIdPool<EntityIdType>* View;
};

/**
 * FBvTechArtBasicView for zero ExcludeComponent
 */
template <typename EntityIdType, typename... IncludeComponent>
class FBvTechArtBasicView<EntityIdType, TBvTechArtExclude<>, IncludeComponent...>
{
public:
	friend class FBvTechArtBasicRegistry<EntityIdType>;

	template <typename ComponentType>
	using FComponentPoolType = FBvTechArtDefaultPool<EntityIdType, ComponentType>;

	FBvTechArtBasicView(FComponentPoolType<IncludeComponent>&... IncludeComponentPool)
		: Pools{ &IncludeComponentPool... }
		, View{ Candidate() }
	{}

	const FBvTechArtEntityIdPool<EntityIdType>* Candidate() const
	{
		auto EntityIdPoolArray = BvTechArtToStaticArray<const FBvTechArtEntityIdPool<EntityIdType>*>(Pools);
		ensure(EntityIdPoolArray.Num() > 0);

		const FBvTechArtEntityIdPool<EntityIdType>* FoundEntitiyIdPool = EntityIdPoolArray[0];
		for (int32 Index = 1; Index < EntityIdPoolArray.Num(); ++Index)
		{
			const FBvTechArtEntityIdPool<EntityIdType>* EntityIdPool = EntityIdPoolArray[Index];
			if (FoundEntitiyIdPool->Num() > EntityIdPool->Num())
			{
				FoundEntitiyIdPool = EntityIdPool;
			}
		}

		return FoundEntitiyIdPool;
	}

	template <typename FuncType, uint32... Indices>
	decltype(auto) InvokeFuncByComponents(EntityIdType InEntityId, FuncType&& InFunc, TIntegerSequence<uint32, Indices...>)
	{
		return InFunc(InEntityId, (Pools.Get<Indices>()->Get(InEntityId))...);
	}

	/**
	 * FuncType signature is [void(EntityIdType, IncludeComponent&)]
	 */
	template<typename FuncType>
	void Each(FuncType InFunc)
	{
		// get the minimum spanning entity id pool from the view
		const FBvTechArtEntityIdPool<EntityIdType>* EntityIdPool = View;

		//*** filter entity ids to call lambda function
		TArray<EntityIdType> FilteredEntityIds;
		{
			for (int32 EntityIdIndex = 0; EntityIdIndex < EntityIdPool->Num(); ++EntityIdIndex)
			{
				EntityIdType EntityId = (*EntityIdPool).Get(EntityIdIndex);

				// checking entity has all include components
				bool IsIncluded = true;
				auto IncludeEntityIdPools = BvTechArtToStaticArray<const FBvTechArtEntityIdPool<EntityIdType>*>(Pools);
				for (int32 IncludeIndex = 0; IncludeIndex < IncludeEntityIdPools.Num(); ++IncludeIndex)
				{
					const FBvTechArtEntityIdPool<EntityIdType>* IncludeEntityIdPool = IncludeEntityIdPools[IncludeIndex];
					if (!IncludeEntityIdPool->Contains(EntityId))
					{
						IsIncluded = false;
						break;
					}
				}

				// failed to include all required components
				if (IsIncluded)
				{
					FilteredEntityIds.Add(EntityId);
				}
			}
		}

		//*** call lambda by entity id
		{
			for (EntityIdType EntityId : FilteredEntityIds)
			{
				InvokeFuncByComponents(EntityId, Forward<FuncType>(InFunc), TMakeIntegerSequence<uint32, sizeof...(IncludeComponent)>());
			}
		}
	}

protected:
	const TTuple<FComponentPoolType<IncludeComponent>*...> Pools;
	const FBvTechArtEntityIdPool<EntityIdType>* View;
};

/**
 * FBvTechArtBasicView for multiple ExcludeComponent (more than one)
 */

template <typename EntityIdType, typename... ExcludeComponent, typename... IncludeComponent>
class FBvTechArtBasicView<EntityIdType, TBvTechArtExclude<ExcludeComponent...>, IncludeComponent...>
{
public:
	friend class FBvTechArtBasicRegistry<EntityIdType>;

	template <typename ComponentType>
	using FComponentPoolType = FBvTechArtDefaultPool<EntityIdType, ComponentType>;

	using FFilterType = TStaticArray<FBvTechArtEntityIdPool<EntityIdType>*, sizeof...(ExcludeComponent)>;

	FBvTechArtBasicView(FComponentPoolType<IncludeComponent>&... IncludeComponentPool, TBvTechArtUnpackAsType<typename FBvTechArtEntityIdPool<EntityIdType>, ExcludeComponent>&... ExcludePool)
		: Pools{&IncludeComponentPool...}
		, View{ Candidate() }
		, Filters{&ExcludePool...}
	{}

	/** get the minimum EntitiyIdPool from Pools */
	const FBvTechArtEntityIdPool<EntityIdType>* Candidate() const
	{
		auto EntityIdPoolArray = BvTechArtToStaticArray<const FBvTechArtEntityIdPool<EntityIdType>*>(Pools);
		ensure(EntityIdPoolArray.Num() > 0);

		const FBvTechArtEntityIdPool<EntityIdType>* FoundEntitiyIdPool = EntityIdPoolArray[0];
		for (int32 Index = 1; Index < EntityIdPoolArray.Num(); ++Index)
		{
			const FBvTechArtEntityIdPool<EntityIdType>* EntityIdPool = EntityIdPoolArray[Index];
			if (FoundEntitiyIdPool->Num() > EntityIdPool->Num())
			{
				FoundEntitiyIdPool = EntityIdPool;
			}
		}

		return FoundEntitiyIdPool;
	}

	template <typename FuncType, uint32... Indices>
	decltype(auto) InvokeFuncByComponents(EntityIdType InEntityId, FuncType&& InFunc, TIntegerSequence<uint32, Indices...>)
	{
		return InFunc(InEntityId, (Pools.Get<Indices>()->Get(InEntityId))...);
	}

	/**
	 * FuncType signature is [void(EntityIdType, IncludeComponent&)]
	 */
	template<typename FuncType>
	void Each(FuncType InFunc)
	{
		// get the minimum spanning entity id pool from the view
		const FBvTechArtEntityIdPool<EntityIdType>* EntityIdPool = View;

		//*** filter entity ids to call lambda function
		TArray<EntityIdType> FilteredEntityIds;
		{
			for (int32 EntityIdIndex = 0; EntityIdIndex < EntityIdPool->Num(); ++EntityIdIndex)
			{
				EntityIdType EntityId = (*EntityIdPool).Get(EntityIdIndex);

				// checking entity has all include components
				bool IsIncluded = true;
				auto IncludeEntityIdPools = BvTechArtToStaticArray<const FBvTechArtEntityIdPool<EntityIdType>*>(Pools);
				for (int32 IncludeIndex = 0; IncludeIndex < IncludeEntityIdPools.Num(); ++IncludeIndex)
				{
					const FBvTechArtEntityIdPool<EntityIdType>* IncludeEntityIdPool = IncludeEntityIdPools[IncludeIndex];
					if (!IncludeEntityIdPool->Contains(EntityId))
					{
						IsIncluded = false;
						break;
					}
				}

				// failed to include all required components
				if (!IsIncluded)
				{
					continue;
				}

				// checking entity has exclude component
				bool IsExcluded = false;
				for (int32 ExcludeIndex = 0; ExcludeIndex < Filters.Num(); ++ExcludeIndex)
				{
					const FBvTechArtEntityIdPool<EntityIdType>* ExcludeEntityIdPool = Filters[ExcludeIndex];
					if (ExcludeEntityIdPool->Contains(EntityId))
					{
						IsExcluded = true;
						break;
					}
				}

				// success to find matched entity id
				if (IsIncluded && !IsExcluded)
				{
					FilteredEntityIds.Add(EntityId);
				}
			}
		}
		
		//*** call lambda by entity id
		{
			for (EntityIdType EntityId : FilteredEntityIds)
			{
				InvokeFuncByComponents(EntityId, Forward<FuncType>(InFunc), TMakeIntegerSequence<uint32, sizeof...(IncludeComponent)>());
			}			
		}
	}

protected:
	const TTuple<FComponentPoolType<IncludeComponent>*...> Pools;
	const FBvTechArtEntityIdPool<EntityIdType>* View;
	FFilterType Filters;
};

/**
 * FBvTechArtSystemContext
 * @brief	system's context to transfer the data processed in previous system
 */
struct BVTECHARTCORE_API FBvTechArtSystemContext
{

};

/**
 * FBvTechSystemExecutor
 * @brief System ticker
 */
class BVTECHARTCORE_API FBvTechArtSystemExecutor
{
public:
	FBvTechArtSystemExecutor();
	virtual ~FBvTechArtSystemExecutor() {}

	/** pure virtual methods */
	virtual void Execute(FBvTechArtSystemContext& InContext) {}
	virtual TCHAR* GetSystemName() const { return TEXT(""); }
};


class BVTECHARTCORE_API FBvTechArtSystemTicker : public FTickableGameObject
{
public:
	DECLARE_DELEGATE_OneParam(FBvExecuteSystemsByGraph, FBvTechArtSystemTicker&);

	static FBvTechArtSystemTicker& Get()
	{
		static FBvTechArtSystemTicker Instance;
		return Instance;
	}

	/** FTickableGameObject methods */
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FBvTechArtSystemTicker, STATGROUP_Tickables); }

	/** get tick number */
	uint32 GetTickNumber() const { return TickNumber; }

	/** Tick */
	virtual void Tick(float DeltaTime) override
	{
		check(IsInGameThread());

		if (!FBvTechArtGlobal::IsEnabledTechArtECS())
		{
			return;
		}

		if (ExecuteSystemsByGraph.IsBound())
		{
			// execute systems by programmer-defined way (like task graph)
			ExecuteSystemsByGraph.Execute(*this);
		}
		else
		{
			FBvTechArtSystemContext DefaultContext;

			// if there is no ticker graph, just execute systems linearly
			for (auto& Executor : SystemExecutors)
			{
				Executor->Execute(DefaultContext);
			}
		}

		// update tick number
		TickNumber++;
	}

protected:
	/** In TechArtGameFramwork module */
	friend class FBvTechArtSystemExecutorGraph;
	friend class FBvTechArtSystemExecutor;

	FBvTechArtSystemTicker()
		: TickNumber(0)
	{}

	/** ONLY allowed to friend class to access SystemExecutors */
	void AddSystem(FBvTechArtSystemExecutor* InSystemExecutor)
	{
		SystemExecutors.Add(InSystemExecutor);
	}
	
	/** system executors properties */
	TArray<FBvTechArtSystemExecutor*> SystemExecutors;	

	/** execute system by graph to override */
	FBvExecuteSystemsByGraph ExecuteSystemsByGraph;

	/** tick number */
	uint32 TickNumber;
};

/**
 * FBvTechArtSystemInterface
 * @brief Entity-Component's system interface
 */
template <typename SystemType>
class FBvTechArtSystemInterface : public FBvTechArtSystemExecutor
{
public:
	virtual ~FBvTechArtSystemInterface() {}
	static SystemType& Get()
	{
		static SystemType Instance;
		return Instance;
	}

protected:
};

/**
 * ECS Common Usage for BvTechArt
 */
using FBvTechArtEntityId = uint32;
using FBvTechECSRegistry = FBvTechArtBasicRegistry<FBvTechArtEntityId>;

template <typename ComponentType>
using TBvTechArtComponentPool = FBvTechArtComponentPool<FBvTechArtEntityId, ComponentType>;

template<typename Type, int32 NumElement = 64>
using TBvInlineArray = TArray<Type, TInlineAllocator<NumElement, TMemStackAllocator<>>>;

class FBvEntityComponentSystem
{
public:	
};
