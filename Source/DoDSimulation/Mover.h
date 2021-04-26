// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mover.generated.h"

UCLASS()
class DODSIMULATION_API AMover : public AActor
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere)
		bool NeedUpdate;

public:	
	// Sets default values for this component's properties
	AMover();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
		
};
