#pragma once

#include "Binder.hpp"

#include "Engine.hpp"
#include "Mesh.hpp"

namespace MeshBind{
	const char* name = "Mesh";

	inline int constructor(lua_State* L);

	inline int _add(lua_State* L);
	inline int _has(lua_State* L);

	inline static int _gc(lua_State* L);

	inline int _load(lua_State* L);

	static const luaL_Reg global[] = {
		{ "add", _add },
		{ "has", _has },
		{ 0, 0 }
	};

	static const luaL_Reg meta[] = {
		{ "__gc", _gc },
		{ 0, 0 }
	};

	static const luaL_Reg methods[] = {
		{ "load", _load },
		{ 0, 0 }
	};
}

inline int MeshBind::constructor(lua_State* L){
	Engine& engine = Binder::getEngine(L);
	uint64_t id = luaL_checkinteger(L, -1);

	engine.manager.getComponent<Mesh>(id);

	Binder::checkEngineError(L);

	Binder::createEntityRef(L, id, name);

	return 1;
}

inline int MeshBind::_add(lua_State* L){
	uint64_t id = luaL_checkinteger(L, 1);

	Engine& engine = Binder::getEngine(L);

	if (!engine.manager.hasComponents<Transform>(id))
		luaL_error(L, "mesh requires entity to have transform");

	engine.manager.addComponent<Mesh>(id, luaL_checkstring(L, 2));

	Binder::checkEngineError(L);

	return 0;
}

int MeshBind::_has(lua_State * L){
	uint64_t id = luaL_checkinteger(L, 1);

	Engine& engine = Binder::getEngine(L);

	bool has = engine.manager.hasComponents<Mesh>(id);

	Binder::checkEngineError(L);

	lua_pushboolean(L, has);

	return 1;
}

int MeshBind::_gc(lua_State * L){
	EntityRef* entity = (EntityRef*)luaL_checkudata(L, 1, name);

	entity->invalidate();

	return 0;
}

int MeshBind::_load(lua_State * L){
	EntityRef* entity = (EntityRef*)luaL_checkudata(L, 1, name);

	Binder::checkEntity(L, entity);

	Binder::getSystem<Renderer>(L)->load(entity->id(), luaL_checkstring(L, 2));

	return 0;
}
