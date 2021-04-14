// Copyright Krafton. Beaver Lab. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(BvTechArtSystemExecutorGraph, All, All)

class FBvTechArtSystemExecutorGraph
{
public:	
	/** bind delegates */
	static void BindDelegates();

	/** task graph for executing systems */
	static void ExecuteSystems(class FBvTechArtSystemTicker& InSystemTicker);
};
