// Fill out your copyright notice in the Description page of Project Settings.


#include "ECSManager.h"
#include "TestSystem.h"
#include "ECS/Database.h"

// Sets default values
AECSManager::AECSManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AECSManager::BeginPlay()
{
	Super::BeginPlay();
	Scheduler.DB = new Database();
	Systems.Add(new TestSystem());
}

// Called every frame
void AECSManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bEnableTick) return;

	for (auto& system : Systems)
	{
		system->Update(DeltaTime, Scheduler);
	}
}

