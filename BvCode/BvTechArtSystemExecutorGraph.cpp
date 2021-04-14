// Copyright Krafton. Beaver Lab. All Rights Reserved.

#include "BvTechArtSystemExecutorGraph.h"
#include "BvDebugHelper.h"
#include "BvEntityComponentSystem.h"
#include "BvScopeTestDuplicate.h"
#include "BvLODActor.h"
#include "BvEngineDebuggerInterface.h"
#include "BvTechArtDB.h"
#include "BvTechArtFrameSync.h"

#define LOCTEXT_NAMESPACE "BvTechArtSystemExecutorGraph"
DEFINE_LOG_CATEGORY(BvTechArtSystemExecutorGraph)

/////////////////////////////////////////////////////
// FBvTechArtSystemExecutorGraph

BV_DECLARE_DEBUG_VARIABLE(void, BvTechArtSystemExecutorGraphDebugValuePtr0);

void FBvTechArtSystemExecutorGraph::BindDelegates()
{
	FBvTechArtSystemTicker::Get().ExecuteSystemsByGraph.BindStatic(&FBvTechArtSystemExecutorGraph::ExecuteSystems);
}

void FBvTechArtSystemExecutorGraph::ExecuteSystems(FBvTechArtSystemTicker& InSystemTicker)
{
	using namespace BvSMCEntityInBvLODActorEntity;

	// sync barrier (SystemExecutor_RenderThread)
	FBvTechArtScopeSyncBarrier SyncBarrier(FBvTechArtFrameSync::Get().GetSyncObject(EBvSyncBarrierType::SystemExecutor_RenderThread));	

	BV_CPUPROFILER_EVENT_SCOPE(BvTechArtSystemExecutorGraph, EBvCpuProfilerGroup::TechArtECS);

	// dummy context
	FBvTechArtSystemContext DefaultContext;

	// system duplicate checking
	FBvScopeTestDuplicate<FBvTechArtSystemExecutor> ScopeDuplicateTest;

	// EXECUTE TASK-GRAPH 
	{
		BV_CPUPROFILER_EVENT_SCOPE(BvProcessPendingEntitiesToAddMeshInstancingKeys, EBvCpuProfilerGroup::TechArtECS);
		FBvProcessPendingEntitiesToAddMeshInstancingKeys::Get().Execute(DefaultContext);
		ScopeDuplicateTest.AddElement(&FBvProcessPendingEntitiesToAddMeshInstancingKeys::Get());
	}	

	// declare the context BvCullingSystem and BvHISMCUpdater are going to use
	BvSMCEntityInBvLODActorEntity::FBvSMCEntityContext SMCEntityContext;
	{
		{
			BV_CPUPROFILER_EVENT_SCOPE(BvCullingSystem, EBvCpuProfilerGroup::TechArtECS);
			FBvCullingSystem::Get().Execute(SMCEntityContext);
			ScopeDuplicateTest.AddElement(&BvSMCEntityInBvLODActorEntity::FBvCullingSystem::Get());
		}

		{
			BV_CPUPROFILER_EVENT_SCOPE(BvHISMCUpdater, EBvCpuProfilerGroup::TechArtECS);
			FBvHISMCUpdater::Get().Execute(SMCEntityContext);
			ScopeDuplicateTest.AddElement(&BvSMCEntityInBvLODActorEntity::FBvHISMCUpdater::Get());
		}

		if (FBvTechArtGlobal::EnableGatherInstancing())
		{
			BV_CPUPROFILER_EVENT_SCOPE(BvInstanceDataGenerator, EBvCpuProfilerGroup::TechArtECS);
			FBvInstanceDataGenerator::Get().Execute(SMCEntityContext);
			ScopeDuplicateTest.AddElement(&BvSMCEntityInBvLODActorEntity::FBvInstanceDataGenerator::Get());
		}
	}	

	// by default, execute linearly	
	for (FBvTechArtSystemExecutor* SystemExecutor : InSystemTicker.SystemExecutors)
	{
		// only execute systems that we didn't execute
		if (!ScopeDuplicateTest.IsElementExists(SystemExecutor))
		{
			SystemExecutor->Execute(DefaultContext);
		}
	}

	// update TechArtDB
	{
		BV_CPUPROFILER_EVENT_SCOPE(FBvTechArtDBDispatcher, EBvCpuProfilerGroup::TechArtECS);
		FBvTechArtDBDispatcher::Get().Tick(0.0f);
	}
}

#undef LOCTEXT_NAMESPACE