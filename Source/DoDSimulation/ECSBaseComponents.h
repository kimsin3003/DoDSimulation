// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentWrapper.h"
#include "ECSBaseComponents.generated.h"


/**
 * 
 */

USTRUCT(BlueprintType)
struct FActorReference {
	GENERATED_BODY()
		FActorReference(AActor* Owner = nullptr) { Ptr = Owner; }

	TWeakObjectPtr<AActor> Ptr;

};
USTRUCT(BlueprintType)
struct FCopyTransformToECS {
	GENERATED_BODY()
		FCopyTransformToECS() : bWorldSpace(true) {};

	UPROPERTY(EditAnywhere, Category = ECS)
		bool bWorldSpace;
};
USTRUCT(BlueprintType)
struct FActorTransform {
	GENERATED_BODY()
		FActorTransform() {};
	FActorTransform(FTransform trns) : transform(trns) {};

	UPROPERTY(EditAnywhere, Category = ECS)
		FTransform transform;
};
USTRUCT(BlueprintType)
struct FCopyTransformToActor {
	GENERATED_BODY()

		FCopyTransformToActor() : bWorldSpace(true), bSweep(false) {};

	UPROPERTY(EditAnywhere, Category = ECS)
		bool bWorldSpace;
	UPROPERTY(EditAnywhere, Category = ECS)
		bool bSweep;
};


UENUM(BlueprintType)
enum class ETransformSyncType : uint8 {
	ECS_To_Actor,
	Actor_To_ECS,
	BothWays,
	Disabled

};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class DODSIMULATION_API UECSLinker : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UECSLinker();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void RegisterWithECS();
	int32 EntityId = -1;
	UPROPERTY(EditAnywhere, Category = "ECS")
		ETransformSyncType TransformSync;
	UPROPERTY(EditAnywhere, Category = "ECS")
		FCopyTransformToActor CopyToActor;
	UPROPERTY(EditAnywhere, Category = "ECS")
		FCopyTransformToECS CopyToECS;
	class AECSManager* ECSManager = nullptr;

};