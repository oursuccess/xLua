#include "lua.h"
#include "debugger_proto.h"

namespace CPL
{
	JsonProtocol::~JsonProtocol()
	{
	}

	nlohmann::json JsonProtocol::Serialize()
	{
		return nlohmann::json::object();
	}

	void JsonProtocol::Deserialize(const nlohmann::json& json)
	{
	}

	nlohmann::json InitParams::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void InitParams::Deserialize(const nlohmann::json& json)
	{
		if (json["helper"].is_string()) {
			helper = json["helper"];
		}

		if (json["ext"].is_array()) {
			for (const auto& item : json["ext"]) {
				ext.emplace_back(item);
			}
		}
	}

	nlohmann::json BreakPoint::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void BreakPoint::Deserialize(const nlohmann::json& json)
	{
		if (json.count("file") != 0) file = json["file"];
		if (json.count("line") != 0) line = json["line"];
		if (json.count("condition") != 0) condition = json["condition"];
		if (json.count("hitCondition") != 0) hitCondition = json["hitCondition"];
		if (json.count("logMessage") != 0) logMessage = json["logMessage"];
	}


	nlohmann::json AddBreakpointParams::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void AddBreakpointParams::Deserialize(const nlohmann::json& json)
	{
		if (json["clear"].is_boolean()) clear = json["clear"];
		if (json["breakPoints"].is_array()) {
			for (const auto& item : json["breakPoints"]) {
				auto bp = std::make_shared<BreakPoint>();
				bp->Deserialize(item);
				breakpoints.emplace_back(bp);
			}
		}
	}

	nlohmann::json RemoveBreakpointParams::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void RemoveBreakpointParams::Deserialize(const nlohmann::json& json)
	{
		if (json["breakPoints"].is_array()) {
			for (const auto& item : json["breakPoints"]) {
				auto bp = std::make_shared<BreakPoint>();
				bp->Deserialize(item);
				breakpoints.emplace_back(bp);
			}
		}
	}

	nlohmann::json ActionParams::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void ActionParams::Deserialize(const nlohmann::json& json)
	{
		if (json.count("action") != 0 && json["action"].is_number_integer()) {
			action = json["action"].get<DebugAction>();
		}
	}

	Variable::Variable() : nameType(LUA_TSTRING), valueType(LUA_TNIL), cacheId(0)	//FIXME: valueType is 0 in emmylua, check this
	{
	}

	nlohmann::json Variable::Serialize()
	{
		auto obj = nlohmann::json::object();
		obj["name"] = name;
		obj["nameType"] = nameType;
		obj["value"] = value;
		obj["valueType"] = valueType;
		obj["valueTypeName"] = valueTypeName;
		obj["cacheId"] = cacheId;

		if (!children.empty()) {
			auto arr = nlohmann::json::array();
			for (auto idx : children) arr.push_back(idx->Serialize());
			obj["children"] = arr;
		}
		return obj;
	}

	void Variable::Deserialize(const nlohmann::json& json)
	{
		JsonProtocol::Deserialize(json);
	}

	Stack::Stack() : level(0), line(0), variableArena(std::make_shared<Arena<Variable>>())
	{
	}

	nlohmann::json Stack::Serialize()
	{
		auto obj = nlohmann::json::object();
		obj["file"] = file;
		obj["functionName"] = functionName;
		obj["level"] = level;
		obj["line"] = line;

		auto arr = nlohmann::json::array();
		for (auto idx : localVariables) arr.push_back(idx->Serialize());
		obj["localVariables"] = arr;

		arr = nlohmann::json::array();
		for (auto idx : upvalueVariables) arr.push_back(idx->Serialize());
		obj["upvalueVariables"] = arr;

		return obj;
	}

	void Stack::Deserialize(const nlohmann::json& json)
	{
		JsonProtocol::Deserialize(json);
	}

	EvalContext::EvalContext()
	{
		result = arena.Alloc();
	}

	nlohmann::json EvalContext::Serialize()
	{
		auto obj = nlohmann::json::object();
		obj["seq"] = seq;
		obj["success"] = success;
		if (success) obj["value"] = result->Serialize();
		else obj["error"] = error;
		return obj;
	}

	void EvalContext::Deserialize(const nlohmann::json& json)
	{
		if (json.count("seq") != 0) seq = json["seq"];
		if (json.count("expr") != 0) expr = json["expr"];
		if (json.count("value") != 0) value = json["value"];
		if (json.count("setValue") != 0) setValue = json["setValue"];
		if (json.count("stackLevel") != 0) stackLevel = json["stackLevel"];
		if (json.count("depth") != 0) depth = json["depth"];
		if (json.count("cacheId") != 0) cacheId = json["cacheId"];
	}

	nlohmann::json EvalParams::Serialize()
	{
		return JsonProtocol::Serialize();
	}

	void EvalParams::Deserialize(const nlohmann::json& json)
	{
		context = std::make_shared<EvalContext>();
		context->Deserialize(json);
	}
}
