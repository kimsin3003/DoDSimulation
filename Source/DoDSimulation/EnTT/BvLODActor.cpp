// Copyright Krafton. Beaver Lab. All Rights Reserved.

#include "BvLODActor.h"
#include "BvDebugHelper.h"
#include "BvTechArtAssetManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Math/BoxSphereBounds.h"
#include "BvScopeTestDuplicate.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/HLODProxy.h"
#include "BvTechArtDebugEntity.h"
#include "BvTechArtMeshInstancingKey.h"
#include "BvTechArtInstancerDB.h"
#include "BvTechArtGlobalDB.h"
#include "BvTechArtCoreDelegates.h"

#define LOCTEXT_NAMESPACE "BvLODActor"
DEFINE_LOG_CATEGORY(BvLODActor)

//////////////////////////////////////////////////////////////////////////
// ABvLODActor

BV_DECLARE_DEBUG_VARIABLE(void, BvLODActorDebugValuePtr0);

// implement the relationship between ABvLODActor and FBvLODActorEntity
BV_TECHART_IMPLEMENT_UOBJECTTYPE_ENTITYTYPE_RELATIONSHIP(ABvLODActor, FBvLODActorEntity, FBvLODActorDB);

TArray<ABvLODActor*> FBvLODActorUtils::CachedBvLODActors;
UStaticMesh* ABvLODActor::DummySM = nullptr;

/** temporary transition distance for BP object */
//float ABvLODActor::BlueprintTransitionDistance = 10000.0f;
float ABvLODActor::BlueprintTransitionDistance = 200000.0f;

/**
 * FBvLODActorDB
 */
void FBvLODActorDB::OnAddEntity(FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId)
{
	check(IsInGameThread());

	using FBvSMCs = BvLODActorEntity::FBvSMCs;

	// get the component from EntityId
	const FBvTechArtGameObjectComponent* GameObjectComponent = InTechArtDB.Query<FBvTechArtGameObjectComponent>().GetComponent(InEntityId);
	ensure(GameObjectComponent);

	// using GameObjectComponent, add components for FBvLODActorEntity
	ABvLODActor* Actor = Cast<ABvLODActor>(GameObjectComponent->UnrealObject.Get());
	ensure(Actor);

	// add FBvSMCs with FBvSMCEntityInBvLODActorEntity's EntityId
	{
		FBvSMCs BvSMCs;
		using FBvSMCRef = BvSMCEntityInBvLODActorEntity::FBvSMCRef;
		using FBvState = BvSMCEntityInBvLODActorEntity::FBvState;

		// extract all SMCs from BvLODActor
		TArray<UStaticMeshComponent*> SMCs;
		FBvLODActorUtils::ExtractSMCsFromBvLODActor(Actor, SMCs);

		// create new FBvSMCEntityInBvLODActorEntity
		for (auto& SMC : SMCs)
		{
			FBvTechArtEntityId SMCEntityId = FBvSMCInBvLODActorDB::Get().AddEntity();
			BvSMCs.SMCIds.Add(SMCEntityId);

			// add component FBvSMCRef
			{
				FBvSMCRef SMCRef{ SMC };
				FBvSMCInBvLODActorDB::Get().AddComponent(SMCEntityId, MoveTemp(SMCRef));
			}
		}

		InTechArtDB.AddComponent(InEntityId, MoveTemp(BvSMCs));
	}
}

void FBvLODActorDB::OnRemoveEntity(FBvTechArtDB<FBvLODActorDB, FBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId)
{
	check(IsInGameThread());

	using FBvSMCs = BvLODActorEntity::FBvSMCs;

	// get the FBvSMCs component
	const FBvSMCs* BvSMCs = InTechArtDB.Query<FBvSMCs>().GetComponent(InEntityId);
	ensure(BvSMCs);

	// remove entity in FBvSMCInBvLODActorDB
	const TArray<FBvTechArtEntityId>& BvSMCEntityIds = BvSMCs->SMCIds;
	{
		for (FBvTechArtEntityId EntityId : BvSMCEntityIds)
		{
			FBvSMCInBvLODActorDB::Get().RemoveEntity(EntityId);
		}
	}
}

/**
 * FBvSMCInBvLODActorDB
 */
void FBvSMCInBvLODActorDB::OnAddEntity(FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId)
{
	check(IsInGameThread());

	using FBvSMCRef = BvSMCEntityInBvLODActorEntity::FBvSMCRef;
	using FBvBounds = BvSMCEntityInBvLODActorEntity::FBvBounds;
	using FBvState = BvSMCEntityInBvLODActorEntity::FBvState;
	using FBvTransform = BvSMCEntityInBvLODActorEntity::FBvTransform;
	using FBvDebugEntity = BvSMCEntityInBvLODActorEntity::FBvDebugEntity;
	using FBvMeshInstancingKey = BvSMCEntityInBvLODActorEntity::FBvMeshInstancingKey;

	// get ThisDB
	FBvSMCInBvLODActorDB& ThisDB = (FBvSMCInBvLODActorDB&)InTechArtDB;

	// get the FBvSMCRef
	const FBvSMCRef* SMCRefComponent = InTechArtDB.Query<FBvSMCRef>().GetComponent(InEntityId);
	ensure(SMCRefComponent);
	ensure(SMCRefComponent->SMC.IsValid());

	// get MeshInstancingKeyLUG
	TMap<FBvComponentEntryKeyForMeshInstancing, FBvTechArtEntityId> MeshInstancingKeyLUT = FBvTechArtMeshInstancingKeyEntityDB::Get().GetMeshInstancingKeyLUT();

	// create components
	UStaticMeshComponent* SMC = SMCRefComponent->SMC.Get();
	{
		// BvBounds component
		{
			FBox Bounds = SMC->Bounds.GetBox();
			FBvBounds BoundsComponent{ Bounds.Min, Bounds.Max };
			InTechArtDB.AddComponent(InEntityId, MoveTemp(BoundsComponent));
		}

		// BvState component
		{
			FBvState StateComponent;
			StateComponent.AliasedData.Data.IsVisible = SMC->IsVisible();			

			// by default, set initial visibility for SMSEntity as visible
			StateComponent.AliasedData.Data.IsCulled = false;
			StateComponent.AliasedData.Data.WasCulled = true;

			InTechArtDB.AddComponent(InEntityId, MoveTemp(StateComponent));
		}

		// BvTransform component
		{
			FBvTransform TransformComponent{ SMC->GetComponentToWorld() };
			InTechArtDB.AddComponent(InEntityId, MoveTemp(TransformComponent));
		}

		// BvMeshInstancingKey component
		{
			FBvComponentEntryKeyForMeshInstancing MeshInstancingKeyData(SMC);

			FBvTechArtEntityId* MeshInstancingKeyEntityId = MeshInstancingKeyLUT.Find(MeshInstancingKeyData);
			if (MeshInstancingKeyEntityId)
			{
				// CAN add MeshInstancingKey component
				FBvMeshInstancingKey MeshInstancingKeyComponent{ *MeshInstancingKeyEntityId };
				InTechArtDB.AddComponent(InEntityId, MoveTemp(MeshInstancingKeyComponent));
			}
			else
			{
				// failed to add MeshInstancingKey component, so defer to add MeshInstancingKey component
				ThisDB.PendingEntitiesToAddMeshInstancingKeys.Add(InEntityId);
			}
		}

#if BV_TECHART_DEBUG_SMC_ENTITY
		// FBvDebugEntity component
		{
			FBvTechArtEntityId DebugEntityId = FBvDebugEntityDB::Get().AddEntity();

			// add FBvDebugEntity's components
			{
				// get SMC's world location
				FVector WorldLocation = SMC->GetComponentToWorld().GetLocation();

				FBvDebugDrawSphere DebugDrawSphereComponent;
				DebugDrawSphereComponent.bDraw = false;
				DebugDrawSphereComponent.DebugDrawColor = FColor::Red;
				DebugDrawSphereComponent.Position = WorldLocation;
				
				FBvDebugEntityDB::Get().AddComponent(DebugEntityId, MoveTemp(DebugDrawSphereComponent));
			}

			FBvDebugEntity DebugEntityComponent{ DebugEntityId };
			InTechArtDB.AddComponent(InEntityId, MoveTemp(DebugEntityComponent));
		}
#endif
	}
}

void FBvSMCInBvLODActorDB::OnRemoveEntity(FBvTechArtDB<FBvSMCInBvLODActorDB, FBvSMCEntityInBvLODActorEntity>& InTechArtDB, const FBvTechArtEntityId& InEntityId)
{
	check(IsInGameThread());

#if BV_TECHART_DEBUG_SMC_ENTITY
	using FBvDebugEntity = BvSMCEntityInBvLODActorEntity::FBvDebugEntity;

	// remove DebugEntityComponent
	{
		const FBvDebugEntity* DebugEntityComponent = InTechArtDB.Query<FBvDebugEntity>().GetComponent(InEntityId);
		ensure(DebugEntityComponent);

		FBvDebugEntityDB::Get().RemoveEntity(DebugEntityComponent->DebugEntityId);
	}
#endif
}

/**
 * FBvProcessPendingEntitiesToAddMeshInstancingKeys
 */

void FBvProcessPendingEntitiesToAddMeshInstancingKeys::Execute(FBvTechArtSystemContext& InContext)
{
	using FBvSMCRef = BvSMCEntityInBvLODActorEntity::FBvSMCRef;
	using FBvMeshInstancingKey = BvSMCEntityInBvLODActorEntity::FBvMeshInstancingKey;

	//*** construct newly added MeshInstancingKey	
	TMap<FBvComponentEntryKeyForMeshInstancing, TArray<FBvTechArtEntityId>> EntitiesForMeshInstancingKeys;
	{
		auto& PendingEntities = FBvSMCInBvLODActorDB::Get().GetPendingEntitiesToAddMeshInstancingKeys();
		for (FBvTechArtEntityId PendingEntityId : PendingEntities)
		{
			const FBvSMCRef* SMCRef = FBvSMCInBvLODActorDB::Get().Query<FBvSMCRef>().GetComponent(PendingEntityId);
			ensure(SMCRef);
			ensure(SMCRef->SMC.IsValid());

			FBvComponentEntryKeyForMeshInstancing MeshInstancingKeyData(SMCRef->SMC.Get());
			TArray<FBvTechArtEntityId>& SMCEntities = EntitiesForMeshInstancingKeys.FindOrAdd(MeshInstancingKeyData);
			SMCEntities.Add(PendingEntityId);
		}
	}

	//*** add MeshInstancingKey and 
	for (auto Iter = EntitiesForMeshInstancingKeys.CreateIterator(); Iter; ++Iter)
	{
		FBvComponentEntryKeyForMeshInstancing& MeshInstancingKey = Iter.Key();
		auto& SMCEntities = Iter.Value();

		// create MeshInstancingKeyEntity
		FBvTechArtEntityId MeshInstancingKeyEntity = FBvTechArtMeshInstancingKeyEntityDB::Get().AddEntity();
		{
			using FBvMeshInstancingKeyData = BvTechArtInstancingKey::FBvMeshInstancingKeyData;

			FBvMeshInstancingKeyData KeyDataComponent{ MeshInstancingKey };
			FBvTechArtMeshInstancingKeyEntityDB::Get().AddComponent(MeshInstancingKeyEntity, MoveTemp(KeyDataComponent));
		}

		// create BvMeshInstancingKey
		for (FBvTechArtEntityId SMCEntity : SMCEntities)
		{
			FBvMeshInstancingKey MeshInstancingKeyComponent{ MeshInstancingKeyEntity };
			FBvSMCInBvLODActorDB::Get().AddComponent(SMCEntity, MoveTemp(MeshInstancingKeyComponent));
		}		
	}
}

/**
 * BvSMCEntityInBvLODActorEntity::FBvCullingSystem
 */

using namespace BvSMCEntityInBvLODActorEntity;

FBvCullingSystem::FBvCullingSystem()
	: Super()
{
#if BVTECHART_DEBUG_CULLING_SYSTEM
	// add view-frustum debug entity
	ViewFrustumDebugEntity = FBvDebugEntityDB::Get().AddEntity();
	{
		FBvDebugDrawFrustum FrustumComponent;
		FBvDebugEntityDB::Get().AddComponent(ViewFrustumDebugEntity, MoveTemp(FrustumComponent));
	}
#endif
}

void FBvCullingSystem::Execute(FBvTechArtSystemContext& InContext)
{
	static const VectorRegister	VECTOR_HALF_HALF_HALF_ZERO = DECLARE_VECTOR_REGISTER(0.5f, 0.5f, 0.5f, 0.0f);

	BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext& Context = (BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext&)InContext;
	Context.BvCullingSystemRunningThreadId = FPlatformTLS::GetCurrentThreadId();

	using FBvViewMatrices = TechArtGlobal::FBvViewMatrices;
	using FBvBounds = BvSMCEntityInBvLODActorEntity::FBvBounds;
	using FBvState = BvSMCEntityInBvLODActorEntity::FBvState;

	// get FBvViewMatrices
	FBvTechArtEntityId GlobalEntityId = FBvTechArtGlobalDB::Get().GetGlobalEntityId();
	const FBvViewMatrices* ViewMatrices = FBvTechArtGlobalDB::Get().Query<FBvViewMatrices>().GetComponent(GlobalEntityId);
	if (ViewMatrices == nullptr || ViewMatrices->FrameNumberRenderThread == 0)
	{
		// still ViewMatrices is NOT updated in render-thread
		return;
	}

	auto BvBoundsReadOnlyView = FBvSMCInBvLODActorDB::Get().Query<FBvBounds>();
	auto BvStateReadOnlyView = FBvSMCInBvLODActorDB::Get().Query<FBvState>();
	if (!BvBoundsReadOnlyView.IsValidView())
	{
		// if View is NOT valid, skip to Execute()
		return;
	}

	// get view-frustum
	const FConvexVolume& ViewFrustum = ViewMatrices->ViewFrustum;

	//*** process CPU culling
	double ElapsedTime;
	{
		// measure elapsed time
		FBvScopedElapsedTime ScopeElapsedTime(ElapsedTime);
		
		auto& SMCEntities = BvStateReadOnlyView.GetEntities();
		auto& BvStateForSMCs = BvStateReadOnlyView.GetComponents();
		auto& BvBoundsForSMCs = BvBoundsReadOnlyView.GetComponents();		
		ensure(BvBoundsForSMCs.Num() == BvStateForSMCs.Num());

		int32 NumComponentsForSMCs = BvBoundsForSMCs.Num();

		const FBvTechArtEntityId* SMCEntityIds = (const FBvTechArtEntityId*)SMCEntities.GetData();
		const FVector* BoundsPtr = (const FVector*)BvBoundsForSMCs.GetData();
		const FBvState* StatePtr = (const FBvState*)BvStateForSMCs.GetData();

		// Since we are moving straight through get a pointer to the data
		ensure(ViewFrustum.PermutedPlanes.Num() == 4);
		const FPlane* RESTRICT PermutedPlanePtr = (FPlane*)ViewFrustum.PermutedPlanes.GetData();

		// Process four planes at a time until we have < 4 left
		// Load 4 planes that are already all Xs, Ys, ...
		const VectorRegister PlanesX = VectorLoadAligned(&PermutedPlanePtr[0]);
		const VectorRegister PlanesY = VectorLoadAligned(&PermutedPlanePtr[1]);
		const VectorRegister PlanesZ = VectorLoadAligned(&PermutedPlanePtr[2]);
		const VectorRegister PlanesW = VectorLoadAligned(&PermutedPlanePtr[3]);

		for (int32 Index = 0; Index < NumComponentsForSMCs; ++Index)
		{
			int32 BoundMinMaxIndex = Index * 2;
			const FVector* BoundsMin = (BoundsPtr + BoundMinMaxIndex);
			const FVector* BoundsMax = (BoundsPtr + BoundMinMaxIndex + 1);

			// cache entity id
			FBvTechArtEntityId SMCEntityId = *(SMCEntityIds + Index);

			// cache local StateData
			FBvState State = *(StatePtr + Index);
			FBvState::FData& StateData = State.AliasedData.Data;

			// only handles visible SMCs
			if (StateData.IsVisible)
			{
				VectorRegister BoxMin = VectorLoadFloat3(BoundsMin);
				VectorRegister BoxMax = VectorLoadFloat3(BoundsMax);

				VectorRegister BoxDiff = VectorSubtract(BoxMax, BoxMin);
				VectorRegister BoxSum = VectorAdd(BoxMax, BoxMin);

				// Load the origin & extent
				VectorRegister Orig = VectorMultiply(VECTOR_HALF_HALF_HALF_ZERO, BoxSum);
				VectorRegister Ext = VectorMultiply(VECTOR_HALF_HALF_HALF_ZERO, BoxDiff);
				// Splat origin into 3 vectors
				VectorRegister OrigX = VectorReplicate(Orig, 0);
				VectorRegister OrigY = VectorReplicate(Orig, 1);
				VectorRegister OrigZ = VectorReplicate(Orig, 2);
				// Splat the abs for the pushout calculation
				VectorRegister AbsExtentX = VectorReplicate(Ext, 0);
				VectorRegister AbsExtentY = VectorReplicate(Ext, 1);
				VectorRegister AbsExtentZ = VectorReplicate(Ext, 2);
				// Calculate the distance (x * x) + (y * y) + (z * z) - w
				VectorRegister DistX = VectorMultiply(OrigX, PlanesX);
				VectorRegister DistY = VectorMultiplyAdd(OrigY, PlanesY, DistX);
				VectorRegister DistZ = VectorMultiplyAdd(OrigZ, PlanesZ, DistY);
				VectorRegister Distance = VectorSubtract(DistZ, PlanesW);
				// Now do the push out FMath::Abs(x * x) + FMath::Abs(y * y) + FMath::Abs(z * z)
				VectorRegister PushX = VectorMultiply(AbsExtentX, VectorAbs(PlanesX));
				VectorRegister PushY = VectorMultiplyAdd(AbsExtentY, VectorAbs(PlanesY), PushX);
				VectorRegister PushOut = VectorMultiplyAdd(AbsExtentZ, VectorAbs(PlanesZ), PushY);

				// Check for completely outside
				bool IsCulled = !!VectorAnyGreaterThan(Distance, PushOut);

				// when IsCulled data is NOT same, update the value and queue to update
				if (StateData.IsCulled != IsCulled)
				{
					StateData.IsCulled = IsCulled;					

					if (FBvTechArtGlobal::EnableGatherInstancing())
					{
						// partial update to minimize overhead for DB component update
						StateData.WasCulled = IsCulled;
						FBvSMCInBvLODActorDB::Get().QueueToUpdate(SMCEntityId, MoveTemp(State));
					}
					else
					{
						// add SMC entity's state to the context
						Context.SMCEntityStates.Add(SMCEntityId, State);
					}
				}

				// add visible entities
				if (FBvTechArtGlobal::EnableGatherInstancing())
				{
					if (!IsCulled)
					{
						Context.VisibleSMCEntities.Add(SMCEntityId);
					}
				}				
			}			
		}
	}

	BV_DEBUG_VALUE(true, ElapsedTime, BvLODActorDebugValuePtr0);

#if BVTECHART_DEBUG_CULLING_SYSTEM
	GenerateDebugInfoForCulling();
#endif
}

#if BVTECHART_DEBUG_CULLING_SYSTEM
void FBvCullingSystem::GenerateDebugInfoForCulling()
{
	using FBvViewMatrices = TechArtGlobal::FBvViewMatrices;
	using FBvState = BvSMCEntityInBvLODActorEntity::FBvState;
	using FBvDebugEntity = BvSMCEntityInBvLODActorEntity::FBvDebugEntity;

	// get FBvViewMatrices
	FBvTechArtEntityId GlobalEntityId = FBvTechArtGlobalDB::Get().GetGlobalEntityId();
	const FBvViewMatrices* ViewMatrices = FBvTechArtGlobalDB::Get().Query<FBvViewMatrices>().GetComponent(GlobalEntityId);
	ensure(ViewMatrices);

	if (ViewMatrices->FrameNumberRenderThread == 0)
	{
		// still ViewMatrices is NOT updated in render-thread
		return;
	}

	const FConvexVolume& ViewFrustum = ViewMatrices->ViewFrustum;

	FMatrix ViewFrustumToWorldToDebug;
	FVector PlaneNormalsToDebug[4];

	if (!bFreezeRendering)
	{
		//*** debug-draw for view frustum
		{
			// cache ViewFrustum plane's normals
			PlaneNormalsToDebug[0] = *((FVector*)&ViewFrustum.Planes[0]);
			PlaneNormalsToDebug[1] = *((FVector*)&ViewFrustum.Planes[1]);
			PlaneNormalsToDebug[2] = *((FVector*)&ViewFrustum.Planes[2]);
			PlaneNormalsToDebug[3] = *((FVector*)&ViewFrustum.Planes[3]);
		}

		//*** get all visible debug entities
		{
			TSet<FBvTechArtEntityId> VisibleDebugEntities;
			{
				auto& BvStateForSMCs = FBvSMCInBvLODActorDB::Get().Query<FBvState>().GetComponents();
				auto& BvDebugEntityForSMCs = FBvSMCInBvLODActorDB::Get().Query<FBvDebugEntity>().GetComponents();
				ensure(BvStateForSMCs.Num() == BvDebugEntityForSMCs.Num());

				int32 NumComponentsForSMCs = BvStateForSMCs.Num();

				// flat memory access
				const FBvState::FData* StatePtr = (const FBvState::FData*)BvStateForSMCs.GetData();
				const FBvTechArtEntityId* DebugEntityPtr = (const FBvTechArtEntityId*)BvDebugEntityForSMCs.GetData();

				for (int32 Index = 0; Index < NumComponentsForSMCs; ++Index)
				{
					const FBvState::FData& StateData = *(StatePtr + Index);
					const FBvTechArtEntityId& DebugEntityId = *(DebugEntityPtr + Index);

					if (!StateData.IsVisible)
					{
						continue;
					}

					// when state is visible, add it to the queue
					if (!StateData.IsCulled)
					{
						VisibleDebugEntities.Add(DebugEntityId);
					}
				}
			}

			// update DebugEntity which is only visible
			{
				auto DebugDrawSpheres = FBvDebugEntityDB::Get().Query<FBvDebugDrawSphere>();
				DebugDrawSpheres.Each([&VisibleDebugEntities](FBvTechArtEntityId EntityId, const FBvDebugDrawSphere& DebugDrawSphere)
					{
						FBvDebugDrawSphere DebugDrawSphereToUpdate = DebugDrawSphere;
						DebugDrawSphereToUpdate.bDraw = VisibleDebugEntities.Contains(EntityId);

						FBvDebugEntityDB::Get().QueueToUpdate<FBvDebugDrawSphere>(EntityId, MoveTemp(DebugDrawSphereToUpdate));
					});
			}
		}
	}
	//*** debug-draw for view frustum
	{
		// update the debug-draw component
		FBvDebugDrawFrustum UpdatedDebugDrawFrustum;
		UpdatedDebugDrawFrustum.bDraw = bDebugDraw;
		UpdatedDebugDrawFrustum.DebugDrawColor = FColor::Red;
		UpdatedDebugDrawFrustum.ViewProjectionMatrix = ViewFrustumToWorldToDebug;

		// set frustum plane normals
		UpdatedDebugDrawFrustum.PlaneNormals[EBvFrustumPlane::LEFT] = PlaneNormalsToDebug[0];
		UpdatedDebugDrawFrustum.PlaneNormals[EBvFrustumPlane::RIGHT] = PlaneNormalsToDebug[1];
		UpdatedDebugDrawFrustum.PlaneNormals[EBvFrustumPlane::TOP] = PlaneNormalsToDebug[2];
		UpdatedDebugDrawFrustum.PlaneNormals[EBvFrustumPlane::BOTTOM] = PlaneNormalsToDebug[3];

		// queue to update the component data
		FBvDebugEntityDB::Get().QueueToUpdate<FBvDebugDrawFrustum>(ViewFrustumDebugEntity, MoveTemp(UpdatedDebugDrawFrustum));
	}
}
#endif

void BvCullSystemFreezeRendering(UWorld* InWorld, bool Next)
{
#if BVTECHART_DEBUG_CULLING_SYSTEM
	// temporary to use debug variables
	static bool ToggleDebugging = false;
	ToggleDebugging = !ToggleDebugging;

	if (ToggleDebugging)
	{
		FBvCullingSystem::Get().bDebugDraw = true;
		FBvCullingSystem::Get().bFreezeRendering = true;
	}
	else
	{
		FBvCullingSystem::Get().bDebugDraw = false;
		FBvCullingSystem::Get().bFreezeRendering = false;
	}
#endif
}

FAutoConsoleCommandWithWorld ExecuteCullSystemFreezeRendering(
	TEXT("BvTechArt.CullSystem.bFreezeRendering"),
	TEXT("BvTechArt.CullSystem.bFreezeRendering"),
	FConsoleCommandWithWorldDelegate::CreateStatic(BvCullSystemFreezeRendering, true)
);

/**
 * FBvHISMCUpdater
 */
void FBvHISMCUpdater::Execute(FBvTechArtSystemContext& InContext)
{
	if (!FBvTechArtGlobal::EnableGatherInstancing())
	{
		ExecuteInnerOld(InContext);
	}	
}

BV_DEBUG_NO_OPTIMIZE_START
void FBvHISMCUpdater::ExecuteInnerOld(FBvTechArtSystemContext& InContext)
{
	BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext& Context = (BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext&)InContext;

	// make sure that FBvCullingSystem and FBvHISMCUpdater are running on the same thread
	ensure(Context.BvCullingSystemRunningThreadId == FPlatformTLS::GetCurrentThreadId());

	using FBvState = BvSMCEntityInBvLODActorEntity::FBvState;
	using FBvTransform = BvSMCEntityInBvLODActorEntity::FBvTransform;
	using FBvMeshInstancingKey = BvSMCEntityInBvLODActorEntity::FBvMeshInstancingKey;

	// if there is no SMC entity states, terminate this Execute loop
	if (Context.SMCEntityStates.Num() == 0)
	{
		return;
	}

	auto MeshInstancingKeyReadOnlyView = FBvSMCInBvLODActorDB::Get().Query<FBvMeshInstancingKey>();
	auto TransformReadOnlyView = FBvSMCInBvLODActorDB::Get().Query<FBvTransform>();
	if (!(MeshInstancingKeyReadOnlyView.IsValidView() && TransformReadOnlyView.IsValidView()))
	{
		return;
	}

	using FBvHISMCEntityId = BvTechArtInstancingKey::FBvHISMCEntityId;
	auto ReadOnlyViewforHISMCEntity = FBvTechArtMeshInstancingKeyEntityDB::Get().Query<FBvHISMCEntityId>();
	if (!ReadOnlyViewforHISMCEntity.IsValidView())
	{
		return;
	}

	using FBvSMCEntityTracker = BvTechArtInstancer::FBvSMCEntityTracker;
	auto ReadOnlyViewForSMCEntityTracker = FBvTechArtInstancerEntityDB::Get().Query<FBvSMCEntityTracker>();
	if (!ReadOnlyViewForSMCEntityTracker.IsValidView())
	{
		return;
	}

	// LUT (MeshInstancingKey -> HISMC)
	TMap<FBvTechArtEntityId, FBvTechArtEntityId> MeshInstancingKeyToHISMC;
	{
		auto& MeshInstancingKeyIds = ReadOnlyViewforHISMCEntity.GetEntities();
		auto& HISMCEntityIds = ReadOnlyViewforHISMCEntity.GetComponents();
		ensure(MeshInstancingKeyIds.Num() == HISMCEntityIds.Num());

		for (int32 Index = 0; Index < HISMCEntityIds.Num(); ++Index)
		{
			FBvTechArtEntityId HISMCEntityId = HISMCEntityIds[Index].EntityId;
			MeshInstancingKeyToHISMC.Add(MeshInstancingKeyIds[Index], HISMCEntityId);
		}
	}

	// instance update
	struct FBvInstanceUpdate
	{
		// AOS for instance data to add
		TBvInlineArray<FBvTechArtEntityId> InstancesToAdd;
		TBvInlineArray<FTransform> InstanceTransformsToAdd;

		// to remove instance, we only need entity-id
		TBvInlineArray<FBvTechArtEntityId> InstancesToRemove;
	};

	// HISMC entity id -> InstanceUpdate
	TMap<FBvTechArtEntityId, FBvInstanceUpdate> HISMCInstanceUpdates;
	{
		for (auto Iterator = MeshInstancingKeyToHISMC.CreateConstIterator(); Iterator; ++Iterator)
		{
			FBvTechArtEntityId HISMCId = Iterator.Value();
			HISMCInstanceUpdates.Add(HISMCId);
		}
	}

	//*** collect instances to add/remove
	{	
		for (auto Iter = Context.SMCEntityStates.CreateIterator(); Iter; ++Iter)
		{
			FBvTechArtEntityId EntityId = Iter.Key();
			FBvState& State = Iter.Value();

			const FBvTransform* TransformForSMC = TransformReadOnlyView.GetComponent(EntityId);
			ensure(TransformForSMC);
			const FBvMeshInstancingKey* MeshInstancingKeyForSMC = MeshInstancingKeyReadOnlyView.GetComponent(EntityId);
			ensure(MeshInstancingKeyForSMC);
			
			FBvState::FData& StateData = State.AliasedData.Data;
			check(StateData.IsVisible);

			// cache IsCulled and WasCulled
			bool IsCulled = StateData.IsCulled;
			bool WasCulled = StateData.WasCulled;

			// update WasCulled and queue to update BvState component
			StateData.WasCulled = IsCulled;
			FBvSMCInBvLODActorDB::Get().QueueToUpdate(EntityId, MoveTemp(State));

			// determine to add/remove instances
			if ((IsCulled ^ WasCulled) == 0)
			{
				// no need to update
				continue;
			}

			// find HISMCId
			FBvTechArtEntityId* HISMCId = MeshInstancingKeyToHISMC.Find(MeshInstancingKeyForSMC->MeshInstancingKeyId);
			ensure(HISMCId);

			// add/remove instance to HISMCInstanceUpdates
			FBvInstanceUpdate* InstancesToUpdate = HISMCInstanceUpdates.Find(*HISMCId);
			ensure(InstancesToUpdate);

			// add instance to HISMCInstanceUpdates
			if (!WasCulled)
			{
				InstancesToUpdate->InstancesToRemove.Add(EntityId);
			}
			else
			{
				InstancesToUpdate->InstancesToAdd.Add(EntityId);
				InstancesToUpdate->InstanceTransformsToAdd.Add(TransformForSMC->WorldTransform);
			}
		}
	}

	//*** iterate each HISMC proxy
	{
		using FBvHISMCObject = BvTechArtInstancer::FBvHISMCObject;

		for (auto HISMCIterator = HISMCInstanceUpdates.CreateIterator(); HISMCIterator; ++HISMCIterator)
		{
			FBvTechArtEntityId HISMCId = HISMCIterator.Key();
			FBvInstanceUpdate& InstanceUpdate = HISMCIterator.Value();

			// get FBvHISMCObject with HISMC entity id
			const FBvHISMCObject* HISMCObjectComponent = FBvTechArtInstancerEntityDB::Get().Query<FBvHISMCObject>().GetComponent(HISMCId);
			ensure(HISMCObjectComponent);
			ensure(HISMCObjectComponent->HISMCRef.IsValid());

			// cache HISMC to add/remove instances
			UHierarchicalInstancedStaticMeshComponent* HISMC = Cast<UHierarchicalInstancedStaticMeshComponent>((HISMCObjectComponent->HISMCRef).Get());
			ensure(HISMC);

			// get the SMCIdTracker component
			const FBvSMCEntityTracker* SMCIdTracker = FBvTechArtInstancerEntityDB::Get().Query<FBvSMCEntityTracker>().GetComponent(HISMCId);
			ensure(SMCIdTracker);

			// update EntityTracker component
			FBvSMCEntityTracker SMCIdTrackerToUpdate = *SMCIdTracker;

			//*** remove instances first
			{
				auto& InstancesToRemove = InstanceUpdate.InstancesToRemove;
				for (FBvTechArtEntityId& SMCId : InstancesToRemove)
				{
					int32 RemoveIndex = SMCIdTrackerToUpdate.RemoveInstance(SMCId);
					ensure(RemoveIndex != -1);

					HISMC->RemoveInstance(RemoveIndex);
				}
			}

			//*** add instances lastly
			{
				int32 NumToAdd = InstanceUpdate.InstancesToAdd.Num();
				ensure(NumToAdd == InstanceUpdate.InstanceTransformsToAdd.Num());

				for (int32 IndexToAdd = 0; IndexToAdd < NumToAdd; ++IndexToAdd)
				{
					FBvTechArtEntityId& SMCId = InstanceUpdate.InstancesToAdd[IndexToAdd];
					FTransform TransformForSMC = InstanceUpdate.InstanceTransformsToAdd[IndexToAdd];

					if (SMCIdTrackerToUpdate.HasInstance(SMCId))
					{
						// @todo - log when it duplicates instance
						continue;
					}

					int32 InstanceIndexToBeExpected = SMCIdTrackerToUpdate.AddInstance(SMCId);

					// update transform relative to HISMC's world transform
					FTransform RelativeTM = TransformForSMC.GetRelativeTransform(HISMC->GetComponentTransform());
					int32 InstanceIndex = HISMC->AddInstance(RelativeTM);
					ensure(InstanceIndex == InstanceIndexToBeExpected);
				}
			}

			// queue to update SMCIdTracker component
			FBvTechArtInstancerEntityDB::Get().QueueToUpdate(HISMCId, MoveTemp(SMCIdTrackerToUpdate));
		}
	}
}
BV_DEBUG_NO_OPTIMIZE_END

BV_DEBUG_NO_OPTIMIZE_START
void FBvInstanceDataGenerator::Execute(FBvTechArtSystemContext& InContext)
{
	BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext& Context = (BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext&)InContext;

	// make sure that FBvCullingSystem and FBvHISMCUpdater are running on the same thread
	ensure(Context.BvCullingSystemRunningThreadId == FPlatformTLS::GetCurrentThreadId());

	using FBvBounds = BvSMCEntityInBvLODActorEntity::FBvBounds;
	using FBvTransform = BvSMCEntityInBvLODActorEntity::FBvTransform;
	using FBvMeshInstancingKey = BvSMCEntityInBvLODActorEntity::FBvMeshInstancingKey;
	using FBvViewMatrices = TechArtGlobal::FBvViewMatrices;
	using FBvHISMCObject = BvTechArtInstancer::FBvHISMCObject;
	using FBvLODDistances = BvTechArtInstancer::FBvLODDistances;
	using FBvHISMCWorldTransform = BvTechArtInstancer::FBvHISMCWorldTransform;
	using FBvInstanceBufferInfo = BvTechArtInstancer::FBvInstanceBufferInfo;
	using FBvSMCEntities = BvTechArtInstancer::FBvSMCEntities;
	using FBvHISMCEntityId = BvTechArtInstancingKey::FBvHISMCEntityId;

	// FBvViewMatrices component read-only view
	auto BvViewMatrices = FBvTechArtGlobalDB::Get().Query<FBvViewMatrices>();
	if (!BvViewMatrices.IsValidView())
	{
		return;
	}

	// FBvBounds component read-only view
	auto BoundsView = FBvSMCInBvLODActorDB::Get().Query<FBvBounds>();
	if (!BoundsView.IsValidView())
	{
		return;
	}

	// FBvTransform component read-only view
	auto BvTransformView = FBvSMCInBvLODActorDB::Get().Query<FBvTransform>();
	if (!BvTransformView.IsValidView())
	{
		return;
	}

	// FBvMeshInstancingKey component read-only view
	auto BvMeshInstancingKeyView = FBvSMCInBvLODActorDB::Get().Query<FBvMeshInstancingKey>();
	if (!BvMeshInstancingKeyView.IsValidView())
	{
		return;
	}

	// FBvHISMCEntityId component read-only view
	auto BvHISMCEntities = FBvTechArtMeshInstancingKeyEntityDB::Get().Query<FBvHISMCEntityId>();
	if (!BvHISMCEntities.IsValidView())
	{
		return;
	}

	// FBvHISMCObject component read-only view
	auto BvHISMCObjectView = FBvTechArtInstancerEntityDB::Get().Query<FBvHISMCObject>();
	if (!BvHISMCObjectView.IsValidView())
	{
		return;
	}

	// FBvHISMCWorldTransform component read-only view
	auto BvHISMCWorldTransformView = FBvTechArtInstancerEntityDB::Get().Query<FBvHISMCWorldTransform>();
	if (!BvHISMCWorldTransformView.IsValidView())
	{
		return;
	}

	// FBvInstanceBufferInfo component read-only view
	auto BvInstanceBufferInfoView = FBvTechArtInstancerEntityDB::Get().Query<FBvInstanceBufferInfo>();
	if (!BvInstanceBufferInfoView.IsValidView())
	{
		return;
	}

	// FBvLODDistances component read-only view
	auto BvLODDistancesView = FBvTechArtInstancerEntityDB::Get().Query<FBvLODDistances>();
	if (!BvLODDistancesView.IsValidView())
	{
		return;
	}

	// when one frame off, process FBvInstanceDataGenerator
	uint32 RenderThreadTickNumber = FBvTechArtCoreDelegates::InstanceBufferInfoDataCache.TickNumber;
	uint32 GameThreadTickNumber = FBvTechArtSystemTicker::Get().GetTickNumber();
	if (FBvTechArtCoreDelegates::InstanceBufferInfoDataCache.TickNumber != GameThreadTickNumber)
	{
		return;
	}

	// SOA of preprocessed SMC entity data
	struct FBvPreprocessSMCEntityData
	{
		TArray<FBvTechArtEntityId> SMCEntities;
		TArray<FVector> Positions;
	};
	
	// construct LUT from instancer entity to visible SMC entities
	TMap<FBvTechArtEntityId, FBvPreprocessSMCEntityData> InstancerEntityToSMCEntities;
	{
		TArray<FBvTechArtEntityId>& VisibleSMCEntities = Context.VisibleSMCEntities;		

		for (FBvTechArtEntityId& EntityId : VisibleSMCEntities)
		{
			const FBvMeshInstancingKey* MeshInstancingKey = BvMeshInstancingKeyView.GetComponent(EntityId);
			ensure(MeshInstancingKey);

			const FBvHISMCEntityId* InstancerEntity = BvHISMCEntities.GetComponent(MeshInstancingKey->MeshInstancingKeyId);	
			ensure(InstancerEntity);

			const FBvBounds* BvBoundComponent = BoundsView.GetComponent(EntityId);
			ensure(BvBoundComponent);

			// calculate SMC entity position
			FVector Position = (BvBoundComponent->Min + BvBoundComponent->Max) / 2.0f;
			
			FBvPreprocessSMCEntityData& Data = InstancerEntityToSMCEntities.FindOrAdd(InstancerEntity->EntityId);
			Data.SMCEntities.Add(EntityId);
			Data.Positions.Add(Position);
		}
	}

	// construct instancer-types in FBvSMCEntityContext
	using FBvInstancerType = BvSMCEntityInBvLODActorEntity::FBvInstancerType;
	{
		// test whether half float is used
		bool bHalfFloat = GVertexElementTypeSupport.IsSupported(VET_Half2);

		for (auto Iter = InstancerEntityToSMCEntities.CreateIterator(); Iter; ++Iter)
		{
			FBvTechArtEntityId InstancerEntity = Iter.Key();
			FBvInstancerType* InstancerType = Context.InstancerTypes.Find(InstancerEntity);
			ensure(InstancerType == nullptr);

			// get LOD distances
			const FBvLODDistances* LODDistancesComponent = BvLODDistancesView.GetComponent(InstancerEntity);
			ensure(LODDistancesComponent);
			const TArray<float>& LODDistances = LODDistancesComponent->LODDistances;

			// add new instancer-types
			FBvInstancerType& AddedInstancerType = Context.InstancerTypes.Add(InstancerEntity);
			AddedInstancerType.InstancerEntityId = InstancerEntity;

			// set lod distances
			AddedInstancerType.LODDistances = LODDistances;

			// pre-size LOD instance data with LOD num
			int32 LODNum = LODDistances.Num();

			// note that LODInstanceCounts are initialized as '0'
			AddedInstancerType.LODInstanceCounts.AddZeroed(LODNum);

			// add default LOD SMC entities
			AddedInstancerType.LODSMCEntities.AddDefaulted(LODNum);
		}

		// calculate view-origin
		FVector ViewLocation = FVector::ZeroVector;
		{
			FBvTechArtEntityId GlobalEntityId = FBvTechArtGlobalDB::Get().GetGlobalEntityId();
			const FBvViewMatrices* ViewMatricesComponent = BvViewMatrices.GetComponent(GlobalEntityId);
			ensure(ViewMatricesComponent);

			ViewLocation = ViewMatricesComponent->ViewLocation;
		}

		// calculate LOD instance count
		for (auto Iter = InstancerEntityToSMCEntities.CreateIterator(); Iter; ++Iter)
		{
			FBvTechArtEntityId InstancerEntity = Iter.Key();
			FBvInstancerType* InstancerType = Context.InstancerTypes.Find(InstancerEntity);
			ensure(InstancerType != nullptr);

			auto& LODDistances = InstancerType->LODDistances;
			auto& LODInstanceCounts = InstancerType->LODInstanceCounts;

			auto& PreprocessedData = Iter.Value();
			ensure(PreprocessedData.Positions.Num() == PreprocessedData.SMCEntities.Num());

			const TArray<FBvTechArtEntityId>& SMCEntities = PreprocessedData.SMCEntities;
			const TArray<FVector>& EntityPositions = PreprocessedData.Positions;
			for (int32 Index = 0; Index < EntityPositions.Num(); ++Index)
			{
				// get SMC entity id
				FBvTechArtEntityId SMCEntityId = SMCEntities[Index];

				// calculate distance between view and entity locations
				float Distance = (EntityPositions[Index] - ViewLocation).Size();
				
				// calculate LOD index
				uint32 CalcLODIndex = LODDistances.Num() - 1;
				for (int32 LODIndex = 1; LODIndex < LODDistances.Num(); ++LODIndex)
				{
					if (Distance < LODDistances[LODIndex])
					{
						CalcLODIndex = LODIndex - 1;
						break;
					}
				}

				// increment instance counts
				InstancerType->LODInstanceCounts[CalcLODIndex]++;

				// add instance entities
				InstancerType->LODSMCEntities[CalcLODIndex].Add(SMCEntityId);
			}
		}

		// update LOD instance buffer data
		for (auto Iter = InstancerEntityToSMCEntities.CreateIterator(); Iter; ++Iter)
		{
			FBvTechArtEntityId InstancerEntity = Iter.Key();
			FBvInstancerType* InstancerType = Context.InstancerTypes.Find(InstancerEntity);
			ensure(InstancerType != nullptr);

			// get instancer world transform
			const FBvHISMCWorldTransform* HISMCWorldTransform = BvHISMCWorldTransformView.GetComponent(InstancerEntity);
			ensure(HISMCWorldTransform);

			// get instance buffer info
			const FBvInstanceBufferInfo* InstanceBufferInfo = BvInstanceBufferInfoView.GetComponent(InstancerEntity);
			ensure(InstanceBufferInfo);

			// get HISMCObject and its transform data
			const FBvHISMCObject* HISMCObject = BvHISMCObjectView.GetComponent(InstancerEntity);
			ensure(HISMCObject);
			ensure(HISMCObject->HISMCRef.IsValid());

			FTransform HISMCTransform = HISMCObject->HISMCRef->GetComponentToWorld();

			// new instance buffer info
			FBvInstanceBufferInfo InstanceBufferInfoToUpdate = *InstanceBufferInfo;

			// new SMCEntities
			FBvSMCEntities NewSMCEntities;

			auto& LODSMCEntities = InstancerType->LODSMCEntities;
			for (int32 LODIndex = 0; LODIndex < LODSMCEntities.Num(); ++LODIndex)
			{
				// get instance count for each LOD
				uint32 InstanceCount = InstancerType->LODInstanceCounts[LODIndex];
				if (InstanceCount == 0)
				{
					// update instance buffer info update
					InstanceBufferInfoToUpdate.Offsets[LODIndex] = 0;
					InstanceBufferInfoToUpdate.Counts[LODIndex] = 0;

					// no need to manipulating global instance buffer
					continue;
				}

				// get reference to SMCEntities for each LODIndex
				TArray<FBvTechArtEntityId>& SMCEntityIdsLOD = NewSMCEntities.SMCEntityIds[LODIndex];

				// request instance buffer copy desc
				ensure(FBvTechArtCoreDelegates::RequestInstanceCopyDesc.IsBound());
				FBvInstanceBufferCopyDesc InstanceBufferCopyDesc = FBvTechArtCoreDelegates::RequestInstanceCopyDesc.Execute(InstanceCount);

				// set instance buffer info
				InstanceBufferInfoToUpdate.Offsets[LODIndex] = InstanceBufferCopyDesc.InstanceOffset;
				InstanceBufferInfoToUpdate.Counts[LODIndex] = InstanceBufferCopyDesc.NumInstances;

				// get buffer start-address and end of size
				uint32 InstanceOriginBufferDataSize = InstanceBufferCopyDesc.Size[FBvInstanceBufferCopyDesc::Origin];
				uint32 InstanceOriginBufferDataStride = sizeof(FVector4);
				ensure(InstanceOriginBufferDataSize / InstanceOriginBufferDataStride == InstanceCount);
				uint8* InstanceOriginBufferDataPtr = InstanceBufferCopyDesc.Data[FBvInstanceBufferCopyDesc::Origin];

				uint32 InstanceTransformBufferDataSize = InstanceBufferCopyDesc.Size[FBvInstanceBufferCopyDesc::Transform];
				uint32 InstanceTransformBufferDataStride = sizeof(FFloat16) * 4 * 3;
				ensure(InstanceTransformBufferDataSize / InstanceTransformBufferDataStride == InstanceCount);
				uint8* InstanceTransformBufferDataPtr = InstanceBufferCopyDesc.Data[FBvInstanceBufferCopyDesc::Transform];

				uint32 InstanceLightMapBufferDataSize = InstanceBufferCopyDesc.Size[FBvInstanceBufferCopyDesc::LightMap];
				uint32 InstanceLightMapBufferDataStride = sizeof(int16) * 4;
				ensure(InstanceLightMapBufferDataSize / InstanceLightMapBufferDataStride == InstanceCount);
				uint8* InstanceLightMapBufferDataPtr = InstanceBufferCopyDesc.Data[FBvInstanceBufferCopyDesc::LightMap];

				uint32 InstanceEnlightenBufferDataSize = InstanceBufferCopyDesc.Size[FBvInstanceBufferCopyDesc::Enlighten];
				uint32 InstanceEnlightenBufferDataStride = sizeof(FVector4);
				ensure(InstanceEnlightenBufferDataSize / InstanceEnlightenBufferDataStride == InstanceCount);
				uint8* InstanceEnlightenBufferDataPtr = InstanceBufferCopyDesc.Data[FBvInstanceBufferCopyDesc::Enlighten];

				TArray<FBvTechArtEntityId>& SMCEntities = LODSMCEntities[LODIndex];
				ensure(SMCEntities.Num() == InstanceCount);

				// copy SMCEntities for this LOD
				SMCEntityIdsLOD = SMCEntities;

				for (int32 EntityIndex = 0; EntityIndex < SMCEntities.Num(); ++EntityIndex)
				{
					FBvTechArtEntityId SMCEntityId = SMCEntities[EntityIndex];

					const FBvTransform* TransformComponent = BvTransformView.GetComponent(SMCEntityId);
					const FTransform& TransformData = TransformComponent->WorldTransform;

					// get relative transform					
					FTransform RelativeTM = TransformData.GetRelativeTransform(HISMCTransform);
					FMatrix RelativeMatrix = RelativeTM.ToMatrixWithScale();

					// new origin data
					FVector4 Origin(RelativeMatrix.M[3][0], RelativeMatrix.M[3][1], RelativeMatrix.M[3][2], 0);

					// new instance-transform
					FVector4 InstanceTransform[3];
					InstanceTransform[0] = FVector4(RelativeMatrix.M[0][0], RelativeMatrix.M[0][1], RelativeMatrix.M[0][2], 0.0f);
					InstanceTransform[1] = FVector4(RelativeMatrix.M[1][0], RelativeMatrix.M[1][1], RelativeMatrix.M[1][2], 0.0f);
					InstanceTransform[2] = FVector4(RelativeMatrix.M[2][0], RelativeMatrix.M[2][1], RelativeMatrix.M[2][2], 0.0f);
					
					FFloat16 InstanceTransformHalfFloat[12];
					{
						InstanceTransformHalfFloat[0] = InstanceTransform[0][0];
						InstanceTransformHalfFloat[1] = InstanceTransform[0][1];
						InstanceTransformHalfFloat[2] = InstanceTransform[0][2];
						InstanceTransformHalfFloat[3] = InstanceTransform[0][3];

						InstanceTransformHalfFloat[4] = InstanceTransform[1][0];
						InstanceTransformHalfFloat[5] = InstanceTransform[1][1];
						InstanceTransformHalfFloat[6] = InstanceTransform[1][2];
						InstanceTransformHalfFloat[7] = InstanceTransform[1][3];

						InstanceTransformHalfFloat[8] = InstanceTransform[2][0];
						InstanceTransformHalfFloat[9] = InstanceTransform[2][1];
						InstanceTransformHalfFloat[10] = InstanceTransform[2][2];
						InstanceTransformHalfFloat[11] = InstanceTransform[2][3];
					}

					// new light map data (default)
					FVector4 InstanceLightMapData(0, 0, 0, 0);

					int16 InstanceLightMapDataVector[4];
					{
						InstanceLightMapDataVector[0] = FMath::Clamp<int32>(FMath::TruncToInt(InstanceLightMapData.X * 32767.0f), MIN_int16, MAX_int16);
						InstanceLightMapDataVector[1] = FMath::Clamp<int32>(FMath::TruncToInt(InstanceLightMapData.Y * 32767.0f), MIN_int16, MAX_int16);
						InstanceLightMapDataVector[2] = FMath::Clamp<int32>(FMath::TruncToInt(InstanceLightMapData.Z * 32767.0f), MIN_int16, MAX_int16);
						InstanceLightMapDataVector[3] = FMath::Clamp<int32>(FMath::TruncToInt(InstanceLightMapData.W * 32767.0f), MIN_int16, MAX_int16);
					}

					// default enlighten data (default)
					FVector4 InstanceEnlightenTransform(0, 0, 0, 0);

					// copy instance data
					ensure(InstanceOriginBufferDataStride * EntityIndex < InstanceOriginBufferDataSize);
					FPlatformMemory::Memcpy(InstanceOriginBufferDataPtr + InstanceOriginBufferDataStride * EntityIndex,
						&Origin, InstanceOriginBufferDataStride);

					ensure(InstanceTransformBufferDataStride * EntityIndex < InstanceTransformBufferDataSize);
					FPlatformMemory::Memcpy(InstanceTransformBufferDataPtr + InstanceTransformBufferDataStride * EntityIndex,
						&InstanceTransformHalfFloat, InstanceTransformBufferDataStride);

					ensure(InstanceLightMapBufferDataStride * EntityIndex < InstanceLightMapBufferDataSize);
					FPlatformMemory::Memcpy(InstanceLightMapBufferDataPtr + InstanceLightMapBufferDataStride * EntityIndex,
						&InstanceLightMapDataVector, InstanceLightMapBufferDataStride);

					// NOTE that this is just temporary operation.... (need to fix) @TODO
					ensure(InstanceEnlightenBufferDataStride* EntityIndex < InstanceEnlightenBufferDataSize);
					FPlatformMemory::Memcpy(InstanceEnlightenBufferDataPtr + InstanceEnlightenBufferDataStride * EntityIndex,
						&InstanceLightMapData, InstanceEnlightenBufferDataStride);
				}
			}

			// update FBvInstanceBufferInfo
			FBvTechArtInstancerEntityDB::Get().QueueToUpdate(InstancerEntity, MoveTemp(InstanceBufferInfoToUpdate));

			// update FBvSMCEntities
			FBvTechArtInstancerEntityDB::Get().QueueToUpdate(InstancerEntity, MoveTemp(NewSMCEntities));
		}
	}
}
BV_DEBUG_NO_OPTIMIZE_END

/**
 * FBvLODActorUtils
 */

void FBvLODActorUtils::UpdateBvLODActors(UWorld* InWorld)
{
	// clear CachedBvLODActors
	CachedBvLODActors.Reset();

	// re-cache BvLODActors
	TArray<ULevel*> Levels = InWorld->GetLevels();
	for (ULevel* Level : Levels)
	{
		if (Level->IsPersistentLevel())
		{
			continue;
		}

		for (AActor* Actor : Level->Actors)
		{
			if (ABvLODActor* CurrBvLODActor = Cast<ABvLODActor>(Actor))
			{
				CachedBvLODActors.Add(CurrBvLODActor);
			}
		}
	}

	BV_DEBUG_VALUE(true, CachedBvLODActors, BvLODActorDebugValuePtr0);
}

static volatile bool EnableDebugBvLODActorByPrimitiveSceneInfo = false;

void FBvLODActorUtils::DebugBvLODActorByPrimitiveSceneInfo(class FPrimitiveSceneInfo* InPrimitiveSceneInfo)
{
	ensure(IsInRenderingThread());

	// to catch EnableDebugBvLODActorByPrimitiveSceneInfo variable, use BV_DEBUG_VALUE
	BV_DEBUG_VALUE(true, EnableDebugBvLODActorByPrimitiveSceneInfo, BvLODActorDebugValuePtr0);

	// we didn't enable DebugBvLODActorByPrimitiveSceneInfo, just skip
	if (!EnableDebugBvLODActorByPrimitiveSceneInfo)
	{
		return;
	}

	// update LODActors first
	UpdateBvLODActors(GWorld);

	// generate debug data
	//...
}

void FBvLODActorUtils::ExtractSMCsFromBvLODActor(ABvLODActor* InBvLODActor, TArray<UStaticMeshComponent*>& OutSMCs)
{
	for (AActor* ChildActor : InBvLODActor->SubActors)
	{
		if (ChildActor)
		{
			TArray<UStaticMeshComponent*> ChildComponents;
			ABvLODActor* CurrBvLODActor = Cast<ABvLODActor>(ChildActor);
			if (CurrBvLODActor)
			{
				ExtractSMCsFromBvLODActor(CurrBvLODActor, ChildComponents);
			}
			else
			{
				ChildActor->GetComponents<UStaticMeshComponent>(ChildComponents);
			}

			OutSMCs.Append(ChildComponents);
		}
	}
}

void FBvLODActorUtils::FilterSMCsForPossibleISMCs(ABvLODActor* InBvLODActor, TArray<UStaticMeshComponent*>& OutSMCs)
{
	//*** collect SMCs
	TArray<UStaticMeshComponent*> CollectedSMCs;
	{
		ExtractSMCsFromBvLODActor(InBvLODActor, CollectedSMCs);
	}
	BV_DEBUG_VALUE(true, CollectedSMCs, BvLODActorDebugValuePtr0);

	//*** filter SMCs whether each SMC has InstanceVertexColors or not
	TArray<UStaticMeshComponent*> FilteredSMCs;
	{
		// create the lambda for HasInstanceVertexColors
		auto HasInstanceVertexColors = [](UStaticMeshComponent* InSMC)
		{
			for (auto& LODInfo : InSMC->LODData)
			{
				if (LODInfo.OverrideVertexColors != nullptr || LODInfo.PaintedVertices.Num() > 0)
				{
					return true;
				}
			}
			return false;
		};

		// filter SMCs from CollectedSMCs by HasInstanceVertexColors
		for (UStaticMeshComponent* SMC : CollectedSMCs)
		{
			if (!HasInstanceVertexColors(SMC))
			{
				OutSMCs.Add(SMC);
			}
		}
	}
	BV_DEBUG_VALUE(true, FilteredSMCs, BvLODActorDebugValuePtr0);
}

void ABvLODActor::CacheDummySM()
{
	if (DummySM == nullptr)
	{
		FSoftObjectPath AssetPathForDummySM = FBvTechArtAssetManager::Get().GetAssetPath(TEXT("BvLODActorDummySM"));
		DummySM = Cast<UStaticMesh>(AssetPathForDummySM.TryLoad());

		// cache dummy SM
		DummySM->AddToRoot();
	}	
}

ABvLODActor::ABvLODActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void ABvLODActor::SetComponentsMinDrawDistance(float InMinDrawDistance, bool bInMarkRenderStateDirty)
{
	Super::SetComponentsMinDrawDistance(InMinDrawDistance, bInMarkRenderStateDirty);
}

void ABvLODActor::RegisterMeshComponents()
{
	Super::RegisterMeshComponents();
}

void ABvLODActor::UnregisterMeshComponents()
{
	Super::UnregisterMeshComponents();
}

FBox ABvLODActor::GetComponentsBoundingBox(bool bNonColliding, bool bIncludeFromChildActors) const
{
	return Super::GetComponentsBoundingBox(bNonColliding, bIncludeFromChildActors);
}

void ABvLODActor::SetLODParent(UPrimitiveComponent* InLODParent, float InParentDrawDistance, bool bInApplyToImposters)
{
	Super::SetLODParent(InLODParent, InParentDrawDistance, bInApplyToImposters);
}

#if WITH_EDITOR
void ABvLODActor::AddSubActor(AActor* InActor)
{
	Super::AddSubActor(InActor);
}

const bool ABvLODActor::RemoveSubActor(AActor* InActor)
{
	return Super::RemoveSubActor(InActor);
}

void ABvLODActor::SetHiddenFromEditorView(const bool InState, const int32 ForceLODLevel)
{
	Super::SetHiddenFromEditorView(InState, ForceLODLevel);
}

void ABvLODActor::RecalculateDrawingDistance(const float InTransitionScreenSize)
{
	Super::RecalculateDrawingDistance(InTransitionScreenSize);
}

void ABvLODActor::UpdateSubActorLODParents()
{
	Super::UpdateSubActorLODParents();
}

void ABvLODActor::DetermineShadowingFlags()
{
	Super::DetermineShadowingFlags();
}
#endif

void ABvLODActor::PostLoad()
{
	Super::PostLoad();
}

void ABvLODActor::SetDummyStaticMesh()
{
	CacheDummySM();
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(DummySM);
	}
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
const bool ABvLODActor::IsBuilt(bool bInForce) const
{
	return Super::IsBuilt(bInForce);
}
#endif

void BvTest_BvLODActor(UWorld* InWorld, bool Next)
{
	
}

FAutoConsoleCommandWithWorld ExecuteBvTest_BvLODActor(
	TEXT("BvTest_BvLODActor"),
	TEXT("BvTest_BvLODActor"),
	FConsoleCommandWithWorldDelegate::CreateStatic(BvTest_BvLODActor, true)
);


#undef LOCTEXT_NAMESPACE
