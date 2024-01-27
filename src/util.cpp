#include "Vertica.h"
#include "VerticaDFS.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "util.h"


using namespace Vertica;
using namespace std;


#define ConfigDir "/ngramTokenizersConfigurations/"


string GetConfigFileName(ServerInterface &srvInterface)
{
    stringstream ss;
    ss << ConfigDir << srvInterface.getFunctionOid();
    return ss.str();
}


string GetConfigFileNameFromProcOID(const string &procOID)
{
    stringstream ss;
    ss << ConfigDir << procOID;
    return ss.str();
}


void ReadConfigFile(ServerInterface &srvInterface, DFSFile &file, map<string, int> &configuration)
{
    DFSFileReader fileReader = DFSFileReader(file);
    fileReader.open();   

    // Read input data
    size_t size;
    size = fileReader.size();

    //unsigned char idxBytes[size];    
    unsigned char* idxBytes = nullptr;
    try 
    {
        idxBytes = new unsigned char[size];
    } catch (std::bad_alloc &e)
    {
        vt_report_error(0, "Could not allocate [%zu] bytes", size);
    }
    
    size_t br = fileReader.read(idxBytes, size);
    if (br == 0)
        return;

    //convert the read buffer into string
    string line(reinterpret_cast<char*> (idxBytes), size);
    ::size_t pos = 0;
    while(pos != string::npos)
    {    
        ::size_t found = line.find("=", pos);
        if(found != string::npos)
        { 
            string name = line.substr(pos, found-pos);   
            pos = line.find(",", found + 1);
            string value;
            if(pos != string::npos)
            {
                value = line.substr(found + 1, pos - found - 1);
                pos++;
            }
            else
            {
                value = line.substr(found + 1);
            }
            int iValue = atoi(value.c_str());
            configuration[name] = iValue;
        }                                         
    }

    
    delete []idxBytes;

    fileReader.close();
}

