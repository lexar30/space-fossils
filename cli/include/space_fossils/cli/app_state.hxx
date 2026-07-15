#pragma once

#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils/core/file_tree/session/session.hxx"

namespace space_fossils::cli {
	using space_fossils::core::file_tree::Storage;
	using space_fossils::core::file_tree::Session;

	struct AppState
	{
		Storage storage;
		Session session;

		bool isQuitRequested = false;
		bool isStorageFilled = false;
	};
}
