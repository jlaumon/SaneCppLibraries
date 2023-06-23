// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#pragma once
#include "../FileSystem/FileSystem.h"
#include "../FileSystem/FileSystemWalker.h"
#include "../FileSystem/Path.h"
#include "../Foundation/StringBuilder.h"
#include "../Testing/Test.h"
#include "../Threading/Threading.h"
#include "Plugin.h"

namespace SC
{
struct PluginTest;
}

struct SC::PluginTest : public SC::TestCase
{
    SmallString<255>   testPluginsPath;
    [[nodiscard]] bool setupPluginPath()
    {
        const StringView testPath = Path::dirname(StringView(__FILE__));
#if SC_MSVC
        if (testPath.startsWithChar('\\'))
        {
            // For some reason on MSVC __FILE__ returns paths on network drives with a single starting slash
            (void)Path::join(testPluginsPath, {""_a8, testPath, "PluginTestDirectory"_a8});
        }
        else
        {
            (void)Path::join(testPluginsPath, {testPath, "PluginTestDirectory"_a8});
        }
#else
        (void)Path::join(testPluginsPath, {testPath, "PluginTestDirectory"_a8});
#endif
        return true;
    }
    PluginTest(SC::TestReport& report) : TestCase(report, "PluginTest")
    {
        using namespace SC;
        if (test_section("PluginDefinition"))
        {
            StringView test =
                R"(
                // SC_BEGIN_PLUGIN
                // Name:          Test Plugin
                // Version:       1
                // Description:   A Simple text plugin
                // Category:      Generic
                // Dependencies:  TestPluginChild,TestPlugin02
                // SC_END_PLUGIN
            )"_a8;
            PluginDefinition definition;
            StringView       extracted;
            SC_TEST_EXPECT(PluginDefinition::find(test, extracted));
            SC_TEST_EXPECT(PluginDefinition::parse(extracted, definition));
            SC_TEST_EXPECT(definition.identity.name == "Test Plugin");
            SC_TEST_EXPECT(definition.identity.version == "1");
            SC_TEST_EXPECT(definition.description == "A Simple text plugin");
            SC_TEST_EXPECT(definition.category == "Generic");
            SC_TEST_EXPECT(definition.dependencies[0] == "TestPluginChild");
            SC_TEST_EXPECT(definition.dependencies[1] == "TestPlugin02");
        }
        if (test_section("PluginScanner/PluginCompiler/PluginRegistry"))
        {
            SC_TEST_EXPECT(setupPluginPath());

            // Scan for definitions
            SmallVector<PluginDefinition, 5> definitions;
            SC_TEST_EXPECT(PluginScanner::scanDirectory(testPluginsPath.view(), definitions));
            SC_TEST_EXPECT(definitions.size() == 2);

            // Save parent and child plugin identifiers and paths
            const int  parentIndex            = definitions.items[0].dependencies.isEmpty() ? 0 : 1;
            const int  childIndex             = parentIndex == 0 ? 1 : 0;
            const auto identifierChildString  = definitions.items[childIndex].identity.identifier;
            const auto identifierParentString = definitions.items[parentIndex].identity.identifier;
            const auto pluginScriptPath       = definitions.items[childIndex].files[0].absolutePath;

            const StringView identifierChild  = identifierChildString.view();
            const StringView identifierParent = identifierParentString.view();

            // Init compiler
            PluginCompiler compiler;
            SC_TEST_EXPECT(PluginCompiler::findBestCompiler(compiler));

            // Setup registry
            PluginRegistry registry;
            SC_TEST_EXPECT(registry.init(move(definitions)));
            SC_TEST_EXPECT(registry.loadPlugin(identifierChild, compiler, report.executableFile));

            // Check that plugins have been compiled and are valid
            const PluginDynamicLibrary* pluginChild  = registry.findPlugin(identifierChild);
            const PluginDynamicLibrary* pluginParent = registry.findPlugin(identifierParent);
            SC_TEST_EXPECT(pluginChild->dynamicLibrary.isValid());
            SC_TEST_EXPECT(pluginParent->dynamicLibrary.isValid());
            using FunctionIsPluginOriginal = bool (*)();
            FunctionIsPluginOriginal isPluginOriginal;
            SC_TEST_EXPECT(pluginChild->dynamicLibrary.getSymbol("isPluginOriginal", isPluginOriginal));
            SC_TEST_EXPECT(isPluginOriginal());

            // Modify child plugin
            String     sourceContent;
            FileSystem fs;
            SC_TEST_EXPECT(fs.read(pluginScriptPath.view(), sourceContent, StringEncoding::Ascii));
            String sourceMod1;
            SC_TEST_EXPECT(StringBuilder(sourceMod1)
                               .appendReplaceAll(sourceContent.view(), //
                                                 "bool isPluginOriginal() { return true; }",
                                                 "bool isPluginOriginal() { return false; }"));
            String sourceMod2;
            SC_TEST_EXPECT(StringBuilder(sourceMod2).appendReplaceAll(sourceMod1.view(), "original", "MODIFIED"));
            SC_TEST_EXPECT(fs.write(pluginScriptPath.view(), sourceMod2.view()));

            // Reload child plugin
            SC_TEST_EXPECT(registry.loadPlugin(identifierChild, compiler, report.executableFile,
                                               PluginRegistry::LoadMode::Reload));

            // Check child plugin modified
            SC_TEST_EXPECT(pluginChild->dynamicLibrary.isValid());
            SC_TEST_EXPECT(pluginChild->dynamicLibrary.getSymbol("isPluginOriginal", isPluginOriginal));
            SC_TEST_EXPECT(not isPluginOriginal());

            // Unload parent plugin
            SC_TEST_EXPECT(registry.unloadPlugin(identifierParent));

            // Check that both parent and child plugin have been unloaded
            SC_TEST_EXPECT(not pluginChild->dynamicLibrary.isValid());
            SC_TEST_EXPECT(not pluginParent->dynamicLibrary.isValid());

            // Cleanup
            SC_TEST_EXPECT(fs.write(pluginScriptPath.view(), sourceContent.view()));
            SC_TEST_EXPECT(registry.removeAllBuildProducts(identifierChild));
            SC_TEST_EXPECT(registry.removeAllBuildProducts(identifierParent));
        }
    }
};