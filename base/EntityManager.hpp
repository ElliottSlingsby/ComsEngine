#pragma once

#include "ObjectPool.hpp"
#include "Component.hpp"
#include "System.hpp"

#include <queue>
#include <tuple>
#include <set>

// Entity debug info for testing
struct EntityDebug{
	uint32_t index;
	uint32_t version;
	uint8_t state;
	uint32_t mask;
	uint32_t enabled;
	uint16_t references;

	bool valid = false;
};

class EntityManager{
	// Byte pools for each component type
	std::vector<BasePool*> _pools;

	// Entity occupant slots
	std::vector<uint8_t> _states;
	// Bit masks representing components of an entity
	std::vector<uint32_t> _masks; 
	// Bit masks representing enabled components
	std::vector<uint32_t> _enabled;
	// Version of each entity
	std::vector<uint32_t> _versions;
	// Reference counters
	std::vector<uint16_t> _references;

	// List of entities to be destroyed
	std::queue<uint64_t> _destroyed;

	// List of recently destroyed entities
	std::queue<uint32_t> _free; 

	// Entity count
	uint32_t _entities = 0;

	bool _purge = false;

	// System register for calling BaseSystem events
	std::set<BaseSystem*> _systems;

	// Error value set when using _check functions
	uint8_t _error = 0;

	// Non-copyable overloads
	EntityManager(const EntityManager& other) = delete;
	EntityManager& operator=(const EntityManager& other) = delete;

	// Erase entity component's from object pools
	inline void _eraseEntity(uint32_t index);
	
	// Error check functions for entities, returns true if error, sets _error to enum value
	inline bool _checkRange(uint32_t index);
	inline bool _checkSlot(uint32_t index);
	inline bool _checkDestroyed(uint32_t index);
	inline bool _checkVersion(uint32_t index, uint32_t version);
	inline bool _checkComponent(uint32_t index, uint8_t type);
	inline bool _checkNoComponent(uint32_t index, uint8_t type);
	inline bool _checkPool(uint8_t type);

	// Prints errors string of _error
	inline void _verboseError();

	// Fill component type tuple with relevent components at existing entity index
	// Entry point of compile time recursive function
	template <typename ...Ts>
	inline void _fillTuple(uint32_t index, std::tuple<Ts*...>& t); 

	// Actual transformation function
	template <uint32_t I, typename ...Ts>
	static inline void _fillTuple(std::vector<BasePool*>& pools, uint32_t index, std::tuple<Ts*...>& t);

	// Body of compiled recursive function
	template <uint32_t I, typename ...Ts>
	struct FillTuple{
		inline void operator()(std::vector<BasePool*>& pools, uint32_t index, std::tuple<Ts*...>& t){ 
			_fillTuple<I, Ts...>(pools, index, t);
			FillTuple<I - 1, Ts...>{}(pools, index, t);
		}
	};

	// Bottom level of compile time recursive function
	template <typename ...Ts>
	struct FillTuple<0, Ts...>{
		inline void operator()(std::vector<BasePool*>& pools, uint32_t index, std::tuple<Ts*...>& t){ 
			_fillTuple<0, Ts...>(pools, index, t);
		}
	};

	// Bottom level for systems with no components
	template <typename ...Ts>
	struct FillTuple<-1, Ts...>{
		inline void operator()(std::vector<BasePool*>& pools, uint32_t index, std::tuple<Ts*...>& t){} 
	};

public:
	// Entity states for entity _states vector
	enum EntityState : uint8_t{ Empty, Active, Inactive, Destroyed };

	// Entity error types
	enum EntityError : uint8_t{ Okay, Invalid, Destructed, Component, NoComponent };

	EntityManager();
	~EntityManager();

	// Create new entity, returns 64 bit ID to new entity
	inline uint64_t createEntity();
	
	// Destroy entity and components using entity ID
	inline uint8_t destroyEntity(uint64_t id);

	// Enable or disable entity
	inline uint8_t setEntityActive(uint64_t id, bool active);

	// Enable or disable individual component of entity 
	template <typename T>
	inline uint8_t setComponentEnabled(uint64_t id, bool enabled);

	// Create and add component type to entity ID with constructor args, returns instance pointer
	template <typename T, typename ...Ts>
	inline T* const addComponent(uint64_t id, Ts... args);

	// Get component instance from entity ID
	template <typename T>
	inline T* const getComponent(uint64_t id);

	// Iterate over all entities related to a system (call from inside system)
	template <typename ...Ts>
	inline void processEntities(System<Ts...>* system);
	
	// Return entity state enum
	inline uint8_t getEntityState(uint64_t id);

	// Return entity mask representing attached components to an entity
	inline uint32_t getEntityMask(uint64_t id);

	// Checks if component types are attached to entity ID
	template <typename ...Ts>
	inline bool hasComponents(uint64_t id);

	// Checks if entity exists
	inline bool entityExists(uint64_t id);

	// Increments an entity's reference count (won't be erased until references hit zero)
	inline uint8_t addReference(uint64_t id);

	// Decrements an entity's reference count (won't be erased until references hit zero)
	inline uint8_t removeReference(uint64_t id);

	// Register a system for entity events like onCreate and onDestroy
	template<typename T>
	inline uint8_t registerSystem(T* system);

	// Erase any previously destroyed entities with a reference count of zero
	inline void eraseDestroyed();

	// Return total amount of references (for testing only)
	inline uint8_t totalReferences();

	// Remove everything (experimental)
	inline void purge();

	// Get error string from returned entity manager error
	inline std::string errorString(uint8_t error);

	// Check if an error was triggered from an entity manager function
	inline uint8_t getError();

	// Check if an error was triggered, but return error string instead
	inline std::string getErrorString();

	// Fill an entity debug struct with info (for testing only)
	inline int fillEntityDebug(uint64_t id, EntityDebug& debug);
};

inline bool EntityManager::_checkRange(uint32_t index){
	if (index < _masks.size())
		return true;

	_error = EntityError::Invalid;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkSlot(uint32_t index){
	if (_states[index] != EntityState::Empty)
		return true;

	_error = EntityError::Invalid;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkDestroyed(uint32_t index){
	if (_states[index] != EntityState::Destroyed)
		return true;

	_error = EntityError::Destructed;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkVersion(uint32_t index, uint32_t version){
	if (_versions[index] == version)
		return true;

	_error = EntityError::Invalid;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkComponent(uint32_t index, uint8_t type){
	if (BitHelper::getBit(type, _masks[index]))
		return true;

	_error = EntityError::NoComponent;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkNoComponent(uint32_t index, uint8_t type){
	if (!BitHelper::getBit(type, _masks[index]))
		return true;

	_error = EntityError::Component;
	_verboseError();

	return false;
}

inline bool EntityManager::_checkPool(uint8_t type){
	if (type < _pools.size() && _pools[type])
		return true;
	
	_error = EntityError::NoComponent;
	_verboseError();

	return false;
}

inline void EntityManager::_verboseError(){
#ifdef _DEBUG
	if (_error)
		printf("%s", errorString(_error).c_str());
#endif
}

template<typename T>
inline uint8_t EntityManager::registerSystem(T* system){
	if (_systems.find(system) != _systems.end())
		return _error;

	_systems.insert(system);

	return EntityError::Okay;
}

inline uint8_t EntityManager::addReference(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version)))
		return _error;

	// Increment references
	_references[index]++;

	return EntityError::Okay;
}

inline uint8_t EntityManager::removeReference(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _references[index] != 0))
		return _error;

	// Decrement referneces
	_references[index]--;

	// If references hits zero, and entity was destroyed
	if (!_references[index] && _states[index] == EntityState::Destroyed)
		_eraseEntity(index); // Erase entity

	return EntityError::Okay;
}

inline void EntityManager::_eraseEntity(uint32_t index){
	// Erase each component from pools
	for (uint8_t i = 0; i < _pools.size(); i++){
		if (BitHelper::getBit(i, _masks[index]))
			_pools[i]->erase(index);
	}

	// Add index to free list
	_free.push(index);

	// Update entity state and increment entity version
	_states[index] = EntityState::Empty;
	_versions[index]++;

	_entities--;
}

inline void EntityManager::eraseDestroyed(){
	// While entities are in destroy queue
	while (_destroyed.size()){
		uint32_t index = BitHelper::front(_destroyed.front());

		// Call onDestroy in each related system
		for (BaseSystem* system : _systems){
			if (system->mask && (system->mask & _masks[index]) == system->mask)
				system->onDestroy(_destroyed.front());
		}

		// If no references, erase entity
		if (!_references[index])
			_eraseEntity(index);

		_destroyed.pop();
	}

	// If purge, kill everything (experimental)
	if (_purge){
		for (BasePool* pool : _pools){
			if (pool)
				delete pool;
		}

		_pools.clear();
		_states.clear();
		_masks.clear();
		_enabled.clear();
		_versions.clear();
		_references.clear();

		_entities = 0;
		_purge = false;

		while (_destroyed.size())
			_destroyed.pop();

		while (_free.size())
			_free.pop();
	}
}

inline uint8_t EntityManager::totalReferences(){
	unsigned int total = 0;

	for (unsigned int i : _references)
		total += i;

	return total;
}

inline void EntityManager::purge(){
	_purge = true;
}

inline std::string EntityManager::errorString(uint8_t error){
	std::string prefix = "EntityManager error (entity ";
	std::string suffix = ")";

	if (error == EntityError::Invalid)
		return prefix + "ID is invalid" + suffix;
	else if (error == EntityError::Destructed)
		return prefix + "is destroyed" + suffix;
	else if (error == EntityError::Component)
		return prefix + "already has component" + suffix;
	else if (error == EntityError::NoComponent)
		return prefix + "does not have component" + suffix;

	return prefix + "is okay" + suffix;
}

inline uint8_t EntityManager::getError(){
	uint8_t error = _error;

	// Clear error
	_error = 0;

	return error;
}

inline std::string EntityManager::getErrorString(){
	return errorString(getError());
}

inline int EntityManager::fillEntityDebug(uint64_t id, EntityDebug& debug){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!_checkRange(index))
		return _error;

	debug.index = index;
	debug.version = _versions[index];
	debug.valid = version == _versions[index];
	debug.mask = _masks[index];
	debug.references = _references[index];
	debug.state = _states[index];
	debug.enabled = _enabled[index];
	
	return EntityError::Okay;
}

template<typename ...Ts>
inline void EntityManager::_fillTuple(uint32_t index, std::tuple<Ts*...>& t){
	const uint32_t size = std::tuple_size<std::tuple<Ts*...>>::value;

	FillTuple<size - 1, Ts...>{}(_pools, index, t);
}

template<uint32_t I, typename ...Ts>
inline void EntityManager::_fillTuple(std::vector<BasePool*>& pools, uint32_t index, std::tuple<Ts*...>& t){
	using T = typename std::tuple_element<I, std::tuple<Ts...>>::type;

	if (pools[T::type()]){
		T* component = pools[T::type()]->get<T>(index);
		std::get<I>(t) = pools[T::type()]->get<T>(index);
	}
}

inline uint64_t EntityManager::createEntity(){
	uint32_t index;

	if (_free.size()){
		// Use index from free lease
		index = _free.front();
		_free.pop();
	}
	else{
		// Increase entity info vectors
		if (_states.size() <= _entities){
			_states.resize(_entities + 1);
			_masks.resize(_entities + 1);
			_versions.resize(_entities + 1);
			_enabled.resize(_entities + 1);
			_references.resize(_entities + 1);
		}

		index = _entities;
	}

	assert(!_states[index]);

	// Set entity state to active
	_states[index] = EntityState::Active;

	// Set entity component and enabled mask to 0
	_masks[index] = 0;
	_enabled[index] = 0;

	// If index is 0, initiate version to 1
	if (!_versions[index])
		_versions[index] = 1;

	_entities++;

	// Combine index location and version to 64 bit ID
	return BitHelper::combine(index, _versions[index]);
}

inline uint8_t EntityManager::destroyEntity(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _checkDestroyed(index)))
		return _error;

	// Set entity state to destroyed
	_states[index] = EntityState::Destroyed;

	// Push id to destroy queue
	_destroyed.push(id);

	return EntityError::Okay;
}

inline uint8_t EntityManager::setEntityActive(uint64_t id, bool active){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _checkDestroyed(index)))
		return _error;

	if (active)
		_states[index] = EntityState::Active;
	else
		_states[index] = EntityState::Inactive;

	// Call onActivate / onDeactivate
	for (BaseSystem* system : _systems){
		if (system->mask && (system->mask & _masks[index]) == system->mask){
			if (active)
				system->onActivate(id);
			else
				system->onDeactivate(id);
		}
	}

	return EntityError::Okay;
}

template <typename T>
inline uint8_t EntityManager::setComponentEnabled(uint64_t id, bool enabled){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _checkComponent(index, T::type()) && _checkDestroyed(index)))
		return _error;

	_enabled[index] = BitHelper::setBit(T::type(), enabled, _enabled[index]);

	return EntityError::Okay;
}

template <typename T, typename ...Ts>
inline T* const EntityManager::addComponent(uint64_t id, Ts... args){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _checkNoComponent(index, T::type()) && _checkDestroyed(index)))
		return nullptr;

	if (T::type() >= _pools.size())
		_pools.resize(T::type() + 1);

	if (!_pools[T::type()])
		_pools[T::type()] = new ObjectPool<T>(T::chunk());

	uint32_t old = _masks[index];

	_masks[index] = BitHelper::setBit(T::type(), true, _masks[index]);
	_enabled[index] = BitHelper::setBit(T::type(), true, _enabled[index]);

	T* component = _pools[T::type()]->insert<T>(index, args...);

	// Call onCreate (for potential new systems)
	for (BaseSystem* system : _systems){ // if (system mask includes entity mask) and ((old system mask didn't include entity mask) or (entity mask was unchanged))
		if (system->mask && (system->mask & _masks[index]) == system->mask && ((system->mask & old) != system->mask) || (old == _masks[index])){
			system->onCreate(id);
		}
	}

	return component;
}

template<typename T>
inline T* const EntityManager::getComponent(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkPool(T::type()) && _checkRange(index) && _checkSlot(index) && _checkVersion(index, version) && _checkComponent(index, T::type())))
		return nullptr;

	// Return component from pool
	return _pools[T::type()]->get<T>(index);
}

template <typename ...Ts>
inline void EntityManager::processEntities(System<Ts...>* system){
	std::tuple<Ts*...> components;

	// Iterate through all entities
	for (uint32_t i = 0; i < _states.size(); i++){
		if (_states[i] != EntityState::Active)
			continue;

		// Call onProcess in systems
		if (system->mask && (system->mask & _enabled[i]) == system->mask){
			_fillTuple(i, components);
			system->onProcess(BitHelper::combine(i, _versions[i]), *std::get<Ts*>(components)...);
		}
	}
}

inline uint8_t EntityManager::getEntityState(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version)))
		return EntityState::Empty;

	// Return entity state
	return _states[index];
}

inline uint32_t EntityManager::getEntityMask(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version)))
		return EntityError::Okay;

	// Return entity component mask
	return _masks[index];
}

template<typename ...Ts>
inline bool EntityManager::hasComponents(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	if (!(_checkRange(index) && _checkSlot(index) && _checkVersion(index, version)))
		return false;

	// Create a component mask from template args
	uint32_t mask = BitHelper::createMask<Ts...>();

	// Return component mask comparison
	return (mask & _masks[index]) == mask;
}

inline bool EntityManager::entityExists(uint64_t id){
	uint32_t index = BitHelper::front(id);
	uint32_t version = BitHelper::back(id);

	return _checkRange(index) && _checkSlot(index) && _checkVersion(index, version);
}