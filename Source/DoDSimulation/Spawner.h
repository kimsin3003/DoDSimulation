// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Spawner.h"
#include "Mover.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spawner.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DODSIMULATION_API USpawner : public UActorComponent
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> Mover;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> Door;
	UPROPERTY(EditAnywhere)
		int32 PoolSize;

public:	
	// Sets default values for this component's properties
	USpawner();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
