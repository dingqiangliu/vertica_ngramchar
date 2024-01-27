/* Copyright (c) DingQiang Liu(dingqiangliu@gmail.com), 2012 - 2024 -*- C++ -*- */
/*
 * Description: User Defined Scalar Function to set configurations of NGram characters tokenizer
 *
 * Create Date: Jan 26, 2024
 */

#include "Vertica.h"
#include "VerticaDFS.h"
#include <sstream>
#include <string.h>
#include "util.h"

using namespace Vertica;
using namespace std;


class SetNGramCharTokenizerParameters : public ScalarFunction 
{
    void setup(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) override
    {
        ParamReader paramReader = srvInterface.getParamReader();
        if (!paramReader.containsParameter("proc_oid")) {
            vt_report_error(0, "Missing parameter 'proc_oid'");
        } 

        const VString &procOID = paramReader.getStringRef("proc_oid");
        string filename = GetConfigFileNameFromProcOID(procOID.str());
        DFSFile file = DFSFile(srvInterface, filename);
        if(file.exists()) {
            file.deleteIt(true);
        } 
        file.create(NS_GLOBAL, HINT_REPLICATE);
        fileWriter = DFSFileWriter(file);
        fileWriter.open();
    }


    using ScalarFunction::destroy;
    void destroy(ServerInterface &srvInterface, const SizedColumnTypes &argTypes) override
    {
        fileWriter.close();
    }


    void processBlock(ServerInterface &srvInterface, BlockReader &arg_reader, BlockWriter &res_writer) override
    {
        const vint minLen = arg_reader.getIntRef(0);
        const vint maxLen = arg_reader.getIntRef(1);
        if ((minLen <= 0) || (maxLen <= 0) || (minLen > maxLen))
            vt_report_error(0
                , "Parameter minLen should be less or equal than maxLen and positive, but current settings are minLen=%d, maxLen=%d ."
                , minLen, maxLen);

        stringstream ss;
        ss << "minLen=" << minLen << ",maxLen=" << maxLen;

        string line = ss.str();
        fileWriter.write(line.c_str(), line.length());

        res_writer.setBool(true);
        res_writer.next();
    }


private:
    DFSFileWriter fileWriter;
};


class SetNGramCharTokenizerParametersFactory : public ScalarFunctionFactory
{
    ScalarFunction* createScalarFunction(ServerInterface &interface) override
    {
        return vt_createFuncObject<SetNGramCharTokenizerParameters>(interface.allocator);
    }


    void getPrototype(ServerInterface &interface, ColumnTypes &argTypes, ColumnTypes &returnType) override
    {
        argTypes.addInt(); //minLen
        argTypes.addInt(); //maxLen
        returnType.addBool(); // success
    }


    void getReturnType(ServerInterface &srvInterface,
                       const SizedColumnTypes &argTypes,
                       SizedColumnTypes &returnType) override
    {
        returnType.addBool("success");
    }


    void getParameterType(ServerInterface &srvInterface, SizedColumnTypes &parameterTypes) override
    {
        parameterTypes.addVarchar(1024, "proc_oid");
    }
};


RegisterFactory(SetNGramCharTokenizerParametersFactory);
