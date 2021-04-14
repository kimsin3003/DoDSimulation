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
	Scheduler.DB = new Database(GetWorld());
	Systems.Add(new TestSystem());

	int size = 10;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			AActor* M = GetWorld()->SpawnActor<AActor>(Mover, 100 * FVector(i - size / 2, j - size / 2, 0), FRotator(0, 0, 0));
			AActor* D = GetWorld()->SpawnActor<AActor>(Door, 100 * FVector(i - size / 2, j - size / 2, 0), FRotator(0, 0, 0));
		
			Scheduler.DB->AddActor(M);
			Scheduler.DB->AddActor(D);
		}
	}
}

// Called every frame
void AECSManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (auto& system : Systems)
	{
		system->Update(DeltaTime, Scheduler);
	}
}

