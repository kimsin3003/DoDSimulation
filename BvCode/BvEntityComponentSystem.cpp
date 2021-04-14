// Copyright Krafton. Beaver Lab. All Rights Reserved.

#include "BvEntityComponentSystem.h"
#include "BvDebugHelper.h"

#define LOCTEXT_NAMESPACE "BvEntityComponentSystem"
DEFINE_LOG_CATEGORY(BvEntityComponentSystem)

/////////////////////////////////////////////////////
// FBvEntityComponentSystem

BV_DECLARE_DEBUG_VARIABLE(void, BvEntityComponentSystemDebugValuePtr0);

FBvTechArtSystemExecutor::FBvTechArtSystemExecutor()
{
	FBvTechArtSystemTicker::Get().AddSystem(this);
}

void BvTest_BvEntityComponentSystem(UWorld* InWorld, bool Next)
{
	struct FTestComponent
	{
		DECLARE_COMPONENT_TYPE_ID(FTestComponent);
	};

	struct FTestComponent1
	{
		DECLARE_COMPONENT_TYPE_ID(FTestComponent1);
	};

	// test registry
	FBvTechArtBasicRegistry<uint32> Registry;
	{
		// create multiple entity ids
		const int32 EntityCount = 10;
		for (int32 EntityIndex = 0; EntityIndex < EntityCount; ++EntityIndex)
		{
			uint32 EntityId = Registry.Create();

			// add FTestComponent
			Registry.Emplace<FTestComponent>(EntityId);

			// add FTestComponent1 in even index
			if (EntityIndex % 2 == 0)
			{
				Registry.Emplace<FTestComponent1>(EntityId);
			}			
		}

		// get FTestComponent
		auto GetTestComponent = Registry.Assure<FTestComponent>();
		BV_DEBUG_VALUE(true, GetTestComponent, BvEntityComponentSystemDebugValuePtr0);

		// get the view from the registry
		auto GetView = Registry.View<FTestComponent, FTestComponent1>();
		BV_DEBUG_VALUE(true, GetView, BvEntityComponentSystemDebugValuePtr0);

		// looping entity ids by View
		{
			TArray<FTestComponent*> TestComponents;
			TArray<FTestComponent1*> TestComponent1s;

			GetView.Each([&TestComponent1s, &TestComponents](uint32 EntityId, FTestComponent& TestComponent, FTestComponent1& TestComponent1)
				{
					TestComponents.Add(&TestComponent);
					TestComponent1s.Add(&TestComponent1);
				});
			BV_DEBUG_VALUE(true, TestComponents, BvEntityComponentSystemDebugValuePtr0);
			BV_DEBUG_VALUE(true, TestComponent1s, BvEntityComponentSystemDebugValuePtr0);
		}		

		// get the view by exclude component
		auto GetViewByExclude = Registry.View<FTestComponent>(TBvTechArtExclude<FTestComponent1>());
		{
			TArray<FTestComponent*> TestComponents;
			GetViewByExclude.Each([&TestComponents](uint32 EntityId, FTestComponent& TestComponent)
				{
					TestComponents.Add(&TestComponent);
				});
			BV_DEBUG_VALUE(true, TestComponents, BvEntityComponentSystemDebugValuePtr0);
		}
	}
}

FAutoConsoleCommandWithWorld ExecuteBvTest_BvEntityComponentSystem(
	TEXT("BvTest_BvEntityComponentSystem"),
	TEXT("BvTest_BvEntityComponentSystem"),
	FConsoleCommandWithWorldDelegate::CreateStatic(BvTest_BvEntityComponentSystem, true)
);


#undef LOCTEXT_NAMESPACE