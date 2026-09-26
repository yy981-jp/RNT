#include <core/dir.h>


void dir(Ctx& ctx, const fs::path& targetDir) {
	for (const auto& e: fs::directory_iterator(targetDir)) {
		Entry ent{
			.id = ctx.cid++,
			.parent = ctx.parents.empty()? EntryId::INVALID() : ctx.parents.top(),
			.name = e.path().filename().string(),
			.depth = ctx.depth
		};
		ctx.entries.push_back(ent);

		if (e.is_directory()) {
			ctx.entries.back().isDir = true;

			ctx.parents.push(ent.id);
			ctx.depth++;
			dir(ctx, e.path());
			ctx.parents.pop();
			ctx.depth--;
		}

	}
}
