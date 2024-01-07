BUILD_DIR = build
BUILD_TYPE = default
# BUILD_TYPE = debug

.PHONY: all clean

$(BUILD_DIR):
	conan build . -of=${BUILD_DIR} -b=missing -pr=${BUILD_TYPE}

clean:
	@rm -rf CMakeUserPresets.json
	@rm -rf $(BUILD_DIR)