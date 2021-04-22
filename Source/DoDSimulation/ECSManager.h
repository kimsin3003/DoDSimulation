// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Spawner.h"
#include "Mover.h"
#include "ECS/Scheduler.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ECSManager.generated.h"

UCLASS()
class DODSIMULATION_API AECSManager : public AActor
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> Mover;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> Door;
	
public:	
	// Sets default values for this actor's properties
	AECSManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	Scheduler Scheduler;
private:
	TArray<class SystemBase*> Systems;
};
