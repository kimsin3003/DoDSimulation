// Fill out your copyright notice in the Description page of Project Settings.


#include "Mover.h"
#include "Math/UnrealMathUtility.h"

// Sets default values for this component's properties
AMover::AMover()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void AMover::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void AMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!NeedUpdate) return;
	UE_LOG(LogTemp, Log, TEXT("Log Message"));
	const auto& location = GetActorLocation();
	SetActorLocation(location + 100 * DeltaTime * FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f)));
}

