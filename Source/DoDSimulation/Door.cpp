// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Mover.h"
#include "EngineUtils.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto doorLocation = GetActorLocation();
	_IsOpen = false;
	for (const AMover* mover : TActorRange<AMover>(GetWorld()))
	{
		auto moverLocation = mover->GetActorLocation();
		if (FVector::Distance(doorLocation, moverLocation) <= 180)
		{
			_IsOpen = true;
			return;
		}
	}
}

