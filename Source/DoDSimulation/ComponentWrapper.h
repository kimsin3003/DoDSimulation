// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentWrapper.generated.h"

UINTERFACE()
class UComponentWrapper : public UInterface {
	GENERATED_BODY()
};

class IComponentWrapper {

	GENERATED_BODY()
public:
	virtual void AddToEntity(class Database* DB, int32 EntityId) = 0;
};
