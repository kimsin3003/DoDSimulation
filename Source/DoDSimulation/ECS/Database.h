
#pragma once
#include <stdint.h>
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Templates/Tuple.h"
#include "UObject/Object.h"
#include "Component.h"


class UWorld;
class AActor;

template<typename... CompType>
struct DataUnit
{
	int64_t EntityId = -1;
	TTuple<CompType...> Comps;
};


struct IArcheType
{
	TSet<FString> Key;
	void* Datas = nullptr;


}

template<typename... CompType>
struct ArcheType
{
	ArcheType() {
		Datas = new TArray<DataUnit<CompType...>>();
	}
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

	const TArray<IArcheType*>& GetArcheTypes() const { return _archeTypes; }

private:
	UWorld* _world = nullptr;
	int64_t _nextEntity = 0;
	TArray<IArcheType*> _archeTypes;
};