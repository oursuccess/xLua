#pragma once

#include "util/util.h"
#include "nlohmann/json_fwd.hpp"
#include "proto.h"

namespace CPL
{
	class DebuggerFacade;

	class ProtoHandler {
	public:
		explicit ProtoHandler(DebuggerFacade* owner);
		void OnDispatch(nlohmann::json params);
	private:
		void OnInitReq(InitParams& params);
		void OnReadyReq();
		void OnAddBreakPointReq(AddBreakpointParams& params);
		void OnRemoveBreakPointReq(RemoveBreakpointParams& params);
		void OnActionReq(ActionParams& params);
		void OnEvalReq(EvalParams& params);

		DebuggerFacade* owner;
	};
}