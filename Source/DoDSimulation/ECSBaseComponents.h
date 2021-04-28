// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentWrapper.h"
#include "ECS/Database.h"
#include "ECSBaseComponents.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDoorComp {
	GENERATED_BODY()
		FDoorComp() {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool IsOpen = false;
};
USTRUCT(BlueprintType)
struct FMoverComp {
	GENERATED_BODY()
		FMoverComp() {}
};

USTRUCT(BlueprintType)
struct FActorReference {
	GENERATED_BODY()
		FActorReference(AActor* Owner = nullptr) { Ptr = Owner; }

	TWeakObjectPtr<AActor> Ptr;

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
struct FCopyTransformToECS {
	GENERATED_BODY()
};
USTRUCT(BlueprintType)
struct FCopyTransformToActor {
	GENERATED_BODY()
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
	class AECSManager* ECSManager = nullptr;

	UPROPERTY(EditAnywhere, Category = "ECS")
		ETransformSyncType TransformSync = ETransformSyncType::BothWays;

};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class DODSIMULATION_API UDoorWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UDoorWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDoorComp Value;
protected:
	void AddToEntity(Database* DB, int32 EntityId) final
	{
		DB->AddComp(EntityId, Value);
	}
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class DODSIMULATION_API UMoverWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMoverWrapper() { PrimaryComponentTick.bCanEverTick = false; };

protected:
	void AddToEntity(Database* DB, int32 EntityId) final
	{
		DB->AddComp(EntityId, FMoverComp());
	}
};