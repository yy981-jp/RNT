#include <core/dir.h>
#include <core/texter.h>


int main(int argc, char *argv[]) {
	fs::path target;
	if (argc < 2) target = fs::current_path();
	else target = fs::path(argv[1]);

	Ctx ctx{};
	dir(ctx, target);

	for (const auto& e: ctx.entries) {
		printf("%llu-%llu:\t%s\n", e.id.value, e.parent.value, e.name.c_str());
	}


}
