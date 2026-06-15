.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube
# List of directories containing source code
SOURCE_DIRS		= src src/ios third_party/bearssl

# CPU architecture names
ifdef IOS_SIM
	ARCH_32	:= x86_64
	ARCH_64	:= arm64
	SDK_TYPE:= iPhoneSimulator
else
	ARCH_32	:= arm64
	ARCH_64	:= arm64
	SDK_TYPE:= iPhoneOS
endif
FULL_TARGET	:= $(TARGET)-$(SDK_TYPE)

# override to change minimum iOS runtime/deployment version
IOS_MIN_VERSION ?= 12.0


#---------------------------------------------------------------------------------
# Paths
#---------------------------------------------------------------------------------
SDK_NAME		:= $(if $(filter $(SDK_TYPE),iPhoneSimulator),iphonesimulator,iphoneos)
IOS_SDK_PATH	:= $(shell xcrun --sdk $(SDK_NAME) --show-sdk-path)

# Directories where object files are placed
BUILD_DIR_ROOT	:= build/ios
BUILD_DIR_APP	:= $(BUILD_DIR_ROOT)/$(FULL_TARGET).app
BUILD_DIR_32	:= $(BUILD_DIR_ROOT)/slice32-$(ARCH_32)
BUILD_DIR_64	:= $(BUILD_DIR_ROOT)/slice64-$(ARCH_64)


#---------------------------------------------------------------------------------
# Compiler tools/paths
#---------------------------------------------------------------------------------
CC 		:= xcrun --sdk $(SDK_NAME) clang
LD 		:= xcrun --sdk $(SDK_NAME) clang
LIPO	:= xcrun lipo
ACTOOL	:= xcrun actool
PLBUDDY	:= /usr/libexec/PlistBuddy
PLUTIL  := /usr/bin/plutil


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
M_FILES := $(wildcard src/ios/*.m)
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS_32	:= $(addprefix $(BUILD_DIR_32)/, $(notdir $(C_FILES:%.c=%.o) $(M_FILES:%.m=%.o)))
OBJS_64	:= $(addprefix $(BUILD_DIR_64)/, $(notdir $(C_FILES:%.c=%.o) $(M_FILES:%.m=%.o)))

ifeq ($(SDK_TYPE),iPhoneSimulator)
DEPLOYMENT_FLAG := -mios-simulator-version-min
ASSET_PLATFORM := iphonesimulator
else
DEPLOYMENT_FLAG := -miphoneos-version-min
ASSET_PLATFORM := iphoneos
endif

CFLAGS  := -g -Os -fno-math-errno -fvisibility=hidden -fno-common -Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wunreachable-code -Wno-missing-braces -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wuninitialized -Wconditional-uninitialized -Wno-shadow -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wno-float-conversion -Wobjc-literal-conversion -Wshorten-64-to-32 -Wpointer-sign -isysroot $(IOS_SDK_PATH) -fstrict-aliasing $(DEPLOYMENT_FLAG)=$(IOS_MIN_VERSION) -Wno-sign-conversion -Wcomma -Wstrict-prototypes
MFLAGS  := $(CFLAGS)

LDFLAGS  := -isysroot $(IOS_SDK_PATH) -Xlinker -rpath -Xlinker @executable_path/Frameworks $(DEPLOYMENT_FLAG)=$(IOS_MIN_VERSION)
LIBS 	 := -framework Foundation -framework CoreFoundation -framework CoreGraphics -framework OpenGLES -framework UIKit -framework Security -framework CoreText -framework QuartzCore -lobjc

# Dependency tracking
DEPFLAGS_32 := -MT $@ -MMD -MP -MF $(BUILD_DIR_32)/$*.d
DEPFLAGS_64 := -MT $@ -MMD -MP -MF $(BUILD_DIR_64)/$*.d
DEPFILES_32 := $(OBJS_32:%.o=%.d)
DEPFILES_64 := $(OBJS_64:%.o=%.d)


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR_32) $(BUILD_DIR_64) $(BUILD_DIR_APP) $(FULL_TARGET).ipa

$(BUILD_DIR_32):
	mkdir -p $(BUILD_DIR_32)

$(BUILD_DIR_64):
	mkdir -p $(BUILD_DIR_64)

$(BUILD_DIR_APP):
	mkdir -p $(BUILD_DIR_APP)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_32)/$(TARGET): $(OBJS_32)
	$(LD) $^ -arch $(ARCH_32) $(LDFLAGS) $(LIBS) -dead_strip -o $@

$(BUILD_DIR_64)/$(TARGET): $(OBJS_64)
	$(LD) $^ -arch $(ARCH_64) $(LDFLAGS) $(LIBS) -dead_strip -o $@


$(BUILD_DIR_APP)/$(TARGET): $(BUILD_DIR_32)/$(TARGET) $(BUILD_DIR_64)/$(TARGET)
ifeq ($(ARCH_32),$(ARCH_64))
	cp $< $@
else
	$(LIPO) -create $^ -output $@
endif

$(BUILD_DIR_APP)/Assets.car:
	$(ACTOOL) --output-format human-readable-text --notices --warnings --output-partial-info-plist $(BUILD_DIR_APP)/assetcatalog_generated_info.plist --app-icon AppIcon --compress-pngs --enable-on-demand-resources YES --target-device iphone --target-device ipad --minimum-deployment-target $(IOS_MIN_VERSION) --platform $(ASSET_PLATFORM) --product-type com.apple.product-type.application --compile $(BUILD_DIR_APP) misc/ios/Assets.xcassets

$(BUILD_DIR_APP)/Info.plist: misc/ios/Info.plist
	cp $^ $@
	$(PLBUDDY) -c "add MinimumOSVersion string $(IOS_MIN_VERSION)" $@
	$(PLBUDDY) -c "add UIDeviceFamily array" $@
	$(PLBUDDY) -c "add UIDeviceFamily:0 integer 1" $@
	$(PLBUDDY) -c "add UIDeviceFamily:1 integer 2" $@
	# must be in binary format, otherwise xcrun simctl install rejects it
	$(PLUTIL) -convert binary1 $@

$(BUILD_DIR_APP)/PkgInfo: misc/ios/PkgInfo
	cp $^ $@


$(FULL_TARGET).ipa: $(BUILD_DIR_APP)/$(TARGET) $(BUILD_DIR_APP)/Assets.car $(BUILD_DIR_APP)/Info.plist $(BUILD_DIR_APP)/PkgInfo
	rm -rf Payload
	mkdir -p Payload
	cp -R $(BUILD_DIR_APP) Payload
	mv Payload/$(FULL_TARGET).app Payload/$(TARGET).app
	zip -r $(FULL_TARGET).ipa Payload


#---------------------------------------------------------------------------------
# 32 bit object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_32)/%.o : src/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_32)/%.o : src/ios/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_32)/%.o : src/ios/%.m
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR_32)/%.o : third_party/bearssl/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# 64 bit object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_64)/%.o : src/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_64)/%.o : src/ios/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_64)/%.o : src/ios/%.m
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR_64)/%.o : third_party/bearssl/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES_32):
$(DEPFILES_64):

include $(wildcard $(DEPFILES_32))
include $(wildcard $(DEPFILES_64))
