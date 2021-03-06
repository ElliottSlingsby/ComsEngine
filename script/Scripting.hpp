#pragma once

#include "System.hpp"

#include "Script.hpp"
#include "Input.hpp"

#include <lua.hpp>
#include <string>
#include <set>

class Scripting : public System<Script>{
	lua_State* _L;

	std::set<std::string> _loadedScripts;

	std::string _scriptPath;

	Input* _input;

	bool _reloaded = false;
	bool _reset = false;

	std::string _startScript;
	bool _started = true;

public:
	Scripting(Engine* engine, const std::string& scriptPath = "");
	~Scripting();

	void load() override;
	void reset() override;
	void update() override;

	void onProcess(uint64_t id, Script& script) override;

	void callFile(const std::string& file);

	bool createInstance(uint64_t id, const std::string& type);
	bool destroyInstance(uint64_t id, const std::string& type, unsigned int number = 0);

	int getInstance(uint64_t id, const std::string& type, unsigned int number = 0);

	bool registerFile(const std::string& type, const std::string& file);

	void onCreate(uint64_t id) override;
	void onDestroy(uint64_t id) override;
};