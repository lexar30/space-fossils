#pragma once

#include "space_fossils/core/file_tree/model/tree_metadata.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils/core/file_tree/session/session.hxx"
#include "space_fossils/core/size_formatter.hxx"

#include <memory>

namespace space_fossils::cli {
	using space_fossils::core::file_tree::Storage;
	using space_fossils::core::file_tree::Session;
	using space_fossils::core::file_tree::TreeMetadata;
	using space_fossils::core::FileSizeUnitSystem;

	struct TreeContext
	{
		Storage storage;
		Session session{ storage };
		TreeMetadata treeMetadata = {};
	};

	struct AppState
	{
		std::unique_ptr<TreeContext> context = std::make_unique<TreeContext>();
		bool isQuitRequested = false;
		FileSizeUnitSystem unitsType = FileSizeUnitSystem::Decimal;

		bool HasActiveTree() const
		{
			return context->storage.GetRoot() != nullptr;
		}

		bool IsFreshStorage() const
		{
			return context->storage.GetVersion() == 0;
		}

		void ResetTreeContext()
		{
			context = std::make_unique<TreeContext>();
		}
	};
}
