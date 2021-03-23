// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Mover.h"
#include "EngineUtils.h"
#include "ECS/Scheduler.h"
#include "ECS/Database.h"
#include "ECS/Query.h"

// Sets default values for this component's properties
USpawner::USpawner()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USpawner::BeginPlay()
{
	Super::BeginPlay();

    int size = 10;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            GetWorld()->SpawnActor<AActor>(Mover, 100 * FVector(i - size/ 2, j - size / 2, 0), FRotator(0,0,0));
            GetWorld()->SpawnActor<AActor>(Door, 100 * FVector(i - size / 2, j - size / 2, 0), FRotator(0, 0, 0));
        }
    }
}


// Called every frame
void USpawner::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

