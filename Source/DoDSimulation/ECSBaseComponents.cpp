// Fill out your copyright notice in the Description page of Project Settings.


#include "ECSBaseComponents.h"
#include "ECSManager.h"
#include "EngineUtils.h"

// Sets default values for this component's properties
UECSLinker::UECSLinker()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UECSLinker::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetOwner()->GetWorld();

	// We do nothing if no is class provided, rather than giving ALL actors!
	if (World)
	{
		for (TActorIterator<AECSManager> It(World, AECSManager::StaticClass()); It; ++It)
		{
			if (IsValid(*It))
			{
				ECSManager = *It;
				break;
			}
		}
	}

	if (ECSManager == nullptr)
	{
		ECSManager = World->SpawnActor<AECSManager>(AECSManager::StaticClass());
	}
	if (ECSManager)
	{
		EntityId = ECSManager->Scheduler.DB->CreateEntity();
	}
	RegisterWithECS();
}

void UECSLinker::RegisterWithECS()
{
	Database* DB = ECSManager->Scheduler.DB;
	DB->AddComp(EntityId, FActorReference(GetOwner()));

	if (TransformSync == ETransformSyncType::Actor_To_ECS || TransformSync == ETransformSyncType::BothWays)
	{
		DB->AddComp(EntityId, CopyToECS);
	}
	if (TransformSync == ETransformSyncType::ECS_To_Actor || TransformSync == ETransformSyncType::BothWays)
	{
		DB->AddComp(EntityId, CopyToActor);
	}

	//request all other components to add their ECS component to this actor
	for (auto c : GetOwner()->GetComponents())
	{
		auto wr = Cast<IComponentWrapper>(c);
		if (wr)
		{
			wr->AddToEntity(DB, EntityId);
		}
	}

}
