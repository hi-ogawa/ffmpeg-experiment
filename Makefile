# auto generate phony targets
.PHONY: $(shell grep --no-filename -E '^([a-zA-Z_-]|/)+:' $(MAKEFILE_LIST) | sed 's/:.*//')

clang-format:
	clang-format -style=Chromium -i $$(find src -type f -name '*.cpp' -o -name '*.hpp')

clang-format/check:
	clang-format -style=Chromium --dry-run --Werror $$(find src -type f -name '*.cpp' -o -name '*.hpp')
