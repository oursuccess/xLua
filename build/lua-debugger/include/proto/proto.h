#pragma once

#include "nlohmann/json.hpp"
#include "arena/arena.h"
#include <vector>
#include <string>

namespace CPL
{
	enum class DebugAction
	{
		None = -1,
		Break = 0,
		Continue,
		StepOver,
		StepIn,
		StepOut,
		Stop,
	};

	class CPLDEBUGGER_API JsonProtocol {
	public:
		template<typename T>
		static nlohmann::json Serialize(std::vector<T>& objs) {
			auto res = nlohmann::json::array();
			for (auto& obj : objs) res.push_back(obj.Serialize());
			return res;
		}

		virtual ~JsonProtocol();

		virtual nlohmann::json Serialize();
		virtual void Deserialize(const nlohmann::json& json);
	};

	class CPLDEBUGGER_API InitParams : public JsonProtocol {
	public:
		std::string helper;
		std::vector<std::string> ext;
		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API BreakPoint : public JsonProtocol {
	public:
		std::string file;
		std::string condition;
		std::string hitCondition;
		std::string logMessage;
		int hitCount = 0;
		int line = 0;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API AddBreakpointParams : public JsonProtocol {
	public:
		bool clear = false;
		std::vector<std::shared_ptr<BreakPoint>> breakpoints;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API RemoveBreakpointParams : public JsonProtocol {
	public:
		std::vector<std::shared_ptr<BreakPoint>> breakpoints;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API ActionParams : public JsonProtocol {
	public:
		DebugAction action = DebugAction::None;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API Variable : public JsonProtocol {
	public:
		Variable();

		std::string name;
		std::string value;
		std::string valueTypeName;
		std::vector<Idx<Variable>> children;
		int nameType = 0;
		int valueType = 0;
		int cacheId = 0;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API Stack : public JsonProtocol {
	public:
		Stack();

		std::string file;
		std::string functionName;
		int level = 0;
		int line = 0;
		std::vector<Idx<Variable>> localVariables;
		std::vector<Idx<Variable>> upvalueVariables;
		std::shared_ptr<Arena<Variable>> variableArena;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};

	class CPLDEBUGGER_API EvalContext : public JsonProtocol {
	public:
		EvalContext();

		std::string expr;
		std::string value;
		std::string error;
		int seq = 0;
		int stackLevel = 0;
		int depth = 0;
		int cacheId = 0;
		Idx<Variable> result;
		bool success = false;
		bool setValue = false;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;

	private:
		Arena<Variable> arena;
	};

	class CPLDEBUGGER_API EvalParams : public JsonProtocol {
	public:
		std::shared_ptr<EvalContext> context;

		virtual nlohmann::json Serialize() override;
		virtual void Deserialize(const nlohmann::json& json) override;
	};
}
