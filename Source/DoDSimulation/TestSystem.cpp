#include "TestSystem.h"
#include <functional>
#include <stdio.h>
#include "ECSBaseComponents.h"

#pragma optimize( "", off )
void TestSystem::Update(float deltaTime, const Scheduler& scheduler)
{
	Database* DB = scheduler.DB;

	Query<TExclude<FActorTransform>, FCopyTransformToECS> ToAddTransform;
	ToAddTransform.Each(*DB, [=](int32 EntityId, FCopyTransformToECS& _)
	{
		DB->AddComp(EntityId, FActorTransform());
		return true;
	});

	Query<FActorReference, FActorTransform> ToCopyTransform;
	ToCopyTransform.Each(*DB, [=](int32 EntityId, const FActorReference& v, FActorTransform& Transform)
	{
		const FTransform& ActorTransform = v.Ptr->GetActorTransform();
		Transform.transform = ActorTransform;
		return true;
	});

	Query<FMoverComp, FActorReference, FActorTransform> MoveMover;
	MoveMover.Each(*DB, [=](int32 EntityId, FMoverComp& _, const FActorReference& v, FActorTransform& Transform)
	{
		const auto& location = Transform.transform.GetLocation();
		FVector NewPos = { location + 1000 * deltaTime * FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f)) };
		Transform.transform.SetLocation(NewPos);
		return true;
	});

	Query<FDoorComp, FActorTransform> ToCheckDoorOpen;
	ToCheckDoorOpen.Each(*DB, [=](int32 EntityId, FDoorComp& DoorComp, FActorTransform& Transform)
	{
		auto doorLocation = Transform.transform.GetLocation();
		DoorComp.IsOpen = false;
		FDoorComp* DoorPtr = &DoorComp;
		Query<FMoverComp, FActorTransform> ForEachMover;
		ForEachMover.Each(*DB, [=](int32 EntityId, FMoverComp& Mover, FActorTransform& MoverTransform)
		{
			auto moverLocation = MoverTransform.transform.GetLocation();
			if (FVector::Distance(doorLocation, moverLocation) <= 100)
			{
				DoorPtr->IsOpen = true;
				return false;
			}
			return true;
		});
	});

	Query<FDoorComp, FActorReference, FActorTransform> ToCopyDoorCompToDoor;
	ToCopyDoorCompToDoor.Each(*DB, [=](int32 EntityId, FDoorComp& DoorComp, FActorReference& ActorRef, FActorTransform& Transform)
	{
		ADoor* Door = Cast<ADoor>(ActorRef.Ptr);
		Door->_IsOpen = DoorComp.IsOpen;
		return true;
	});

	Query<FCopyTransformToActor, FActorReference, FActorTransform> ToCopyTransformToActor;
	ToCopyTransformToActor.Each(*DB, [=](int32 EntityId, FCopyTransformToActor& _, FActorReference& v, FActorTransform& Transform)
	{
		v.Ptr->SetActorTransform(Transform.transform);
		return true;
	});
}
#pragma optimize( "", on )

