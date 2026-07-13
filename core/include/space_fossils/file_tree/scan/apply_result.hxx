#pragma once

#include "space_fossils/file_tree/storage/storage_change.hxx"

#include <optional>

namespace space_fossils::core::file_tree::scan {
	enum class ApplyStatus
	{
		NoJob,
		Applied,
		Rejected
	};

	struct ApplyResult
	{
		ApplyStatus status = ApplyStatus::NoJob;
		std::optional<AppliedChange> appliedChange;
	};
}
