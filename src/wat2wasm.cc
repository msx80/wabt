#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "config.h"

#include "src/binary-writer.h"
#include "src/common.h"
#include "src/error-formatter.h"
#include "src/feature.h"
#include "src/filenames.h"
#include "src/ir.h"
#include "src/option-parser.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-parser.h"
#include "src/wat2wasm.h"

using namespace wabt;

extern "C" {

void* wat2wasm(const void* data, long unsigned int size, long unsigned int * outSize, char** errorStringToBeFreed) {

  *errorStringToBeFreed = NULL;

  std::unique_ptr<WastLexer> lexer = WastLexer::CreateBufferLexer("source.wasm", data, size);

  Errors errors;
  std::unique_ptr<Module> module;
  Features s_features;

  WastParseOptions parse_wast_options(s_features);
  Result result = ParseWatModule(lexer.get(), &module, &errors, &parse_wast_options);
  
  bool s_validate = true;
  if (Succeeded(result) && s_validate) {
    ValidateOptions options(s_features);
    result = ValidateModule(module.get(), &errors, options);
  }

  if (Succeeded(result)) {
   MemoryStream stream(NULL);
   WriteBinaryOptions s_write_binary_options;

    s_write_binary_options.features = s_features;
    result = WriteBinaryModule(&stream, module.get(), s_write_binary_options);
    stream.Flush();
    if (Succeeded(result)) {
        *outSize = stream.output_buffer().data.size();
        void* res = malloc(*outSize);
        memcpy(res, stream.output_buffer().data.data(), *outSize);
        return res;
    }
  }

  auto line_finder = lexer->MakeLineFinder();
  //FormatErrorsToFile(errors, Location::Type::Text, line_finder.get());

  std::string str = FormatErrorsToString(errors, Location::Type::Text, line_finder.get());
  *errorStringToBeFreed = (char*)malloc(str.length() + 1);
  strcpy(*errorStringToBeFreed, str.c_str());
  
  return NULL; // result != Result::Ok;
}

}
