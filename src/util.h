/* Copyright (c) 2007 EntIT Software LLC, a Micro Focus company -*- C++ -*- */
/**
 * \brief Implements UDx's to read/write from/to DFS
 */

#include "Vertica.h"
#include "VerticaDFS.h"
#include <map>
#include <string>

#ifndef UTIL_H
#define	UTIL_H

using namespace Vertica;
using namespace std;

string GetConfigFileName(ServerInterface &srvInterface);    

string GetConfigFileNameFromProcOID(const string &procOID);    

void ReadConfigFile(ServerInterface &srvInterface, DFSFile &file, map<string, int> &configuration);

#endif	/* UTIL_H */

