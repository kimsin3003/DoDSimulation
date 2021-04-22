#include "TestSystem.h"
#include <functional>
#include <stdio.h>
#include "ECSBaseComponents.h"

void TestSystem::Update(float deltaTime, const Scheduler& scheduler)
{
	Database* DB = scheduler.DB;

	Query<TExclude<FActorTransform>, FCopyTransformToECS> ToAddTransform;
	ToAddTransform.Each(*DB, [=](int32 EntityId, FCopyTransformToECS& _)
	{
		DB->AddComp(EntityId, FActorTransform());
	});

	Query<FCopyTransformToECS, FActorReference, FActorTransform> ToCopyTransform;
	ToCopyTransform.Each(*DB, [=](int32 EntityId, FCopyTransformToECS& _, const FActorReference& v, FActorTransform& Transform)
	{
		const FTransform& ActorTransform = v.Ptr->GetActorTransform();
		Transform.transform = ActorTransform;

		const auto& location = Transform.transform.GetLocation();
		FVector NewPos = { location + 100 * deltaTime * FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f)) };
		Transform.transform.TransformPosition(NewPos);
	});

	Query<FCopyTransformToActor, FActorReference, FActorTransform> ToCopyTransformToActor;
	ToCopyTransformToActor.Each(*DB, [=](int32 EntityId, FCopyTransformToActor& _, FActorReference& v, FActorTransform& Transform)
	{
		v.Ptr->SetActorTransform(Transform.transform);
	});
}

