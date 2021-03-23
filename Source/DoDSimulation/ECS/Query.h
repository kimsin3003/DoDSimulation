#pragma once
#include <array>
#include <string>
#include <functional>
#include <typeinfo>
#include "Database.h"
#include "../EnTT/entt.hpp"

template<typename... ComponentType>
class Query
{
public:
	void Each(const Database& database, const TFunction<void(ComponentType...)>& func) const
	{
		entt::registry registry;
		auto view = registry.view<const position, velocity>();
		view.each([](const auto& pos, auto& vel) { /* ... */ });
		TSet<FString> types = { (typeid(ComponentType).name())... };

		const TArray<ArcheType*>& archeTypes = database.GetArcheTypes();
		for (ArcheType* archeType : archeTypes)
		{
			if (archeType->Key.Includes(types))
			{
				for (DataUnit& data : archeType->Datas)
				{
					func(data.Comps ...);
				}
			}
		}
	}
};
