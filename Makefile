# auto generate phony targets
.PHONY: $(shell grep --no-filename -E '^([a-zA-Z_-]|/)+:' $(MAKEFILE_LIST) | sed 's/:.*//')

clang-format:
	clang-format -i $$(find src -type f -name '*.cpp' -o -name '*.hpp')
