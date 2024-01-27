/* Copyright (c) DingQiang Liu(dingqiangliu@gmail.com), 2012 - 2024 -*- C++ -*- */
/*
 * Description: User Defined Transform Function for NGram characters tokenizer
 *
 * Create Date: Jan 26, 2024
 */

#include "Vertica.h"
#include <sstream>
#include "util.h"

using namespace Vertica;
using namespace std;

#define DEFAULT_minLen 1
#define DEFAULT_maxLen 3


static inline size_t
nextValidCharLength(const char *str, size_t len)
{
    char c = *str;
    if ((c & 0xc0) != 0xc0)
        return 1;
    size_t r;
    if ((c & 0x20) == 0)
        r = 2;                          // b'110xxxxx
    else if ((c & 0x10) == 0)
        r = 3;                          // b'1110xxxx
    else if ((c & 0x08) == 0)
        r = 4;                          // b'11110xxx
    else
        r = 1;                          // b'11111xxx (invalid)

    if (r > len)
        return 1;                       // character is too long
    switch (r) {
    case 4: if ((str[3] & 0xc0) != 0x80) return 1;
    case 3: if ((str[2] & 0xc0) != 0x80) return 1;
    case 2: if ((str[1] & 0xc0) != 0x80) return 1;
    default: ;
    }
    return r;
}


class NGramCharTokenizer : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface,
                                  PartitionReader &input_reader,
                                  PartitionWriter &output_writer)
    {
        do {
            size_t argCount = input_reader.getNumCols();
            size_t idxTextColum = (argCount == 1)?  0: 1;
            const VString &sentence = input_reader.getStringRef(idxTextColum);
            int length = sentence.length();

            // no tokens of NULL or empty string
            if(sentence.isNull() || (length == 0))
                continue;

            const char *str = sentence.data();

            // cache: [start_of_1st_char, end_of_1st_char/start_of_2nd_char, end_of_2nd_char/start_of_3rd_char, ...]
            size_t cacheCharEnd[maxLen+1]; 
            size_t cacheCharCount = 0;
            cacheCharEnd[cacheCharCount] = 0;
            size_t pos = cacheCharEnd[0];

            while(pos < length)
            {
                // cache a char
                size_t lenChar = nextValidCharLength(str + pos, length - pos);
                cacheCharCount++;
                pos += lenChar;
                cacheCharEnd[cacheCharCount] = pos;

                // each new char group with cacheCharEnd[0, cacheCharCount - minLen]
                if(cacheCharCount >= minLen)
                {
                    for(size_t n = 0; n <= cacheCharCount-minLen ; n++)
                    {
                        VString &token = output_writer.getStringRef(0);
                        token.copy(str + cacheCharEnd[n], cacheCharEnd[cacheCharCount] - cacheCharEnd[n]);
                        output_writer.next();
                        
                        // write the remaining arguments to output
                        size_t outputIdx = 1;
                        for (size_t inputIdx = 2; inputIdx < argCount; inputIdx++) {
                            output_writer.copyFromInput(outputIdx, input_reader, inputIdx);
                            outputIdx++;
                        }
                    }
                }

                // move cache left if it's full
                if(cacheCharCount == maxLen)
                {
                    for(size_t n = 0; n <= cacheCharCount-1; n++)
                        cacheCharEnd[n] = cacheCharEnd[n+1];

                    cacheCharCount--;
                }
            }
        } while (input_reader.next());
     }


    void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) override
    {
        // load configurations from DSF
        string fileName = GetConfigFileName(srvInterface);
        DFSFile file = DFSFile(srvInterface, fileName);
        if(file.exists())
        {
            map<string, int> configuration;
            ReadConfigFile(srvInterface, file, configuration);
            if(configuration.size() > 0)
            {
                minLen = configuration["minLen"];
                maxLen = configuration["maxLen"];
            }
        } 

        // check parameters
        ParamReader paramReader = srvInterface.getParamReader();
        if (paramReader.containsParameter("minLen"))
            minLen = paramReader.getIntRef("minLen");
        if (paramReader.containsParameter("maxLen"))
            maxLen = paramReader.getIntRef("maxLen");

        if ((minLen <= 0) || (maxLen <= 0) || (minLen > maxLen))
            vt_report_error(0
                , "Parameter minLen should be less or equal than maxLen and positive, but current settings are minLen=%d, maxLen=%d ."
                , minLen, maxLen);

    }


private:
    int minLen = DEFAULT_minLen;
    int maxLen = DEFAULT_maxLen;
};


class NGramCharTokenizerFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        ParamReader paramReader = srvInterface.getParamReader();
        argTypes.addAny();
        // Note: need not add any type to returnType. empty returnType means any columns and types!
    }


    virtual void getReturnType(ServerInterface &srvInterface,
                               const SizedColumnTypes &input_types,
                               SizedColumnTypes &output_types)
    {
        size_t argCount = input_types.getColumnCount();
        if (argCount == 0)
            vt_report_error(0, "Function needs at least 1 argument, but %zu provided", input_types.getColumnCount());

        // argument must be a varbinary or varchar, this is the column we tokenize
        size_t idxTextColum = (argCount == 1)?  0: 1;
        if (!input_types.getColumnType(idxTextColum).isStringType()) {
            vt_report_error(0, "Argument to tokenizer must be of varchar type.");
        }

        vint len = input_types.getColumnType(idxTextColum).getStringLength();
        output_types.addVarchar((len > 0? len: 1), "token");
    }


    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes)
    {
        parameterTypes.addInt("minLen");
        parameterTypes.addInt("maxLen");
    }


    virtual void getFunctionProperties(ServerInterface &srvInterface,
            const SizedColumnTypes &argTypes,
            Properties &properties) override
    {
        properties.isExploder = true;
    }


    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    {
        return vt_createFuncObj(srvInterface.allocator, NGramCharTokenizer);
    }
};


RegisterFactory(NGramCharTokenizerFactory);
