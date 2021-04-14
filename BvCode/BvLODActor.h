// Copyright Krafton. Beaver Lab. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/LODActor.h"
#include "Math/BoxSphereBounds.h"
#include "BvMeshMergeUtils.h"
#include "BvTechArtDB.h"
#include "BvTechArtGameObjectSystem.h"

#include "BvLODActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(BvLODActor, All, All)

/**
 * FBvLODActorEntity
 * @brief	EntityType for ABvLODActor
 */
struct FBvLODActorEntity
{	
	BV_DECLARE_ENTITY_TYPE_ID(FBvLODActorEntity);
};

/**
 * Components for FBvLODActorEntity
 */
namespace BvLODActorEntity
{
	struct FBvSMCs
	{
		DECLARE_COMPONENT_TYPE_ID(FBvSMCs);

		/** BvLODActor's SMC entity type ids (FBvSMCEntityInBvLODActorEntity) */
		TArray<FBvTechArtEntityId> SMCIds;
	};
}

/**
 * FBvLODActorDB
 * @brief	FBvLODActor's ECS
 */
class BVARTASSETPROCESSRUNTIME_API FBvLODActorDB : public FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>
{
public:
	using Super = FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>;

	FBvLODActorDB()
		: Super()
	{
		// register OnAddEntityEvent
		AddEntityEventDelegate(&FBvLODActorDB::OnAddEntity);
		RemoveEntityEventDelegate(&FBvLODActorDB::OnRemoveEntity);

		// register Components
		{
			RegisterSchema<BvLODActorEntity::FBvSMCs>();
		}
	}

	/** events for TechArtDB */
	static void OnAddEntity(FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId);
	static void OnRemoveEntity(FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId);
};

/**
 * FBvSMCEntityInBvLODActorEntity
 * @brief	SMC entity which came from BvLODActor's SubActors
 */
struct FBvSMCEntityInBvLODActorEntity
{
	BV_DECLARE_ENTITY_TYPE_ID(FBvSMCEntityInBvLODActorEntity);
};

#define BV_TECHART_DEBUG_SMC_ENTITY (!(UE_BUILD_SHIPPING || UE_BUILD_TEST))

namespace BvSMCEntityInBvLODActorEntity
{
	/**
	 * FBvSMCRef
	 * @brief	1:1 relationship between BvSMCEntity and UStaticMeshComponent
	 */
	struct FBvSMCRef
	{
		DECLARE_COMPONENT_TYPE_ID(FBvSMCRef);

		/** indirect reference to SMC which located in BvLODActor's SubActors */
		TWeakObjectPtr<class UStaticMeshComponent> SMC;
	};

	struct FBvBounds
	{
		DECLARE_COMPONENT_TYPE_ID(FBvBounds);

		FBvBounds()
			: Min(ForceInit)
			, Max(ForceInit)
		{}

		FBvBounds(const FVector& InMin, const FVector& InMax)
			: Min(InMin), Max(InMax)
		{}

		FVector Min;
		FVector Max;
	};

	struct FBvState
	{
		DECLARE_COMPONENT_TYPE_ID(FBvState);

		FBvState()
		{
			AliasedData.State = 0;
		}

		struct FData
		{
			/** whether current SMC is visible in world */
			uint8 IsVisible		: 1;

			/** current SMC is culled */
			uint8 IsCulled		: 1;

			/** current SMC is previously culled */
			uint8 WasCulled		: 1;

			/** add more state flags in here (but do not exceed 8!) */
		};

		union FUnionData
		{
			// aliasing type
			FData Data;

			// bit flag for Data
			uint8 State;
		};

		FUnionData AliasedData;
	};

	struct FBvTransform
	{
		DECLARE_COMPONENT_TYPE_ID(FBvTransform);

		FTransform WorldTransform;
	};

	struct FBvMeshInstancingKey
	{
		DECLARE_COMPONENT_TYPE_ID(FBvMeshInstancingKey);

		/** mesh instancing key's entity id in ECS */
		FBvTechArtEntityId MeshInstancingKeyId;
	};

	struct FBvDebugEntity
	{
		DECLARE_COMPONENT_TYPE_ID(FBvDebugEntity);

		FBvTechArtEntityId DebugEntityId;
	};
}

class BVARTASSETPROCESSRUNTIME_API FBvSMCInBvLODActorDB : public FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>
{
public:
	using Super = FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>;

	FBvSMCInBvLODActorDB()
		: Super()
	{
		// register OnAddEntityEvent
		AddEntityEventDelegate(&FBvSMCInBvLODActorDB::OnAddEntity);
		RemoveEntityEventDelegate(&FBvSMCInBvLODActorDB::OnRemoveEntity);

		// register Components
		{
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvSMCRef>();
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvBounds>();
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvState>();
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvTransform>();
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvDebugEntity>();
			RegisterSchema<BvSMCEntityInBvLODActorEntity::FBvMeshInstancingKey>();
		}
	}

	virtual void Dispatch() override
	{		
		FBvScopeMemoryLock Lock(SyncObject, EMemoryFlags::WriteLock);

		// clear pending entity to add mesh-instancing keys
		PendingEntitiesToAddMeshInstancingKeys.Reset();

		// dispatch DB
		Super::Dispatch();
	}

	/** get pending entities to add mesh instancing keys */
	const TArray<FBvTechArtEntityId>& GetPendingEntitiesToAddMeshInstancingKeys()
	{
		FBvScopeMemoryLock Lock(SyncObject, EMemoryFlags::ReadLock);
		return PendingEntitiesToAddMeshInstancingKeys;
	}

	/** events for TechArtDB */
	static void OnAddEntity(FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId);
	static void OnRemoveEntity(FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId);

protected:
	/** pending to add FBvMeshInstancingKey components (entity ids) */
	TArray<FBvTechArtEntityId> PendingEntitiesToAddMeshInstancingKeys;
};

/**
 * FBvProcessPendingEntitiesToAddMeshInstancingKeys
 * @brief	process pending MeshInstancingKeys
 */
class BVARTASSETPROCESSRUNTIME_API FBvProcessPendingEntitiesToAddMeshInstancingKeys : public FBvTechArtSystemInterface<FBvProcessPendingEntitiesToAddMeshInstancingKeys>
{
public:
	virtual void Execute(FBvTechArtSystemContext& InContext) final;
	virtual TCHAR* GetSystemName() const final
	{
		return TEXT("FBvProcessPendingEntitiesToAddMeshInstancingKeys");
	}
};

#define BV_TECHART_CULLING_SYSTEM_ENABLE_DEBUG 0
#define BVTECHART_DEBUG_CULLING_SYSTEM (BV_TECHART_CULLING_SYSTEM_ENABLE_DEBUG && (!(UE_BUILD_SHIPPING || UE_BUILD_TEST)))

namespace BvSMCEntityInBvLODActorEntity
{
	struct FBvInstancerType
	{
		/** instancer entity id */
		FBvTechArtEntityId InstancerEntityId;

		/**
		 * All attributes below is:
		 * * index == LODIndex
		 */

		/** LOD distances */
		TArray<float> LODDistances;

		/** LOD instance counts */
		TArray<uint32> LODInstanceCounts;

		/** LOD instance entities */
		TArray<TArray<FBvTechArtEntityId>> LODSMCEntities;
	};

	struct BVARTASSETPROCESSRUNTIME_API FBvSMCEntityContext : public FBvTechArtSystemContext
	{
		/** tracking thread id that the BvCullingSystem run*/
		uint32 BvCullingSystemRunningThreadId;

		/** updated SMCEntities by FBvCullingSystem */
		TSortedMap<FBvTechArtEntityId, FBvState> SMCEntityStates;

		/** visible entities */
		TArray<FBvTechArtEntityId> VisibleSMCEntities;

		/** instancer types */
		TMap<FBvTechArtEntityId, FBvInstancerType> InstancerTypes;
	};

	/**
	 * FBvSMCEntityCullingSystem
	 * @brief process culling SMCEntity in LODActor
	 */
	class BVARTASSETPROCESSRUNTIME_API FBvCullingSystem : public FBvTechArtSystemInterface<FBvCullingSystem>
	{
	public:
		using Super = FBvTechArtSystemInterface<FBvCullingSystem>;

		FBvCullingSystem();

		virtual void Execute(FBvTechArtSystemContext& InContext) final;
		virtual TCHAR* GetSystemName() const final
		{
			return TEXT("BvSMCEntityInBvLODActorEntity::FBvCullingSystem");
		}

#if BVTECHART_DEBUG_CULLING_SYSTEM
		/** generate debug info for culling data */
		void GenerateDebugInfoForCulling();

		bool bDebugDraw = false;
		bool bFreezeRendering = false;

		/** view frustum debug-entity */
		FBvTechArtEntityId ViewFrustumDebugEntity;
#endif
	};

	/**
	 * FBvHISMCUpdater
	 * @brief	update HISMC instances based on FBvState
	 */
	class BVARTASSETPROCESSRUNTIME_API FBvHISMCUpdater : public FBvTechArtSystemInterface<FBvHISMCUpdater>
	{
	public:
		using Super = FBvTechArtSystemInterface<FBvHISMCUpdater>;

		FBvHISMCUpdater()
			: Super()
		{}

		virtual void Execute(FBvTechArtSystemContext& InContext) final;

		/** old version of BvHISMCUpdater with UHierarchicalInstancedStaticMeshComponent */
		void ExecuteInnerOld(FBvTechArtSystemContext& InContext);

		virtual TCHAR* GetSystemName() const final
		{
			return TEXT("BvSMCEntityInBvLODActorEntity::FBvHISMCUpdater");
		}
	};

	/**
	 * FBvLODCalculator
	 * @brief calculate LOD index and LOD instance counts
	 */
	class BVARTASSETPROCESSRUNTIME_API FBvInstanceDataGenerator : public FBvTechArtSystemInterface<FBvInstanceDataGenerator>
	{
	public:
		using Super = FBvTechArtSystemInterface<FBvInstanceDataGenerator>;

		FBvInstanceDataGenerator()
			: Super()
		{}

		virtual void Execute(FBvTechArtSystemContext& InContext) final;
		virtual TCHAR* GetSystemName() const final
		{
			return TEXT("BvSMCEntityInBvLODActorEntity::FBvInstanceDataGenerator");
		}
	};
}

/**
 * @author	SangHyeok Hong
 * @brief	Utils for BvLODActor
 */
class BVARTASSETPROCESSRUNTIME_API FBvLODActorUtils
{
public:
	/** Recursively extract all SMCs from BvLODActor */
	static void ExtractSMCsFromBvLODActor(class ABvLODActor* InBvLODActor, TArray<class UStaticMeshComponent*>& OutSMCs);

	/** filter all SMCs which could generate ISMCs */
	static void FilterSMCsForPossibleISMCs(class ABvLODActor* InBvLODActor, TArray<class UStaticMeshComponent*>& OutSMCs);

	/** cache BvLODActor */
	static void UpdateBvLODActors(class UWorld* InWorld);

	/** get BvLODActor by FPrimitiveSceneInfo */
	static void DebugBvLODActorByPrimitiveSceneInfo(class FPrimitiveSceneInfo* InPrimitiveSceneInfo);

protected:
	static TArray<class ABvLODActor*> CachedBvLODActors;
};

/**
 * @author	SangHyeok Hong
 * @brief	ALODActor extension for TechArt Plugin
 */
UCLASS(notplaceable, hidecategories = (Object, Collision, Display, Input, Blueprint, Transform, Physics))
class BVARTASSETPROCESSRUNTIME_API ABvLODActor : public ALODActor
	, public FBvTechArtGameObjectHandle<FBvLODActorEntity>
{
public:
	GENERATED_UCLASS_BODY()

	using Super = ALODActor;

	virtual void SetComponentsMinDrawDistance(float InMinDrawDistance, bool bInMarkRenderStateDirty) override;
	virtual void RegisterMeshComponents() override;
	virtual void UnregisterMeshComponents() override;
	virtual FBox GetComponentsBoundingBox(bool bNonColliding = false, bool bIncludeFromChildActors = false) const override;
	virtual void SetLODParent(UPrimitiveComponent* InLODParent, float InParentDrawDistance, bool bInApplyToImposters) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void AddSubActor(AActor* InActor) override;
	virtual const bool RemoveSubActor(AActor* InActor) override;
	virtual void SetHiddenFromEditorView(const bool InState, const int32 ForceLODLevel) override;
	virtual void RecalculateDrawingDistance(const float TransitionScreenSize) override;
	virtual void UpdateSubActorLODParents() override;
	virtual void DetermineShadowingFlags() override;
#endif
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	virtual const bool IsBuilt(bool bInForce = false) const override;
#endif
	void SetDummyStaticMesh();

protected:
	static void CacheDummySM();

	/** dummy static mesh for proxy rendering of blueprint object */
	static UStaticMesh* DummySM;

	/** draw distance to transit BP */
	static float BlueprintTransitionDistance;

	friend struct FBvTechArtHierarchicalLODBuilder;
};
