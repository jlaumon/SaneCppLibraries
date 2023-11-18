// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "../FileSystem.h"
#include "../../FileSystem/Path.h"
#include "../../Testing/Testing.h"

namespace SC
{
struct FileSystemTest;
}

struct SC::FileSystemTest : public SC::TestCase
{
    FileSystemTest(SC::TestReport& report) : TestCase(report, "FileSystemTest")
    {
        using namespace SC;
        if (test_section("formatError"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            fs.preciseErrorMessages = true;

            Result res = fs.removeEmptyDirectory("randomNonExistingDirectory"_a8);
            SC_TEST_EXPECT(not res);
            fs.preciseErrorMessages = false;

            res = fs.removeEmptyDirectory("randomNonExistingDirectory"_a8);
            SC_TEST_EXPECT(not res);
        }
        if (test_section("makeDirectory / isDirectory / removeEmptyDirectory"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));

            SC_TEST_EXPECT(not fs.existsAndIsDirectory("Test0"));
            SC_TEST_EXPECT(fs.makeDirectory("Test0"_a8));
            SC_TEST_EXPECT(fs.exists("Test0"));
            SC_TEST_EXPECT(fs.existsAndIsDirectory("Test0"));
            SC_TEST_EXPECT(not fs.existsAndIsFile("Test0"));
            SC_TEST_EXPECT(fs.makeDirectory({"Test1", "Test2"}));
            SC_TEST_EXPECT(fs.existsAndIsDirectory("Test1"));
            SC_TEST_EXPECT(fs.existsAndIsDirectory("Test2"));
            SC_TEST_EXPECT(fs.removeEmptyDirectory("Test0"_a8));
            SC_TEST_EXPECT(fs.removeEmptyDirectory({"Test1", "Test2"}));
            SC_TEST_EXPECT(not fs.exists("Test0"));
            SC_TEST_EXPECT(not fs.existsAndIsFile("Test0"));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("Test0"));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("Test1"));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("Test2"));
        }
        if (test_section("makeDirectoryRecursive / removeEmptyDirectoryRecursive"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            SC_TEST_EXPECT(fs.makeDirectoryRecursive("Test3/Subdir"_a8));
            SC_TEST_EXPECT(fs.existsAndIsDirectory("Test3"));
            SC_TEST_EXPECT(fs.existsAndIsDirectory("Test3/Subdir"));
            SC_TEST_EXPECT(fs.removeEmptyDirectoryRecursive("Test3/Subdir"_a8));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("Test3"));
        }
        if (test_section("write / read / removeFile"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            StringView content = "ASDF content"_a8;
            SC_TEST_EXPECT(not fs.exists("file.txt"));
            SC_TEST_EXPECT(fs.write("file.txt", content));
            SC_TEST_EXPECT(fs.existsAndIsFile("file.txt"));
            String newString;
            SC_TEST_EXPECT(fs.read("file.txt", newString, StringEncoding::Ascii));
            SC_TEST_EXPECT(newString.view() == content);
            SC_TEST_EXPECT(fs.removeFile("file.txt"_a8));
            SC_TEST_EXPECT(not fs.exists("file.txt"));
        }
        if (test_section("copyfile/existsAndIsFile"))
        {
            FileSystem fs;
            StringView contentSource = "this is some content"_a8;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            SC_TEST_EXPECT(not fs.exists("sourceFile.txt"));
            SC_TEST_EXPECT(fs.write("sourceFile.txt", contentSource));
            SC_TEST_EXPECT(fs.existsAndIsFile("sourceFile.txt"));
            SC_TEST_EXPECT(not fs.exists("destinationFile.txt"));
            SC_TEST_EXPECT(fs.copyFile("sourceFile.txt", "destinationFile.txt",
                                       FileSystem::CopyFlags().setOverwrite(true).setUseCloneIfSupported(false)));
            String content;
            SC_TEST_EXPECT(fs.read("destinationFile.txt", content, StringEncoding::Ascii));
            SC_TEST_EXPECT(content.view() == contentSource);
            SC_TEST_EXPECT(fs.copyFile("sourceFile.txt", "destinationFile.txt",
                                       FileSystem::CopyFlags().setOverwrite(true).setUseCloneIfSupported(true)));
            SC_TEST_EXPECT(fs.existsAndIsFile("destinationFile.txt"));
            SC_TEST_EXPECT(fs.read("destinationFile.txt", content, StringEncoding::Ascii));
            SC_TEST_EXPECT(content.view() == contentSource);
            SC_TEST_EXPECT(fs.removeFile({"sourceFile.txt", "destinationFile.txt"}));
            SC_TEST_EXPECT(not fs.exists("sourceFile.txt"));
            SC_TEST_EXPECT(not fs.exists("destinationFile.txt"));
        }
        if (test_section("Copy Directory (recursive)"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            // Create a directory
            SC_TEST_EXPECT(fs.makeDirectory("copyDirectory"_a8));
            SC_TEST_EXPECT(fs.write("copyDirectory/testFile.txt", "asdf"_a8));
            SC_TEST_EXPECT(fs.existsAndIsFile("copyDirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.makeDirectory("copyDirectory/subdirectory"_a8));
            SC_TEST_EXPECT(fs.write("copyDirectory/subdirectory/testFile.txt", "asdf"_a8));

            SC_TEST_EXPECT(fs.copyDirectory("copyDirectory", "COPY_copyDirectory"));
            SC_TEST_EXPECT(fs.existsAndIsFile("COPY_copyDirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.existsAndIsFile("COPY_copyDirectory/subdirectory/testFile.txt"_a8));

            SC_TEST_EXPECT(not fs.copyDirectory("copyDirectory", "COPY_copyDirectory"));

            SC_TEST_EXPECT(
                fs.copyDirectory("copyDirectory", "COPY_copyDirectory", FileSystem::CopyFlags().setOverwrite(true)));

            SC_TEST_EXPECT(fs.removeFile("copyDirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.removeFile("copyDirectory/subdirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.removeEmptyDirectory("copyDirectory/subdirectory"_a8));
            SC_TEST_EXPECT(fs.removeEmptyDirectory("copyDirectory"_a8));
            SC_TEST_EXPECT(fs.removeFile("COPY_copyDirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.removeFile("COPY_copyDirectory/subdirectory/testFile.txt"_a8));
            SC_TEST_EXPECT(fs.removeEmptyDirectory("COPY_copyDirectory/subdirectory"_a8));
            SC_TEST_EXPECT(fs.removeEmptyDirectory("COPY_copyDirectory"_a8));
        }
        if (test_section("Remove Directory (recursive)"))
        {
            FileSystem fs;
            SC_TEST_EXPECT(fs.init(report.applicationRootDirectory));
            SC_TEST_EXPECT(fs.makeDirectory("removeDirectoryTest"_a8));
            SC_TEST_EXPECT(fs.write("removeDirectoryTest/testFile.txt", "asdf"_a8));
            SC_TEST_EXPECT(fs.makeDirectory("removeDirectoryTest/another"_a8));
            SC_TEST_EXPECT(fs.write("removeDirectoryTest/another/yeah.txt", "asdf"_a8));
            SC_TEST_EXPECT(fs.removeDirectoryRecursive("removeDirectoryTest"_a8));
            SC_TEST_EXPECT(not fs.existsAndIsFile("removeDirectoryTest/testFile.txt"_a8));
            SC_TEST_EXPECT(not fs.existsAndIsFile("removeDirectoryTest/another/yeah.txt"_a8));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("removeDirectoryTest/another"_a8));
            SC_TEST_EXPECT(not fs.existsAndIsDirectory("removeDirectoryTest"_a8));
        }
    }
};

namespace SC
{
void runFileSystemTest(SC::TestReport& report) { FileSystemTest test(report); }
} // namespace SC
