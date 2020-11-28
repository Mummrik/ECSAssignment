#pragma once
#include <iostream>
#include <bitset>
#include <queue>
#include <cassert>
#include <array>
#include <unordered_map>
#include <set>

#include "Components.h"

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 5000;
using ComponentType = std::uint8_t;
const ComponentType MAX_COMPONENTS = 500;
using Signature = std::bitset<MAX_COMPONENTS>;

class EntityManager
{
public:
	EntityManager()
	{
		for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
		{
			m_AvailableEntities.push(entity);
		}
	}

	Entity CreateEntity()
	{
		assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

		Entity id = m_AvailableEntities.front();
		m_AvailableEntities.pop();
		++m_LivingEntityCount;

		return id;
	}

	void DestroyEntity(Entity entity)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		m_Signatures[entity].reset();

		m_AvailableEntities.push(entity);
		--m_LivingEntityCount;
	}

	void SetSignature(Entity entity, Signature signature)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		m_Signatures[entity] = signature;
	}

	Signature GetSignature(Entity entity)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		return m_Signatures[entity];
	}

private:
	std::queue<Entity> m_AvailableEntities{};

	std::array<Signature, MAX_ENTITIES> m_Signatures{};

	uint32_t m_LivingEntityCount{};
};

class IComponentArray
{
public:
	virtual ~IComponentArray() = default;
	virtual void EntityDestroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray
{
public:
	void InsertData(Entity entity, T component)
	{
		assert(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end() && "Component added to same entity more than once.");

		size_t newIndex = m_Size;
		m_EntityToIndexMap[entity] = newIndex;
		m_IndexToEntityMap[newIndex] = entity;
		m_ComponentArray[newIndex] = component;
		++m_Size;
	}

	void RemoveData(Entity entity)
	{
		assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Removing non-existent component.");

		size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
		size_t indexOfLastElement = m_Size - 1;
		m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];

		Entity entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
		m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
		m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

		m_EntityToIndexMap.erase(entity);
		m_IndexToEntityMap.erase(indexOfLastElement);

		--m_Size;
	}

	T& GetData(Entity entity)
	{
		assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Retrieving non-existent component.");

		return m_ComponentArray[m_EntityToIndexMap[entity]];
	}

	void EntityDestroyed(Entity entity) override
	{
		if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end())
		{
			RemoveData(entity);
		}
	}

private:
	std::array<T, MAX_ENTITIES> m_ComponentArray;

	std::unordered_map<Entity, size_t> m_EntityToIndexMap;

	std::unordered_map<size_t, Entity> m_IndexToEntityMap;

	size_t m_Size;
};

class ComponentManager
{
public:
	template<typename T>
	void RegisterComponent()
	{
		const char* typeName = typeid(T).name();

		assert(m_ComponentTypes.find(typeName) == m_ComponentTypes.end() && "Registering component type more than once.");

		m_ComponentTypes.insert({ typeName, m_NextComponentType });

		m_ComponentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

		++m_NextComponentType;
	}

	template<typename T>
	ComponentType GetComponentType()
	{

		const char* typeName = typeid(T).name();

		assert(m_ComponentTypes.find(typeName) != m_ComponentTypes.end() && "Component not registered before use.");

		return m_ComponentTypes[typeName];
	}

	template<typename T>
	void AddComponent(Entity entity, T component)
	{
		GetComponentArray<T>()->InsertData(entity, component);
	}

	template<typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>()->RemoveData(entity);
	}

	template<typename T>
	T& GetComponent(Entity entity)
	{
		return GetComponentArray<T>()->GetData(entity);
	}

	void EntityDestroyed(Entity entity)
	{
		for (auto const& pair : m_ComponentArrays)
		{
			auto const& component = pair.second;

			component->EntityDestroyed(entity);
		}
	}

private:
	std::unordered_map<const char*, ComponentType> m_ComponentTypes{};

	std::unordered_map<const char*, std::shared_ptr<IComponentArray>> m_ComponentArrays{};

	ComponentType m_NextComponentType{};

	template<typename T>
	std::shared_ptr<ComponentArray<T>> GetComponentArray()
	{
		const char* typeName = typeid(T).name();

		assert(m_ComponentTypes.find(typeName) != m_ComponentTypes.end() && "Component not registered before use.");

		return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
	}
};

class System
{
public:
	std::set<Entity> m_Entities;

	Entity GetEntity(uint32_t index)
	{
		for (auto const& entity : m_Entities)
			if (entity == index)
				return entity;
	}
};

class SystemManager
{
public:
	template<typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		const char* typeName = typeid(T).name();

		assert(m_Systems.find(typeName) == m_Systems.end() && "Registering system more than once.");

		auto system = std::make_shared<T>();
		m_Systems.insert({ typeName, system });
		return system;
	}

	template<typename T>
	void SetSignature(Signature signature)
	{
		const char* typeName = typeid(T).name();

		assert(m_Systems.find(typeName) != m_Systems.end() && "System used before registered.");

		m_Signatures.insert({ typeName, signature });
	}

	void EntityDestroyed(Entity entity)
	{
		for (auto const& pair : m_Systems)
		{
			auto const& system = pair.second;

			system->m_Entities.erase(entity);
		}
	}

	void EntitySignatureChanged(Entity entity, Signature entitySignature)
	{
		for (auto const& pair : m_Systems)
		{
			auto const& type = pair.first;
			auto const& system = pair.second;
			auto const& systemSignature = m_Signatures[type];

			if ((entitySignature & systemSignature) == systemSignature)
			{
				system->m_Entities.insert(entity);
			}
			else
			{
				system->m_Entities.erase(entity);
			}
		}
	}
private:
	std::unordered_map<const char*, Signature> m_Signatures{};

	std::unordered_map<const char*, std::shared_ptr<System>> m_Systems{};
};

class Coordinator
{
public:
	void Init()
	{
		m_ComponentManager = std::make_unique<ComponentManager>();
		m_EntityManager = std::make_unique<EntityManager>();
		m_SystemManager = std::make_unique<SystemManager>();
	}

	Entity CreateEntity()
	{
		return m_EntityManager->CreateEntity();
	}

	void DestroyEntity(Entity entity)
	{
		m_EntityManager->DestroyEntity(entity);
		m_ComponentManager->EntityDestroyed(entity);
		m_SystemManager->EntityDestroyed(entity);
	}

	template<typename T>
	void RegisterComponent()
	{
		m_ComponentManager->RegisterComponent<T>();
	}

	template<typename T>
	void AddComponent(Entity entity, T component)
	{
		m_ComponentManager->AddComponent<T>(entity, component);

		auto signature = m_EntityManager->GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), true);
		m_EntityManager->SetSignature(entity, signature);

		m_SystemManager->EntitySignatureChanged(entity, signature);
	}

	template<typename T>
	void RemoveComponent(Entity entity)
	{
		m_ComponentManager->RemoveComponent<T>(entity);

		auto signature = m_EntityManager->GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), false);
		m_EntityManager->SetSignature(entity, signature);

		m_SystemManager->EntitySignatureChanged(entity, signature);
	}

	template<typename T>
	T& GetComponent(Entity entity)
	{
		return m_ComponentManager->GetComponent<T>(entity);
	}

	template<typename T>
	ComponentType GetComponentType()
	{
		return m_ComponentManager->GetComponentType<T>();
	}

	template<typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		return m_SystemManager->RegisterSystem<T>();
	}

	template<typename T>
	void SetSystemSignature(Signature signature)
	{
		m_SystemManager->SetSignature<T>(signature);
	}

private:
	std::unique_ptr<ComponentManager> m_ComponentManager;
	std::unique_ptr<EntityManager> m_EntityManager;
	std::unique_ptr<SystemManager> m_SystemManager;
};


//Systems
class PhysicsSystem : public System
{
public:
	void Update(float deltaTime, olc::PixelGameEngine* pge);
};

class RenderSystem : public System
{
public:
	void Render(olc::PixelGameEngine* pge);
};

class MovementSystem : public System
{
public:
	void OnMove(olc::vf2d direction);
};

class CollisionSystem : public System
{
public:
	void CheckCollision(Entity other);
	void CheckAllCollision();
};

class BulletSystem : public System
{
public:
	void MoveBullet(float deltaTime, olc::PixelGameEngine* engine, std::shared_ptr<CollisionSystem> collisionSystem);
};

class AISystem : public System
{
public:
	void Move(float deltaTime, olc::PixelGameEngine* engine, std::shared_ptr<CollisionSystem> collisionSystem);
	void Shoot(float deltaTime);
};