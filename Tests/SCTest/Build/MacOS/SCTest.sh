#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PATH=$PATH:/opt/homebrew/bin/
cd "${SCRIPT_DIR}/../../../.."


POSITIONAL_ARGS=()

GCC_DEBUG_FLAG=-g
GCC_CONFIGURATION=Debug
GCC_DIRECTORY=gcc-generic

while [[ $# -gt 0 ]]; do
  case $1 in
    -r|--release)
      GCC_DEBUG_FLAG="-O2"
      GCC_CONFIGURATION="Release"
      shift # past argument
      ;;
    -d|--debug)
      GCC_DEBUG_FLAG="-g"
      GCC_CONFIGURATION="Debug"
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      GCC_DIRECTORY="$1"
      shift # past argument
      ;;
  esac
done
rm -rf "_BuildOutput/${GCC_DIRECTORY}/${GCC_CONFIGURATION}"
mkdir -p "_BuildOutput/${GCC_DIRECTORY}/${GCC_CONFIGURATION}"

set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters
time g++-12 -std=c++14 -nostdinc++ ${GCC_DEBUG_FLAG} \
-o _BuildOutput/${GCC_DIRECTORY}/${GCC_CONFIGURATION}/SCTest \
Libraries/Foundation/Assert.cpp         \
Libraries/Foundation/Console.cpp        \
Libraries/Foundation/Memory.cpp         \
Libraries/Foundation/OSDarwin.cpp       \
Libraries/Foundation/OSPosix.cpp        \
Libraries/Foundation/StaticAsserts.cpp  \
Libraries/Foundation/String.cpp         \
Libraries/Foundation/StringBuilder.cpp  \
Libraries/Foundation/StringFormat.cpp   \
Libraries/Foundation/StringUtility.cpp  \
Libraries/Foundation/StringView.cpp     \
Libraries/Foundation/Test.cpp           \
${SCRIPT_DIR}/../../SCTest.cpp