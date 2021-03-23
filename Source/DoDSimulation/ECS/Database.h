
#pragma once
#include <stdint.h>
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Templates/Tuple.h"
#include "UObject/Object.h"
#include "Component.h"


class UWorld;
class AActor;

struct DataUnit
{
	int64_t EntityId = -1;
	TArray<Component> Comps;
};

struct ArcheType
{
	TSet<FString> Key;
	TArray<DataUnit>* Datas = new TArray<DataUnit>();
};

class Database
{
public:
	Database(UWorld* world);
	int AddActor(AActor* actor);

//	template<typename... ComponentType>
// 	TArray<TTuple<ComponentType...>> ForEach() const
// 	{
// 		TArray<DataUnit*> result;
// 		TSet<FString> types = { (typeid(ComponentType).name())... };
// 
// 		TArray<TSet<FString>> keys;
// 		_data.GetKeys(keys);
// 		for (auto archeType : _archeTypes)
// 		{
// 			if(archeType.Key.Includes(types))
// 				result.Append(archeType.Datas);
// 		}
// 
// 		return result;
// 	}

	const TArray<ArcheType*>& GetArcheTypes() const { return _archeTypes; }

private:
	UWorld* _world = nullptr;
	int64_t _nextEntity = 0;
	TArray<ArcheType*> _archeTypes;
};