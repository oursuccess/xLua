#include "proto/proto_handler.h"

#include "debugger/facade.h"
#include "transporter/transporter.h"
#include "nlohmann/json.hpp"

namespace CPL
{
	CPL::ProtoHandler::ProtoHandler(DebuggerFacade* owner) : owner(owner)
	{
	}
	void ProtoHandler::OnDispatch(nlohmann::json doc)
	{
		if (!doc["cmd"].is_number_integer()) return;
		switch (doc["cmd"].get<MessageCMD>()) {
		case MessageCMD::InitReq: {
			InitParams params;
			params.Deserialize(doc);
			OnInitReq(params);
			break;
		}
		case MessageCMD::ReadyReq: {
			OnReadyReq();
			break;
		}
		case MessageCMD::AddBreakPointReq: {
			AddBreakpointParams params;
			params.Deserialize(doc);
			OnAddBreakPointReq(params);
			break;
		}
		case MessageCMD::RemoveBreakPointReq: {

			RemoveBreakpointParams params;
			params.Deserialize(doc);
			OnRemoveBreakPointReq(params);
			break;
		}
		case MessageCMD::ActionReq: {
			ActionParams params;
			params.Deserialize(doc);
			OnActionReq(params);
			break;
		}
		case MessageCMD::EvalReq: {
			EvalParams params;
			params.Deserialize(doc);
			OnEvalReq(params);
			break;
		}
		default: 
			break;
		}
	}
	void ProtoHandler::OnInitReq(InitParams& params)
	{
		owner->InitReq(params);
	}
	void ProtoHandler::OnReadyReq()
	{
		owner->ReadyReq();
	}
	void ProtoHandler::OnAddBreakPointReq(AddBreakpointParams& params)
	{
		auto& manager = owner->GetDebuggerManager();
		if (params.clear) manager.RemoveAllBreakpoints();
		for (auto& bp : params.breakpoints) manager.AddBreakpoint(bp);
	}
	void ProtoHandler::OnRemoveBreakPointReq(RemoveBreakpointParams& params)
	{
		auto& manager = owner->GetDebuggerManager();
		for (auto& bp : params.breakpoints) manager.RemoveBreakpoint(bp->file, bp->line);
	}
	void ProtoHandler::OnActionReq(ActionParams& params)
	{
		auto& manager = owner->GetDebuggerManager();
		manager.DoAction(params.action);
	}
	void ProtoHandler::OnEvalReq(EvalParams& params)
	{
		auto& manager = owner->GetDebuggerManager();
		manager.Eval(params.context);
	}
}