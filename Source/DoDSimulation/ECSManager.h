// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ECS/Scheduler.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ECSManager.generated.h"

UCLASS()
class DODSIMULATION_API AECSManager : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	AECSManager();
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool bEnableTick;

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
